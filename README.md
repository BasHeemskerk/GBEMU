# GB-EMU

A Game Boy emulator built from scratch in C++.

Currently targets Nintendo 3DS. Multi-platform support planned.

## Status

**Work in progress.** Core emulation works - games are playable.

- [x] CPU (all opcodes)
- [x] PPU (background, window, sprites)
- [x] Memory banking (MBC1, MBC3)
- [x] Timer
- [x] Joypad
- [x] ROM loading / file browser
- [ ] Audio (implemented but not connected)
- [ ] Save data (SRAM)

## Building

Requires [devkitPro](https://devkitpro.org/) with 3DS toolchain.

```bash
make
```

Output: `gbemu.3dsx`

## Usage

1. Place ROMs in `sdmc:/roms/` on your 3DS SD card
2. Launch the homebrew
3. Select a ROM
4. Play

**Controls:**
| Game Boy | 3DS |
|----------|-----|
| D-Pad | D-Pad |
| A | A |
| B | B |
| Start | Start |
| Select | Select |

`START + SELECT` to return to ROM menu.

## Planned

**Short term:**
- Audio output
- SRAM saves
- Save states
- Performance optimization

**Architecture overhaul:**
- GameBoy wrapper class (instantiable emulator object)
- Platform abstraction layer
- Portable UI library
- Multi-platform builds (DS, PSP, PC)

**Features:**
- Replay recording/playback
- Rewind
- Link cable (dual instance)
- Netplay
- Custom opcode tables (runtime-swappable CPU definitions)

## Structure

```
source/
├── gb/              # Core emulation (platform-independent)
│   ├── cpu.cpp      # Z80-like CPU, all opcodes
│   ├── ppu.cpp      # Pixel Processing Unit
│   ├── memory.cpp   # Memory mapping, banking
│   ├── timer.cpp    # Timer registers
│   ├── apu.cpp      # Audio (WIP)
│   ├── cartridge.cpp
│   └── joypad.cpp
│
├── gui.cpp          # 3DS UI / rendering
└── main.cpp         # 3DS entry point
```

## License

MIT
