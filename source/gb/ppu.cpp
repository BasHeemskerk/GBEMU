#include "included/ppu.hpp"
#include "included/state.hpp"
#include "included/memory.hpp"
#include <cstring>

namespace gb {
    namespace ppu {

        //the lookup table - decodes tile bytes to pixel colors
        // tileLUT[high_byte][low_byte][pixel_index] = color_index (0-3)
        uint8_t tileLUT[256][256][8];

        void buildLUT(){
            //for every possible combo of high and low bytes
            for (int high = 0; high < 256; high++){
                for (int low = 0; low < 256; low++){
                    //decode all 8 pixels
                    for (int px = 0; px < 8; px++){
                        int bit = 7 - px;
                        uint8_t low_bit = (low >> bit) & 1;
                        uint8_t high_bit = (high >> bit) & 1;
                        tileLUT[high][low][px] = (high_bit << 1) | low_bit;
                    }
                }
            }
        }

        void initialize(GBState& state) {
            auto& ppu = state.ppu;

            memset(ppu.framebuffer, 0, sizeof(ppu.framebuffer));
            ppu.frameReady = false;
            ppu.scanlineCycles = 0;

            //build the lookup table from here
            static bool lutBuilt = false;
            if (!lutBuilt){
                buildLUT();
                lutBuilt = true;
            }
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
                        io[memory::IO_STAT] = (stat & 0xFC) | MODE_DRAWING;
                    }
                    break;

                case MODE_DRAWING:
                    if (ppu.scanlineCycles >= CYCLES_OAM + CYCLES_DRAWING) {
                        renderScanline(state);
                        io[memory::IO_STAT] = (stat & 0xFC) | MODE_HBLANK;
                        if (stat & 0x08) {
                            io[memory::IO_IF] |= 0x02;
                        }
                    }
                    break;

                case MODE_HBLANK:
                    if (ppu.scanlineCycles >= CYCLES_SCANLINE) {
                        ppu.scanlineCycles -= CYCLES_SCANLINE;
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
            uint8_t tileY = y >> 3; //same as y / 8 but way faster
            uint8_t pixelY = y & 0x07; //same as y % 8 but also faster

            //get pointer to this scanline in framebuffer
            uint8_t* fb = &ppu.framebuffer[ly * SCREEN_WIDTH];

            //precompute the 4 possible colors from palette
            uint8_t colors[4];
            colors[0] = (bgp >> 0) & 0x03;
            colors[1] = (bgp >> 2) & 0x03;
            colors[2] = (bgp >> 4) & 0x03;
            colors[3] = (bgp >> 6) & 0x03;

            //calculate base map address for this tile row
            uint16_t mapRowBase = tileMap + (tileY << 5); //tileY * 32

            int screenX = 0;
            uint8_t x = scx;

            while (screenX < SCREEN_WIDTH) {
                //uint8_t x = screenX + scx;
                uint8_t tileX = x >> 3; //same as x / 8
                uint8_t startPixel = x & 0x07; //same as x % 8

                //fetch tile number from map
                uint8_t tileNum = mem.vram[mapRowBase + tileX];

                //calculate tile data address
                uint16_t tileAddr;
                if (signedTiles){
                    tileAddr = tileData + (((int8_t)tileNum + 128) << 4);
                } else {
                    tileAddr = tileData + (tileNum << 4);
                }
                tileAddr += pixelY << 1; //add row offset (2bytes per row)

                //fetch the two bytes for this tile row
                uint8_t low = mem.vram[tileAddr];
                uint8_t high = mem.vram[tileAddr + 1];

                //use the lookup table to get all 8 pixel colors
                uint8_t* tilePixels = tileLUT[high][low];

                //how many pixels from this tile do we need?
                int endPixel = 8;
                int remaining = SCREEN_WIDTH - screenX;
                if (remaining < (8 - startPixel)){
                    endPixel = startPixel + remaining;
                }

                //copy pixel from tile to framebuffer
                for (int px = startPixel; px < endPixel; px++){
                    fb[screenX++] = colors[tilePixels[px]];
                }

                //move to next tile
                x = (x + (endPixel - startPixel)) & 0xFF;
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

            //window not visible on this scanline
            if (ly < wy) return;
            if (wx > 166) return;

            //get tile map and data addresses from lcdc
            uint16_t tileMap = (lcdc & 0x40) ? 0x1C00 : 0x1800;
            uint16_t tileData = (lcdc & 0x10) ? 0x0000 : 0x0800;
            bool signedTiles = !(lcdc & 0x10);

            //calculate which row of tiles we are on
            uint8_t y = ly - wy;
            uint8_t tileY = y >> 3; //same as y / 8
            uint8_t pixelY = y & 0x07; //same as y % 7

            //pointer to current scanline in framebuffer
            uint8_t* fb = &ppu.framebuffer[ly * SCREEN_WIDTH];

            //precompute palette colors
            uint8_t colors[4];
            colors[0] = (bgp >> 0) & 0x03;
            colors[1] = (bgp >> 2) & 0x03;
            colors[2] = (bgp >> 4) & 0x03;
            colors[3] = (bgp >> 6) & 0x03;

            //base address for this row of tiles in the map
            uint16_t mapRowBase = tileMap + (tileY << 5);

            //window starts at wx - 7
            int windowStartX = wx - 7;
            if (windowStartX < 0){
                windowStartX = 0;
            }

            int windowX = 0;
            for (int screenX = windowStartX; screenX < SCREEN_WIDTH;){
                //get tile coordinates
                uint8_t tileX = windowX >> 3;
                uint8_t startPixel = windowX & 0x07;

                //fetch tile number from map
                uint8_t tileNum = mem.vram[mapRowBase + tileX];

                //calculate tile data address
                uint16_t tileAddr;
                if (signedTiles){
                    tileAddr = tileData + (((int8_t)tileNum + 128) << 4);
                } else {
                    tileAddr = tileData + (tileNum << 4);
                }
                tileAddr += pixelY << 1;

                //fetch tile row bytes
                uint8_t low = mem.vram[tileAddr];
                uint8_t high = mem.vram[tileAddr + 1];

                //decode tile row using lookup table
                uint8_t* tilePixels = tileLUT[high][low];

                //calculate how many pixels to draw from this tile
                int endPixel = 8;
                int remaining = SCREEN_WIDTH - screenX;
                if (remaining < (8 - startPixel)){
                    endPixel = startPixel + remaining;
                }

                //copy pixels from tile to framebuffer
                for (int px = startPixel; px < endPixel; px++){
                    fb[screenX++] = colors[tilePixels[px]];
                }

                //advance window position
                windowX += (endPixel - startPixel);
            }
        }

        void renderSprites(GBState& state) {
            auto& ppu = state.ppu;
            auto& mem = state.memory;
            auto& io = mem.io;

            uint8_t lcdc = io[memory::IO_LCDC];
            uint8_t ly = io[memory::IO_LY];

            //sprites can be 8 or 16 pixels tall
            int spriteHeight = (lcdc & 0x04) ? 16 : 8;

            //pointer to current scanline in framebuffer
            uint8_t* fb = &ppu.framebuffer[ly * SCREEN_WIDTH];

            //loop through all 40 sprites (reverse order for priority)
            for (int i = 39; i >= 0; i--){
                //get sprite data from oam
                uint8_t* sprite = &mem.oam[i << 2];

                int y = sprite[0] - 16;
                int x = sprite[1] - 8;
                uint8_t tileNum = sprite[2];
                uint8_t flags = sprite[3];

                //skip if sprite is not on this scanline
                if (ly < y || ly >= y + spriteHeight){
                    continue;
                }
                if (x <= -8 || x >= SCREEN_WIDTH){
                    continue;
                }

                //read sprite flags
                bool flipX = flags & 0x20;
                bool flipY = flags & 0x40;
                bool priority = flags & 0x80;
                uint8_t palette = (flags & 0x10) ? io[memory::IO_OBP1] : io[memory::IO_OBP0];

                //precompute palette colors
                uint8_t colors[4];
                colors[0] = (palette >> 0) & 0x03;
                colors[1] = (palette >> 2) & 0x03;
                colors[2] = (palette >> 4) & 0x03;
                colors[3] = (palette >> 6) & 0x03;

                //calculate which row of the sprite we are drawing
                uint8_t tileY = ly - y;
                if (flipY){
                    tileY = spriteHeight - 1 - tileY;
                }

                //for 8x16 sprites, ignore lowest bit of the tile number
                if (spriteHeight == 16){
                    tileNum &= 0xFE;
                }

                //fetch tile row bytes
                uint16_t tileAddr = (tileNum << 4) + (tileY << 1);
                uint8_t low = mem.vram[tileAddr];
                uint8_t high = mem.vram[tileAddr + 1];

                //decode tile row using the lookup table
                uint8_t* tilePixels = tileLUT[high][low];

                //draw 8 pixels of the sprite
                for (int px = 0; px < 8; px++){
                    int screenX = x + px;
                    
                    //skip if offscreen
                    if (screenX < 0 || screenX >= SCREEN_WIDTH){
                        continue;
                    }

                    //handle horizontal flip
                    int tilePx = flipX ? (7 - px) : px;
                    uint8_t colorNum = tilePixels[tilePx];

                    //color 0 is transparant
                    if (colorNum == 0){
                        continue;
                    }

                    //priority flag; sprite behind background
                    if (priority && fb[screenX] != 0){
                        continue;
                    }

                    //write pixel to framebuffer
                    fb[screenX] = colors[colorNum];
                }
            }
        }

        uint8_t getColor(uint8_t palette, uint8_t colorNum) {
            return (palette >> (colorNum * 2)) & 0x03;
        }

    }
}
