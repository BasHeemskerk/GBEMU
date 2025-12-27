#include "included/ppu.hpp"
#include "included/state.hpp"
#include "included/memory.hpp"
#include <cstring>

namespace gb {
    namespace ppu {

        void initialize(GBState& state) {
            auto& ppu = state.ppu;

            memset(ppu.framebuffer, 0, sizeof(ppu.framebuffer));
            ppu.frameReady = false;
            ppu.scanlineCycles = 0;
        }

        void tick(GBState& state, int cycles) {
            auto& ppu = state.ppu;
            auto& io = state.memory.io;

            uint8_t lcdc = io[memory::IO_LCDC];

            if (!(lcdc & 0x80)) {
                return;
            }

            ppu.scanlineCycles += cycles;

            uint8_t stat = io[memory::IO_STAT];
            uint8_t mode = stat & 0x03;
            uint8_t ly = io[memory::IO_LY];

            switch (mode) {
                case MODE_OAM:
                    if (ppu.scanlineCycles >= CYCLES_OAM) {
                        ppu.scanlineCycles -= CYCLES_OAM;
                        io[memory::IO_STAT] = (stat & 0xFC) | MODE_DRAWING;
                    }
                    break;

                case MODE_DRAWING:
                    if (ppu.scanlineCycles >= CYCLES_DRAWING) {
                        ppu.scanlineCycles -= CYCLES_DRAWING;
                        renderScanline(state);
                        io[memory::IO_STAT] = (stat & 0xFC) | MODE_HBLANK;
                        if (stat & 0x08) {
                            io[memory::IO_IF] |= 0x02;
                        }
                    }
                    break;

                case MODE_HBLANK:
                    if (ppu.scanlineCycles >= CYCLES_HBLANK) {
                        ppu.scanlineCycles -= CYCLES_HBLANK;
                        io[memory::IO_LY]++;
                        ly = io[memory::IO_LY];

                        if (ly >= SCANLINES_VISIBLE) {
                            io[memory::IO_STAT] = (stat & 0xFC) | MODE_VBLANK;
                            io[memory::IO_IF] |= 0x01;
                            if (stat & 0x10) {
                                io[memory::IO_IF] |= 0x02;
                            }
                            ppu.frameReady = true;
                        } else {
                            io[memory::IO_STAT] = (stat & 0xFC) | MODE_OAM;
                            if (stat & 0x20) {
                                io[memory::IO_IF] |= 0x02;
                            }
                        }
                        checkLYC(state);
                    }
                    break;

                case MODE_VBLANK:
                    if (ppu.scanlineCycles >= CYCLES_SCANLINE) {
                        ppu.scanlineCycles -= CYCLES_SCANLINE;
                        io[memory::IO_LY]++;
                        ly = io[memory::IO_LY];

                        if (ly >= SCANLINES_TOTAL) {
                            io[memory::IO_LY] = 0;
                            io[memory::IO_STAT] = (stat & 0xFC) | MODE_OAM;
                            if (stat & 0x20) {
                                io[memory::IO_IF] |= 0x02;
                            }
                        }
                        checkLYC(state);
                    }
                    break;
            }
        }

        void checkLYC(GBState& state) {
            auto& io = state.memory.io;
            uint8_t stat = io[memory::IO_STAT];

            if (io[memory::IO_LY] == io[memory::IO_LYC]) {
                io[memory::IO_STAT] |= 0x04;
                if (stat & 0x40) {
                    io[memory::IO_IF] |= 0x02;
                }
            } else {
                io[memory::IO_STAT] &= ~0x04;
            }
        }

        void renderScanline(GBState& state) {
            auto& io = state.memory.io;
            uint8_t lcdc = io[memory::IO_LCDC];

            if (lcdc & 0x01) renderBackground(state);
            if (lcdc & 0x20) renderWindow(state);
            if (lcdc & 0x02) renderSprites(state);
        }

        void renderBackground(GBState& state) {
            auto& ppu = state.ppu;
            auto& mem = state.memory;
            auto& io = mem.io;

            uint8_t lcdc = io[memory::IO_LCDC];
            uint8_t ly = io[memory::IO_LY];
            uint8_t scy = io[memory::IO_SCY];
            uint8_t scx = io[memory::IO_SCX];
            uint8_t bgp = io[memory::IO_BGP];

            uint16_t tileMap = (lcdc & 0x08) ? 0x1C00 : 0x1800;
            uint16_t tileData = (lcdc & 0x10) ? 0x0000 : 0x0800;
            bool signedTiles = !(lcdc & 0x10);

            uint8_t y = ly + scy;
            uint8_t tileY = y / 8;
            uint8_t pixelY = y % 8;

            for (int screenX = 0; screenX < SCREEN_WIDTH; screenX++) {
                uint8_t x = screenX + scx;
                uint8_t tileX = x / 8;
                uint8_t pixelX = x % 8;

                uint16_t mapAddr = tileMap + (tileY * 32) + tileX;
                uint8_t tileNum = mem.vram[mapAddr];

                uint16_t tileAddr;
                if (signedTiles) {
                    tileAddr = tileData + ((int8_t)tileNum + 128) * 16;
                } else {
                    tileAddr = tileData + tileNum * 16;
                }

                tileAddr += pixelY * 2;

                uint8_t lo = mem.vram[tileAddr];
                uint8_t hi = mem.vram[tileAddr + 1];

                uint8_t bit = 7 - pixelX;
                uint8_t colorNum = ((hi >> bit) & 1) << 1 | ((lo >> bit) & 1);
                uint8_t color = getColor(bgp, colorNum);

                ppu.framebuffer[ly * SCREEN_WIDTH + screenX] = color;
            }
        }

        void renderWindow(GBState& state) {
            auto& ppu = state.ppu;
            auto& mem = state.memory;
            auto& io = mem.io;

            uint8_t lcdc = io[memory::IO_LCDC];
            uint8_t ly = io[memory::IO_LY];
            uint8_t wy = io[memory::IO_WY];
            uint8_t wx = io[memory::IO_WX];
            uint8_t bgp = io[memory::IO_BGP];

            if (ly < wy) return;
            if (wx > 166) return;

            uint16_t tileMap = (lcdc & 0x40) ? 0x1C00 : 0x1800;
            uint16_t tileData = (lcdc & 0x10) ? 0x0000 : 0x0800;
            bool signedTiles = !(lcdc & 0x10);

            uint8_t y = ly - wy;
            uint8_t tileY = y / 8;
            uint8_t pixelY = y % 8;

            for (int screenX = 0; screenX < SCREEN_WIDTH; screenX++) {
                int windowX = screenX - (wx - 7);
                if (windowX < 0) continue;

                uint8_t tileX = windowX / 8;
                uint8_t pixelX = windowX % 8;

                uint16_t mapAddr = tileMap + (tileY * 32) + tileX;
                uint8_t tileNum = mem.vram[mapAddr];

                uint16_t tileAddr;
                if (signedTiles) {
                    tileAddr = tileData + ((int8_t)tileNum + 128) * 16;
                } else {
                    tileAddr = tileData + tileNum * 16;
                }

                tileAddr += pixelY * 2;

                uint8_t lo = mem.vram[tileAddr];
                uint8_t hi = mem.vram[tileAddr + 1];

                uint8_t bit = 7 - pixelX;
                uint8_t colorNum = ((hi >> bit) & 1) << 1 | ((lo >> bit) & 1);
                uint8_t color = getColor(bgp, colorNum);

                ppu.framebuffer[ly * SCREEN_WIDTH + screenX] = color;
            }
        }

        void renderSprites(GBState& state) {
            auto& ppu = state.ppu;
            auto& mem = state.memory;
            auto& io = mem.io;

            uint8_t lcdc = io[memory::IO_LCDC];
            uint8_t ly = io[memory::IO_LY];

            int spriteHeight = (lcdc & 0x04) ? 16 : 8;

            for (int i = 39; i >= 0; i--) {
                uint8_t y = mem.oam[i * 4] - 16;
                uint8_t x = mem.oam[i * 4 + 1] - 8;
                uint8_t tileNum = mem.oam[i * 4 + 2];
                uint8_t flags = mem.oam[i * 4 + 3];

                if (ly < y || ly >= y + spriteHeight) continue;
                if (x >= SCREEN_WIDTH) continue;

                bool flipX = flags & 0x20;
                bool flipY = flags & 0x40;
                bool priority = flags & 0x80;
                uint8_t palette = (flags & 0x10) ? io[memory::IO_OBP1] : io[memory::IO_OBP0];

                uint8_t tileY = ly - y;
                if (flipY) tileY = spriteHeight - 1 - tileY;

                if (spriteHeight == 16) tileNum &= 0xFE;

                uint16_t tileAddr = tileNum * 16 + tileY * 2;
                uint8_t lo = mem.vram[tileAddr];
                uint8_t hi = mem.vram[tileAddr + 1];

                for (int px = 0; px < 8; px++) {
                    int screenX = x + px;
                    if (screenX < 0 || screenX >= SCREEN_WIDTH) continue;

                    uint8_t bit = flipX ? px : (7 - px);
                    uint8_t colorNum = ((hi >> bit) & 1) << 1 | ((lo >> bit) & 1);

                    if (colorNum == 0) continue;

                    if (priority && ppu.framebuffer[ly * SCREEN_WIDTH + screenX] != 0) continue;

                    uint8_t color = getColor(palette, colorNum);
                    ppu.framebuffer[ly * SCREEN_WIDTH + screenX] = color;
                }
            }
        }

        uint8_t getColor(uint8_t palette, uint8_t colorNum) {
            return (palette >> (colorNum * 2)) & 0x03;
        }

    }
}
