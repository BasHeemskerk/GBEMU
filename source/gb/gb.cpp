#include "included/gb.hpp"
#include "included/cpu.hpp"
#include "included/memory.hpp"
#include "included/cartridge.hpp"
#include "included/ppu.hpp"
#include "included/timer.hpp"
#include "included/joypad.hpp"
#include "included/apu.hpp"

namespace gb {

    void initialize() {
        cpu::initialize();
        memory::initialize();
        cartridge::initialize();
        ppu::initialize();
        timer::initialize();
        joypad::initialize();
        apu::initialize();
        apu::initAudio();
    }

    bool loadROM(const char* filepath) {
        return cartridge::loadRom(filepath);
    }

    void runFrame() {
        // run until PPU signals frame is ready
        ppu::frameReady = false;

        while (!ppu::frameReady) {
            step();
        }
    }

    void step() {
        // run one CPU instruction
        int cycles = cpu::step();

        // update other components with elapsed cycles
        ppu::tick(cycles);
        timer::tick(cycles);
        apu::tick(cycles);

        // handle interrupts
        handleInterrupts();
    }

    void handleInterrupts() {
        // check if interrupts are enabled
        if (!cpu::ime) {
            return;
        }

        // get pending interrupts (IF AND IE)
        uint8_t pending = memory::io[memory::IO_IF] & memory::ie & 0x1F;

        if (pending == 0) {
            return;
        }

        // wake from halt
        cpu::halted = false;

        // handle highest priority interrupt
        // priority: vblank > stat > timer > serial > joypad

        uint16_t handler = 0;
        uint8_t bit = 0;

        if (pending & 0x01) {
            // vblank interrupt
            handler = 0x0040;
            bit = 0x01;
        }
        else if (pending & 0x02) {
            // LCD STAT interrupt
            handler = 0x0048;
            bit = 0x02;
        }
        else if (pending & 0x04) {
            // timer interrupt
            handler = 0x0050;
            bit = 0x04;
        }
        else if (pending & 0x08) {
            // serial interrupt
            handler = 0x0058;
            bit = 0x08;
        }
        else if (pending & 0x10) {
            // joypad interrupt
            handler = 0x0060;
            bit = 0x10;
        }

        if (handler != 0) {
            // disable interrupts
            cpu::ime = false;

            // clear the interrupt flag
            memory::io[memory::IO_IF] &= ~bit;

            // push PC onto stack and jump to handler
            cpu::SP--;
            memory::write(cpu::SP, cpu::PC >> 8);
            cpu::SP--;
            memory::write(cpu::SP, cpu::PC & 0xFF);
            cpu::PC = handler;
        }
    }

}  // namespace gb