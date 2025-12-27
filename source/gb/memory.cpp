#include "included/memory.hpp"
#include "included/state.hpp"
#include "included/cartridge.hpp"
#include "included/joypad.hpp"
#include "included/apu.hpp"
#include <cstring>

namespace gb {
    namespace memory {

        void initialize(GBState& state) {
            auto& mem = state.memory;

            memset(mem.vram, 0, sizeof(mem.vram));
            memset(mem.wram, 0, sizeof(mem.wram));
            memset(mem.oam, 0, sizeof(mem.oam));
            memset(mem.io, 0, sizeof(mem.io));
            memset(mem.hram, 0, sizeof(mem.hram));
            mem.ie = 0;

            // Initial IO values
            mem.io[IO_LCDC] = 0x91;
            mem.io[IO_STAT] = 0x85;
            mem.io[IO_BGP]  = 0xFC;
        }

        uint8_t read(GBState& state, uint16_t address) {
            auto& mem = state.memory;

            // ROM
            if (address < 0x8000) {
                return cartridge::read(state, address);
            }

            // VRAM
            if (address < 0xA000) {
                return mem.vram[address - 0x8000];
            }

            // External RAM (cartridge)
            if (address < 0xC000) {
                return cartridge::readRAM(state, address);
            }

            // Work RAM
            if (address < 0xE000) {
                return mem.wram[address - 0xC000];
            }

            // Echo RAM
            if (address < 0xFE00) {
                return mem.wram[address - 0xE000];
            }

            // OAM
            if (address < 0xFEA0) {
                return mem.oam[address - 0xFE00];
            }

            // Unusable
            if (address < 0xFF00) {
                return 0xFF;
            }

            // IO registers
            if (address < 0xFF80) {
                uint8_t reg = address - 0xFF00;

                if (reg == IO_JOYP) {
                    return joypad::read(state);
                }

                if (reg >= 0x10 && reg <= 0x3F) {
                    return apu::readRegister(state, reg);
                }

                return mem.io[reg];
            }

            // High RAM
            if (address < 0xFFFF) {
                return mem.hram[address - 0xFF80];
            }

            // Interrupt Enable
            return mem.ie;
        }

        void write(GBState& state, uint16_t address, uint8_t value) {
            auto& mem = state.memory;

            // ROM (cartridge handles banking)
            if (address < 0x8000) {
                cartridge::write(state, address, value);
                return;
            }

            // VRAM
            if (address < 0xA000) {
                mem.vram[address - 0x8000] = value;
                return;
            }

            // External RAM
            if (address < 0xC000) {
                cartridge::writeRAM(state, address, value);
                return;
            }

            // Work RAM
            if (address < 0xE000) {
                mem.wram[address - 0xC000] = value;
                return;
            }

            // Echo RAM
            if (address < 0xFE00) {
                mem.wram[address - 0xE000] = value;
                return;
            }

            // OAM
            if (address < 0xFEA0) {
                mem.oam[address - 0xFE00] = value;
                return;
            }

            // Unusable
            if (address < 0xFF00) {
                return;
            }

            // IO registers
            if (address < 0xFF80) {
                uint8_t reg = address - 0xFF00;

                if (reg == IO_JOYP) {
                    joypad::write(state, value);
                    return;
                }

                if (reg == IO_DIV) {
                    mem.io[IO_DIV] = 0;
                    return;
                }

                if (reg == IO_DMA) {
                    doDMA(state, value);
                    return;
                }

                if (reg >= 0x10 && reg <= 0x3F) {
                    apu::writeRegister(state, reg, value);
                    return;
                }

                mem.io[reg] = value;
                return;
            }

            // High RAM
            if (address < 0xFFFF) {
                mem.hram[address - 0xFF80] = value;
                return;
            }

            // Interrupt Enable
            mem.ie = value;
        }

        void doDMA(GBState& state, uint8_t value) {
            uint16_t source = value << 8;
            for (int i = 0; i < 0xA0; i++) {
                state.memory.oam[i] = read(state, source + i);
            }
        }

    }
}
