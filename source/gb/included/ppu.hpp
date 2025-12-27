#ifndef GB_PPU_HPP
#define GB_PPU_HPP

#include <cstdint>

namespace gb{
    namespace ppu{
        //screen dimensions
        constexpr int SCREEN_WIDTH = 160;
        constexpr int SCREEN_HEIGHT = 144;

        // framebuffer - stores final pixel colors (0-3 for each pixel)
        extern uint8_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT];

        //set when frame is ready to be displayed
        extern bool frameReady;

        //internal cycle counter for current scanline
        extern int scanlineCycles;

        // PPU modes (stored in STAT register bits 0-1)
        // Mode 0: HBlank (204 cycles) - CPU can access VRAM/OAM
        // Mode 1: VBlank (4560 cycles) - CPU can access VRAM/OAM
        // Mode 2: OAM scan (80 cycles) - CPU can access VRAM only
        // Mode 3: Drawing (172 cycles) - CPU can't access VRAM/OAM
        constexpr uint8_t MODE_HBLANK = 0;
        constexpr uint8_t MODE_VBLANK = 1;
        constexpr uint8_t MODE_OAM = 2;
        constexpr uint8_t MODE_DRAWING = 3;

        // timing constants
        constexpr int CYCLES_OAM = 80;
        constexpr int CYCLES_DRAWING = 172;
        constexpr int CYCLES_HBLANK = 204;
        constexpr int CYCLES_SCANLINE = 456;  // total per line
        constexpr int SCANLINES_VISIBLE = 144;
        constexpr int SCANLINES_TOTAL = 154;

        //functions
        void initialize();
        void tick(int cycles);
        void checkLYC();

        //internal rendering functions
        void renderScanline();
        void renderBackground();
        void renderWindow();
        void renderSprites();

        //helper to get colors from palette
        uint8_t getColor(uint8_t palette, uint8_t colorNum);
    }
}

#endif