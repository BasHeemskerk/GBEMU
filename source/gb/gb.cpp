#include "included/gb.hpp"
#include "included/state.hpp"
#include "included/cpu.hpp"
#include "included/memory.hpp"
#include "included/cartridge.hpp"
#include "included/ppu.hpp"
#include "included/timer.hpp"
#include "included/joypad.hpp"
#include "included/apu.hpp"

namespace gb {

    // Global emulator state
    static GBState g_state;

    GBState& getState() {
        return g_state;
    }

    void initialize() {
        cpu::initialize(g_state);
        memory::initialize(g_state);
        cartridge::initialize(g_state);
        ppu::initialize(g_state);
        timer::initialize(g_state);
        joypad::initialize(g_state);
        apu::initialize(g_state);
    }

    bool loadROM(const char* filepath) {
        return cartridge::loadRom(g_state, filepath);
    }

    void runFrame() {
        g_state.ppu.frameReady = false;

        while (!g_state.ppu.frameReady) {
            step();
        }
    }

    void step() {
        int cycles = cpu::step(g_state);
        ppu::tick(g_state, cycles);
        timer::tick(g_state, cycles);
        apu::tick(g_state, cycles);
        handleInterrupts();
    }

    void handleInterrupts() {
        if (!g_state.cpu.ime) {
            return;
        }

        uint8_t pending = g_state.memory.io[memory::IO_IF] & g_state.memory.ie & 0x1F;

        if (pending == 0) {
            return;
        }

        g_state.cpu.halted = false;

        uint16_t handler = 0;
        uint8_t bit = 0;

        if (pending & 0x01) {
            handler = 0x0040;
            bit = 0x01;
        }
        else if (pending & 0x02) {
            handler = 0x0048;
            bit = 0x02;
        }
        else if (pending & 0x04) {
            handler = 0x0050;
            bit = 0x04;
        }
        else if (pending & 0x08) {
            handler = 0x0058;
            bit = 0x08;
        }
        else if (pending & 0x10) {
            handler = 0x0060;
            bit = 0x10;
        }

        if (handler != 0) {
            g_state.cpu.ime = false;
            g_state.memory.io[memory::IO_IF] &= ~bit;

            g_state.cpu.SP--;
            memory::write(g_state, g_state.cpu.SP, g_state.cpu.PC >> 8);
            g_state.cpu.SP--;
            memory::write(g_state, g_state.cpu.SP, g_state.cpu.PC & 0xFF);
            g_state.cpu.PC = handler;
        }
    }

    // Accessors
    uint8_t* getFramebuffer() {
        return g_state.ppu.framebuffer;
    }

    bool isFrameReady() {
        return g_state.ppu.frameReady;
    }

    void setFrameReady(bool ready) {
        g_state.ppu.frameReady = ready;
    }

    int16_t* getAudioBuffer() {
        return g_state.apu.audioBuffer;
    }

    int getAudioBufferPosition() {
        return g_state.apu.bufferPosition;
    }

    void setAudioBufferPosition(int pos) {
        g_state.apu.bufferPosition = pos;
    }

    const char* getROMTitle() {
        return cartridge::getTitle(g_state);
    }

    int getRAMSize() {
        return g_state.cartridge.ramSize;
    }

}  // namespace gb
