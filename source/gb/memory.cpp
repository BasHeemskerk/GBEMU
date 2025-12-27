#include "included/memory.hpp"
#include "included/cartridge.hpp"
#include "included/ppu.hpp"
#include "included/timer.hpp"
#include "included/joypad.hpp"
#include "included/apu.hpp"
#include <cstring>
#include <cstdio>

namespace gb{
    namespace memory{
        uint8_t vram[0x2000];   // 8KB video RAM
        uint8_t wram[0x2000];   // 8KB work RAM
        uint8_t hram[0x7F];     // 127 bytes high RAM
        uint8_t oam[0xA0];      // 160 bytes sprite data
        uint8_t io[0x80];       // 128 bytes I/O registers
        uint8_t ie;

        void initialize(){
            memset(vram, 0, sizeof(vram));
            memset(wram, 0, sizeof(wram));
            memset(hram, 0, sizeof(hram));
            memset(oam, 0, sizeof(oam));
            memset(io, 0, sizeof(io));
            ie = 0;

            io[IO_JOYP] = 0xCF;  // joypad - all buttons released
            io[IO_DIV]  = 0xAB;  // divider
            io[IO_TAC]  = 0xF8;  // timer control
            io[IO_IF]   = 0xE1;  // interrupt flag
            io[IO_LCDC] = 0x91;  // LCD on, BG on
            io[IO_STAT] = 0x85;  // LCD status
            io[IO_SCY]  = 0x00;  // scroll Y
            io[IO_SCX]  = 0x00;  // scroll X
            io[IO_LY]   = 0x00;  // current scanline
            io[IO_LYC]  = 0x00;  // scanline compare
            io[IO_BGP]  = 0xFC;  // background palette
            io[IO_OBP0] = 0xFF;  // object palette 0
            io[IO_OBP1] = 0xFF;  // object palette 1
            io[IO_WY]   = 0x00;  // window Y
            io[IO_WX]   = 0x00;  // window X
        }

        uint8_t read(uint16_t address) {
            // ===========================================
            // Route read to correct memory region
            // ===========================================

            if (address < 0x8000) {
                // 0x0000 - 0x7FFF: Cartridge ROM
                // first 16KB is bank 0, second 16KB is switchable
                return cartridge::read(address);
            }
            else if (address < 0xA000) {
                // 0x8000 - 0x9FFF: Video RAM
                // contains tile data and background maps
                return vram[address - 0x8000];
            }
            else if (address < 0xC000) {
                // 0xA000 - 0xBFFF: External RAM (cartridge)
                // optional, may be battery backed for saves
                return cartridge::readRAM(address - 0xA000);
            }
            else if (address < 0xE000) {
                // 0xC000 - 0xDFFF: Work RAM
                // general purpose RAM for the game
                return wram[address - 0xC000];
            }
            else if (address < 0xFE00) {
                // 0xE000 - 0xFDFF: Echo RAM
                // mirror of 0xC000 - 0xDDFF
                // we just read from WRAM with adjusted address
                return wram[address - 0xE000];
            }
            else if (address < 0xFEA0) {
                // 0xFE00 - 0xFE9F: OAM (sprite attribute table)
                // 40 sprites × 4 bytes = 160 bytes
                return oam[address - 0xFE00];
            }
            else if (address < 0xFF00) {
                // 0xFEA0 - 0xFEFF: Unusable
                // returns 0xFF on real hardware (usually)
                return 0xFF;
            }
            else if (address < 0xFF80) {
                // 0xFF00 - 0xFF7F: I/O registers
                // needs special handling for some registers
                return readIO(address - 0xFF00);
            }
            else if (address < 0xFFFF) {
                // 0xFF80 - 0xFFFE: High RAM
                // fast RAM, accessible during DMA
                return hram[address - 0xFF80];
            }
            else {
                // 0xFFFF: Interrupt Enable register
                return ie;
            }
        }

        void write(uint16_t address, uint8_t value) {
            // ===========================================
            // Route write to correct memory region
            // ===========================================

            if (address < 0x8000) {
                // 0x0000 - 0x7FFF: Cartridge ROM
                // writes here control the memory bank controller (MBC)
                // the cartridge handles this
                cartridge::write(address, value);
            }
            else if (address < 0xA000) {
                // 0x8000 - 0x9FFF: Video RAM
                // PPU might block access during certain modes
                // for now we just write directly
                vram[address - 0x8000] = value;
            }
            else if (address < 0xC000) {
                // 0xA000 - 0xBFFF: External RAM (cartridge)
                cartridge::writeRAM(address - 0xA000, value);
            }
            else if (address < 0xE000) {
                // 0xC000 - 0xDFFF: Work RAM
                wram[address - 0xC000] = value;
            }
            else if (address < 0xFE00) {
                // 0xE000 - 0xFDFF: Echo RAM
                // writes here also write to WRAM
                wram[address - 0xE000] = value;
            }
            else if (address < 0xFEA0) {
                // 0xFE00 - 0xFE9F: OAM
                oam[address - 0xFE00] = value;
            }
            else if (address < 0xFF00) {
                // 0xFEA0 - 0xFEFF: Unusable
                // writes are ignored
            }
            else if (address < 0xFF80) {
                // 0xFF00 - 0xFF7F: I/O registers
                // needs special handling
                writeIO(address - 0xFF00, value);
            }
            else if (address < 0xFFFF) {
                // 0xFF80 - 0xFFFE: High RAM
                hram[address - 0xFF80] = value;
            }
            else {
                // 0xFFFF: Interrupt Enable register
                ie = value;
            }
        }

        uint8_t readIO(uint8_t reg) {
            // ===========================================
            // Handle I/O register reads
            // Some registers have special behavior
            // ===========================================

            switch (reg) {
                case IO_JOYP:
                    // joypad register - needs special handling
                    // returns button states based on selection bits
                    return joypad::read();

                case IO_DIV:
                    // divider register
                    // increments at 16384 Hz
                    return io[IO_DIV];

                case IO_TIMA:
                    // timer counter
                    return io[IO_TIMA];

                case IO_TMA:
                    // timer modulo
                    return io[IO_TMA];

                case IO_TAC:
                    // timer control
                    // only bits 0-2 are used, rest return 1
                    return io[IO_TAC] | 0xF8;

                case IO_IF:
                    // interrupt flag
                    // only bits 0-4 are used, rest return 1
                    return io[IO_IF] | 0xE0;

                case IO_LCDC:
                    // LCD control
                    return io[IO_LCDC];

                case IO_STAT:
                    // LCD status
                    // bit 7 always returns 1
                    return io[IO_STAT] | 0x80;

                case IO_LY:
                    // current scanline (0-153)
                    // read-only, set by PPU
                    return io[IO_LY];

                default:
                    // most registers just return their value
                    return io[reg];
            }
        }

        void writeIO(uint8_t reg, uint8_t value) {
            // ===========================================
            // Handle I/O register writes
            // Some registers have special behavior
            // ===========================================

            if (reg >= 0x10 && reg <= 0x3F){
                apu::writeRegister(reg, value);
                io[reg] = value;
                return;
            }

            switch (reg) {
                case IO_JOYP:
                    // joypad register
                    // only bits 4-5 are writable (select buttons/dpad)
                    joypad::write(value);
                    io[IO_JOYP] = value;
                    break;

                case IO_DIV:
                    // divider register
                    // any write resets it to 0
                    io[IO_DIV] = 0;
                    break;

                case IO_TIMA:
                    // timer counter
                    io[IO_TIMA] = value;
                    break;

                case IO_TMA:
                    // timer modulo (reload value)
                    io[IO_TMA] = value;
                    break;

                case IO_TAC:
                    // timer control
                    // only bits 0-2 are writable
                    io[IO_TAC] = value & 0x07;
                    break;

                case IO_IF:
                    // interrupt flag
                    // only bits 0-4 are writable
                    io[IO_IF] = value & 0x1F;
                    break;

                case IO_LCDC:
                    // LCD control
                    // bit 7 turns LCD on/off
                    // turning off LCD only allowed during vblank
                    io[IO_LCDC] = value;
                    break;

                case IO_STAT:
                    // LCD status
                    // bits 0-2 are read-only (set by PPU)
                    // bits 3-6 are writable (interrupt enables)
                    io[IO_STAT] = (io[IO_STAT] & 0x07) | (value & 0x78);
                    break;

                case IO_SCY:
                    // scroll Y
                    io[IO_SCY] = value;
                    break;

                case IO_SCX:
                    // scroll X
                    io[IO_SCX] = value;
                    break;

                case IO_LY:
                    // current scanline
                    // read-only, writes are ignored
                    // (some docs say it resets to 0, but most games don't rely on this)
                    break;

                case IO_LYC:
                    // scanline compare
                    io[IO_LYC] = value;
                    break;

                case IO_DMA:
                    // DMA transfer
                    // writing here starts a DMA transfer from (value * 0x100) to OAM
                    // transfer copies 160 bytes, takes 160 cycles
                    // for now we do it instantly
                    {
                        uint16_t source = value << 8;  // value * 0x100
                        for (int i = 0; i < 160; i++) {
                            oam[i] = read(source + i);
                        }
                    }
                    io[IO_DMA] = value;
                    break;

                case IO_BGP:
                    // background palette
                    // maps tile colors 0-3 to actual shades
                    io[IO_BGP] = value;
                    break;

                case IO_OBP0:
                    // object palette 0
                    io[IO_OBP0] = value;
                    break;

                case IO_OBP1:
                    // object palette 1
                    io[IO_OBP1] = value;
                    break;

                case IO_WY:
                    // window Y position
                    io[IO_WY] = value;
                    break;

                case IO_WX:
                    // window X position
                    io[IO_WX] = value;
                    break;

                default:
                    // most registers just store the value
                    io[reg] = value;
                    break;
            }
        }
    }
}