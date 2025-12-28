# GB-EMU

A Game Boy emulator built from scratch in C++.

Currently targets Nintendo 3DS. Multi-platform support planned.

## Status

**Work in progress.** Core emulation works - games are playable.

- [x] CPU (all opcodes via data-driven microcode system)
- [x] PPU (background, window, sprites with 10-per-scanline limit)
- [x] Memory banking (MBC1, MBC3, MBC5)
- [x] Timer
- [x] Joypad
- [x] ROM loading / file browser
- [x] Audio (APU with threaded output)
- [ ] Save data (SRAM)

## Building

Requires [devkitPro](https://devkitpro.org/) with 3DS toolchain.

```bash
# Install devkitPro packages
sudo dkp-pacman -S 3ds-dev

# Build
make
```

Output: `gbemu.3dsx`

## Usage

1. Place ROMs in `sdmc:/gb_roms/` on your 3DS SD card
2. **For audio:** Dump DSP firmware using [DSP1](https://github.com/zoogie/DSP1) → `sdmc:/3ds/dspfirm.cdc`
3. Launch the homebrew
4. Select a ROM
5. Play

**Controls:**
| Game Boy | 3DS |
|----------|-----|
| D-Pad | D-Pad |
| A | A |
| B | B |
| Start | Start |
| Select | Select |
| - | L: Toggle audio |
| - | R: Test tone (debug) |

`START + SELECT` to exit to Homebrew Menu.

## Architecture

The emulator separates platform-independent emulation (the "core") from platform-specific code (rendering, audio output, input). This allows the same Game Boy emulation logic to run on different hardware with minimal porting effort.

### The Wrapper

The `GameBoy` class provides a clean interface to the emulator core. All internal state is hidden - platform code only interacts through simple methods:

```cpp
// wrapper/gameboy.cpp - simplified view of the main loop

void GameBoy::runFrame() {
    if (!romLoaded) return;
    
    updateInput();
    state.ppu.frameReady = false;
    
    while (!state.ppu.frameReady) {
        // Run CPU instructions in batches for efficiency
        int batchCycles = 0;
        
        for (int i = 0; i < 16 && !state.ppu.frameReady; i++) {
            int cycles = gb::cpu::step(state);
            batchCycles += cycles;
            
            if (state.cpu.ime) {
                handleInterrupts();
            }
        }
        
        // Tick all subsystems with accumulated cycles
        gb::ppu::tick(state, batchCycles);
        gb::timer::tick(state, batchCycles);
        gb::apu::tick(state, batchCycles);
    }
}
```

Porting to a new platform becomes straightforward:

```cpp
GameBoy gb;
gb.init();
gb.loadOpcodeTable("opcodes/default.gb_opcode");
gb.loadROM("game.gb");

while (running) {
    // Update input
    gb.input.a = isButtonPressed(BUTTON_A);
    gb.input.b = isButtonPressed(BUTTON_B);
    // ...
    
    // Run one frame
    gb.runFrame();
    
    // Render - framebuffer is 160x144 pixels, 2-bit palette indices (0-3)
    uint8_t* pixels = gb.getFramebuffer();
    drawToScreen(pixels);
    
    // Audio - read samples from ring buffer in your audio thread
    // gb.getAPUState().ringBuffer contains stereo 16-bit samples
}
```

### Core Emulation

#### CPU - Data-Driven Opcode System

Instead of hardcoding all 512 opcode implementations, opcodes are defined in external `.gb_opcode` files that are parsed at runtime. Each opcode becomes an `OpcodeEntry`:

```cpp
// gb/included/opcode_parser.hpp

struct OpcodeEntry {
    MicroOp op;         // The operation (LD8, ADD8, JP, etc.)
    Operand dst;        // Destination operand
    Operand src;        // Source operand
    uint8_t cycles;     // Clock cycles (normal)
    uint8_t cyclesBranch; // Clock cycles (branch not taken)
};

struct OpcodeTable {
    char name[32];
    uint8_t version;
    OpcodeEntry main[256];  // 0x00-0xFF
    OpcodeEntry cb[256];    // CB-prefixed opcodes
    bool loaded;
};
```

The CPU executor is a simple loop that fetches opcodes and dispatches based on the `MicroOp`:

```cpp
// gb/cpu.cpp

int cpu::step(GBState& state) {
    if (state.cpu.halted) return 4;
    
    uint8_t opcode = memory::read(state, state.cpu.PC++);
    const OpcodeEntry& entry = state.opcodes.main[opcode];
    
    return executeOp(state, entry);
}

static int executeOp(GBState& state, const OpcodeEntry& entry) {
    switch (entry.op) {
        case MicroOp::LD8: {
            uint8_t val = getValue8(state, entry.src);
            setValue8(state, entry.dst, val);
            break;
        }
        
        case MicroOp::ADD8: {
            uint8_t val = getValue8(state, entry.src);
            int result = state.cpu.A + val;
            setFlags(state, 
                (result & 0xFF) == 0,                        // Z
                false,                                        // N
                ((state.cpu.A & 0x0F) + (val & 0x0F)) > 0x0F, // H
                result > 0xFF);                               // C
            state.cpu.A = result & 0xFF;
            break;
        }
        
        case MicroOp::JP: {
            state.cpu.PC = getValue16(state, entry.dst);
            break;
        }
        
        case MicroOp::CB: {
            uint8_t cbOpcode = memory::read(state, state.cpu.PC++);
            return executeOp(state, state.opcodes.cb[cbOpcode]);
        }
        
        // ... ~50 more cases
    }
    return entry.cycles;
}
```

Operands are resolved through helper functions that handle registers, memory, and immediates uniformly:

```cpp
static uint8_t getValue8(GBState& state, Operand op) {
    switch (op) {
        case Operand::A: return state.cpu.A;
        case Operand::B: return state.cpu.B;
        // ... other registers
        case Operand::MEM_HL: return memory::read(state, state.cpu.HL);
        case Operand::MEM_HL_INC: return memory::read(state, state.cpu.HL++);
        case Operand::MEM_HL_DEC: return memory::read(state, state.cpu.HL--);
        case Operand::IMM8: return memory::read(state, state.cpu.PC++);
        // ...
    }
}
```

#### The `.gb_opcode` File Format

Opcode definitions are human-readable text files:

```
#GB_OPCODE_INFO
NAME: Default
VERSION: 1
#GB_OPCODE_INFO_END

#GB_OPCODE_TABLE
; Format: opcode | cycles | MICROOP dst, src
; Cycles can be "12" or "12/8" for conditional ops (taken/not taken)

0x00 | 4 | NOP
0x01 | 12 | LD16 BC, nn
0x02 | 8 | ST8 (BC), A
0x03 | 8 | INC16 BC
0x04 | 4 | INC8 B
0x05 | 4 | DEC8 B
0x06 | 8 | LD8 B, n
0x0A | 8 | LD8 A, (BC)
0x18 | 12 | JR e
0x20 | 12/8 | JR_NZ e
0x3E | 8 | LD8 A, n
0x77 | 8 | ST8 (HL), A
0xC3 | 16 | JP nn
0xC9 | 16 | RET
0xCD | 24 | CALL nn
0xCB | 4 | CB
#GB_OPCODE_TABLE_END

#GB_OPCODE_CB_TABLE
0x00 | 8 | RLC B
0x40 | 8 | BIT 0, B
0x80 | 8 | RES 0, B
0xC0 | 8 | SET 0, B
#GB_OPCODE_CB_TABLE_END
```

**Available operands:**

| Type | Operands |
|------|----------|
| 8-bit registers | `A` `B` `C` `D` `E` `H` `L` |
| 16-bit registers | `AF` `BC` `DE` `HL` `SP` |
| Memory (register) | `(BC)` `(DE)` `(HL)` `(HL+)` `(HL-)` |
| Memory (immediate) | `(nn)` `(FF00+n)` `(FF00+C)` |
| Immediates | `n` (8-bit) `nn` (16-bit) `e` (signed 8-bit) `SP+e` |
| Bit indices | `0` `1` `2` `3` `4` `5` `6` `7` |
| RST vectors | `00H` `08H` `10H` `18H` `20H` `28H` `30H` `38H` |

This design allows opcode behavior to be modified or corrected without recompiling the emulator.

#### PPU - Scanline Renderer with Lookup Tables

The PPU renders one scanline at a time, driven by cycle counting. The key optimization is a precomputed lookup table that decodes tile bytes directly to pixel colors:

```cpp
// gb/ppu.cpp

// Precomputed decode table: tileLUT[high_byte][low_byte][pixel] = color (0-3)
uint8_t tileLUT[256][256][8];  // 512 KB

void buildLUT() {
    for (int high = 0; high < 256; high++) {
        for (int low = 0; low < 256; low++) {
            for (int px = 0; px < 8; px++) {
                int bit = 7 - px;
                uint8_t lowBit = (low >> bit) & 1;
                uint8_t highBit = (high >> bit) & 1;
                tileLUT[high][low][px] = (highBit << 1) | lowBit;
            }
        }
    }
}
```

Background rendering becomes a series of table lookups:

```cpp
void renderBackground(GBState& state) {
    // ... setup code ...
    
    while (screenX < SCREEN_WIDTH) {
        // Fetch tile number from map
        uint8_t tileNum = mem.vram[mapRowBase + tileX];
        
        // Calculate tile data address
        uint16_t tileAddr = tileData + (tileNum << 4) + (pixelY << 1);
        
        // Fetch the two bytes for this tile row
        uint8_t low = mem.vram[tileAddr];
        uint8_t high = mem.vram[tileAddr + 1];
        
        // ONE lookup gives us all 8 pixel colors
        uint8_t* tilePixels = tileLUT[high][low];
        
        // Copy pixels to framebuffer
        for (int px = startPixel; px < endPixel; px++) {
            fb[screenX++] = colors[tilePixels[px]];
        }
    }
}
```

Without the LUT, each pixel would require 4 shifts, 2 ANDs, and 1 OR. With 160×144 pixels at 60fps, that's millions of operations saved per second.

Sprite rendering enforces the hardware limit of 10 sprites per scanline:

```cpp
void renderSprites(GBState& state) {
    int spritesDrawn = 0;
    
    // Loop through all 40 sprites in OAM
    for (int i = 39; i >= 0 && spritesDrawn < 10; i--) {
        uint8_t* sprite = &mem.oam[i * 4];
        
        int y = sprite[0] - 16;
        if (ly < y || ly >= y + spriteHeight) continue;  // Not on this scanline
        
        // ... render sprite pixels ...
        
        spritesDrawn++;
    }
}
```

#### APU - Lock-Free Ring Buffer

The APU generates samples on the main thread while a separate audio thread outputs them. They communicate via a lock-free ring buffer:

```cpp
// gb/included/state.hpp

struct APUState {
    static constexpr int RING_BUFFER_SIZE = 4096;
    int16_t ringBuffer[RING_BUFFER_SIZE * 2];  // Stereo
    volatile int writePos;  // Written by main thread only
    volatile int readPos;   // Written by audio thread only
    // ... channel state ...
};
```

The main thread writes samples:

```cpp
// gb/apu.cpp - called during generateSample()

int nextWrite = (apu.writePos + 1) & (RING_BUFFER_SIZE - 1);
if (nextWrite != apu.readPos) {  // Buffer not full
    apu.ringBuffer[apu.writePos * 2] = leftSample;
    apu.ringBuffer[apu.writePos * 2 + 1] = rightSample;
    apu.writePos = nextWrite;
}
```

The audio thread reads samples:

```cpp
// 3ds/audio.cpp - runs on CPU core 1

while (apu.readPos != apu.writePos) {  // Buffer not empty
    int16_t left = apu.ringBuffer[apu.readPos * 2];
    int16_t right = apu.ringBuffer[apu.readPos * 2 + 1];
    // ... output to hardware ...
    apu.readPos = (apu.readPos + 1) & (RING_BUFFER_SIZE - 1);
}
```

No mutexes are needed because each index is only written by one thread. The `volatile` keyword ensures the compiler doesn't cache the values.

## Structure

```
source/
├── gb/                     # Core emulation (platform-independent)
│   ├── cpu.cpp             # Opcode executor
│   ├── ppu.cpp             # Scanline renderer with tile LUTs
│   ├── apu.cpp             # 4-channel audio, ring buffer output
│   ├── memory.cpp          # Memory map, bank switching, IO routing
│   ├── timer.cpp           # DIV/TIMA registers
│   ├── cartridge.cpp       # ROM loading, MBC1/3/5 emulation
│   ├── joypad.cpp          # Button state
│   ├── opcode_parser.cpp   # .gb_opcode file parser
│   └── included/
│       ├── state.hpp       # All emulator state (GBState struct)
│       └── opcode_parser.hpp # MicroOp/Operand enums, OpcodeTable
│
├── wrapper/
│   ├── gameboy.cpp         # GameBoy class implementation
│   └── included/
│       └── gameboy.hpp     # Public interface
│
├── 3ds/                    # Nintendo 3DS platform
│   ├── main.cpp            # Entry point, main loop
│   ├── gui.cpp             # File browser, scaled rendering
│   ├── audio.cpp           # Threaded ndsp output
│   └── included/
│
├── nds/                    # Nintendo DS (planned)
├── psp/                    # PlayStation Portable (planned)
└── include/
    └── platform.hpp        # Platform detection macros

romfs/
└── opcodes/
    └── default.gb_opcode   # CPU opcode definitions
```

## Planned

**Short term:**
- SRAM saves
- Save states

**Multi-platform:**
- Nintendo DS
- PSP
- PC (SDL)

**Features:**
- Rewind
- Fast forward
- Link cable emulation
- GBC support

## License

MIT