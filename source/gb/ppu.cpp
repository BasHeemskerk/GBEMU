#include "included/ppu.hpp"
#include "included/memory.hpp"
#include <3ds.h>
#include <cstring>

namespace gb {
    namespace ppu {

        uint8_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
        bool frameReady;
        int scanlineCycles;

        void initialize() {
            memset(framebuffer, 0, sizeof(framebuffer));
            frameReady = false;
            scanlineCycles = 0;
        }

        void tick(int cycles) {
            if (!(memory::io[memory::IO_LCDC] & 0x80)) {
                return;
            }

            scanlineCycles += cycles;

            uint8_t stat = memory::io[memory::IO_STAT];
            uint8_t mode = stat & 0x03;
            uint8_t ly = memory::io[memory::IO_LY];

            switch (mode) {
                case MODE_OAM:
                    if (scanlineCycles >= CYCLES_OAM) {
                        // FIX: was (stat = 0xFC), should be (stat & 0xFC)
                        memory::io[memory::IO_STAT] = (stat & 0xFC) | MODE_DRAWING;
                    }
                    break;

                case MODE_DRAWING:
                    if (scanlineCycles >= CYCLES_OAM + CYCLES_DRAWING) {
                        renderScanline();
                        memory::io[memory::IO_STAT] = (stat & 0xFC) | MODE_HBLANK;

                        if (stat & 0x08) {
                            memory::io[memory::IO_IF] |= 0x02;
                        }
                    }
                    break;

                case MODE_HBLANK:
                    if (scanlineCycles >= CYCLES_SCANLINE) {
                        scanlineCycles -= CYCLES_SCANLINE;
                        ly++;
                        memory::io[memory::IO_LY] = ly;

                        if (ly == SCANLINES_VISIBLE) {
                            memory::io[memory::IO_STAT] = (stat & 0xFC) | MODE_VBLANK;
                            frameReady = true;
                            memory::io[memory::IO_IF] |= 0x01;

                            if (stat & 0x10) {
                                memory::io[memory::IO_IF] |= 0x02;
                            }
                        } else {
                            memory::io[memory::IO_STAT] = (stat & 0xFC) | MODE_OAM;

                            if (stat & 0x20) {
                                memory::io[memory::IO_IF] |= 0x02;
                            }
                        }

                        checkLYC();
                    }
                    break;

                case MODE_VBLANK:
                    if (scanlineCycles >= CYCLES_SCANLINE) {
                        scanlineCycles -= CYCLES_SCANLINE;
                        ly++;

                        if (ly >= SCANLINES_TOTAL) {
                            ly = 0;
                            memory::io[memory::IO_STAT] = (stat & 0xFC) | MODE_OAM;

                            if (stat & 0x20) {
                                memory::io[memory::IO_IF] |= 0x02;
                            }
                        }

                        memory::io[memory::IO_LY] = ly;
                        checkLYC();
                    }
                    break;
            }
        }

        void checkLYC() {
            uint8_t ly = memory::io[memory::IO_LY];
            uint8_t lyc = memory::io[memory::IO_LYC];
            uint8_t stat = memory::io[memory::IO_STAT];

            if (ly == lyc) {
                memory::io[memory::IO_STAT] = stat | 0x04;

                if (stat & 0x40) {
                    memory::io[memory::IO_IF] |= 0x02;
                }
            } else {
                memory::io[memory::IO_STAT] = stat & ~0x04;
            }
        }

        void renderScanline() {
            uint8_t lcdc = memory::io[memory::IO_LCDC];

            if (lcdc & 0x01) {
                renderBackground();
            }

            // FIX: was 0x29, should be 0x20
            if (lcdc & 0x20) {
                renderWindow();
            }

            if (lcdc & 0x02) {
                renderSprites();
            }
        }

        void renderBackground() {
            uint8_t lcdc = memory::io[memory::IO_LCDC];
            uint8_t scx = memory::io[memory::IO_SCX];
            uint8_t scy = memory::io[memory::IO_SCY];
            uint8_t ly = memory::io[memory::IO_LY];
            uint8_t bgp = memory::io[memory::IO_BGP];

            uint16_t tileMapBase = (lcdc & 0x08) ? 0x1C00 : 0x1800;
            bool unsignedTiles = lcdc & 0x10;
            uint16_t tileDataBase = unsignedTiles ? 0x0000 : 0x0800;

            uint8_t y = ly + scy;
            uint8_t tileRow = y / 8;
            uint8_t pixelRow = y % 8;

            for (int screenX = 0; screenX < SCREEN_WIDTH; screenX++) {
                uint8_t x = screenX + scx;
                uint8_t tileCol = x / 8;
                uint8_t pixelCol = x % 8;

                uint16_t tileMapAddr = tileMapBase + (tileRow * 32) + tileCol;
                uint8_t tileIndex = memory::vram[tileMapAddr];

                uint16_t tileDataAddr;
                if (unsignedTiles) {
                    tileDataAddr = tileDataBase + (tileIndex * 16);
                } else {
                    int8_t signedIndex = (int8_t)tileIndex;
                    tileDataAddr = tileDataBase + ((signedIndex + 128) * 16);
                }

                uint8_t byte1 = memory::vram[tileDataAddr + (pixelRow * 2)];
                uint8_t byte2 = memory::vram[tileDataAddr + (pixelRow * 2) + 1];

                uint8_t bit = 7 - pixelCol;
                uint8_t colorNum = ((byte2 >> bit) & 1) << 1 | ((byte1 >> bit) & 1);

                uint8_t color = getColor(bgp, colorNum);
                framebuffer[ly * SCREEN_WIDTH + screenX] = color;
            }
        }

        void renderWindow() {
            uint8_t lcdc = memory::io[memory::IO_LCDC];
            uint8_t wx = memory::io[memory::IO_WX];
            uint8_t wy = memory::io[memory::IO_WY];
            uint8_t ly = memory::io[memory::IO_LY];
            uint8_t bgp = memory::io[memory::IO_BGP];

            int windowX = wx - 7;

            if (ly < wy || windowX >= SCREEN_WIDTH) {
                return;
            }

            uint16_t tileMapBase = (lcdc & 0x40) ? 0x1C00 : 0x1800;
            bool unsignedTiles = lcdc & 0x10;
            uint16_t tileDataBase = unsignedTiles ? 0x0000 : 0x0800;

            uint8_t y = ly - wy;
            uint8_t tileRow = y / 8;
            uint8_t pixelRow = y % 8;

            for (int screenX = (windowX < 0 ? 0 : windowX); screenX < SCREEN_WIDTH; screenX++) {
                int x = screenX - windowX;
                uint8_t tileCol = x / 8;
                uint8_t pixelCol = x % 8;

                uint16_t tileMapAddr = tileMapBase + (tileRow * 32) + tileCol;
                uint8_t tileIndex = memory::vram[tileMapAddr];

                uint16_t tileDataAddr;
                if (unsignedTiles) {
                    tileDataAddr = tileDataBase + (tileIndex * 16);
                } else {
                    int8_t signedIndex = (int8_t)tileIndex;
                    tileDataAddr = tileDataBase + ((signedIndex + 128) * 16);
                }

                uint8_t byte1 = memory::vram[tileDataAddr + (pixelRow * 2)];
                uint8_t byte2 = memory::vram[tileDataAddr + (pixelRow * 2) + 1];

                uint8_t bit = 7 - pixelCol;
                uint8_t colorNum = ((byte2 >> bit) & 1) << 1 | ((byte1 >> bit) & 1);

                uint8_t color = getColor(bgp, colorNum);
                framebuffer[ly * SCREEN_WIDTH + screenX] = color;
            }
        }

        void renderSprites() {
            uint8_t lcdc = memory::io[memory::IO_LCDC];
            uint8_t ly = memory::io[memory::IO_LY];

            int spriteHeight = (lcdc & 0x04) ? 16 : 8;
            int spriteCount = 0;

            for (int i = 0; i < 40 && spriteCount < 10; i++) {
                // FIX: use int for y to handle negative values properly
                int spriteY = memory::oam[i * 4] - 16;
                int spriteX = memory::oam[i * 4 + 1] - 8;
                uint8_t tileIndex = memory::oam[i * 4 + 2];
                uint8_t attrs = memory::oam[i * 4 + 3];

                // check if sprite is on this scanline
                if ((int)ly < spriteY || (int)ly >= spriteY + spriteHeight) {
                    continue;
                }

                spriteCount++;

                bool flipX = attrs & 0x20;
                bool flipY = attrs & 0x40;
                bool priority = attrs & 0x80;
                uint8_t palette = (attrs & 0x10) ? memory::io[memory::IO_OBP1] : memory::io[memory::IO_OBP0];

                int row = ly - spriteY;
                if (flipY) {
                    row = (spriteHeight - 1) - row;
                }

                if (spriteHeight == 16) {
                    tileIndex &= 0xFE;
                }

                uint16_t tileDataAddr = (tileIndex * 16) + (row * 2);
                uint8_t byte1 = memory::vram[tileDataAddr];
                uint8_t byte2 = memory::vram[tileDataAddr + 1];

                for (int col = 0; col < 8; col++) {
                    int screenX = spriteX + col;

                    if (screenX < 0 || screenX >= SCREEN_WIDTH) {
                        continue;
                    }

                    int bit = flipX ? col : (7 - col);
                    uint8_t colorNum = ((byte2 >> bit) & 1) << 1 | ((byte1 >> bit) & 1);

                    if (colorNum == 0) {
                        continue;
                    }

                    if (priority && framebuffer[ly * SCREEN_WIDTH + screenX] != 0) {
                        continue;
                    }

                    uint8_t color = getColor(palette, colorNum);
                    framebuffer[ly * SCREEN_WIDTH + screenX] = color;
                }
            }
        }

        uint8_t getColor(uint8_t palette, uint8_t colorNum) {
            return (palette >> (colorNum * 2)) & 0x03;
        }

    }  // namespace ppu
}  // namespace gb