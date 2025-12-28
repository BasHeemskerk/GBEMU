#ifndef GB_PPU_HPP
#define GB_PPU_HPP

#include <cstdint>

namespace gb {

    struct GBState;

    namespace ppu {

        constexpr int SCREEN_WIDTH = 160;
        constexpr int SCREEN_HEIGHT = 144;

        constexpr uint8_t MODE_HBLANK = 0;
        constexpr uint8_t MODE_VBLANK = 1;
        constexpr uint8_t MODE_OAM = 2;
        constexpr uint8_t MODE_DRAWING = 3;

        constexpr int CYCLES_OAM = 80;
        constexpr int CYCLES_DRAWING = 172;
        constexpr int CYCLES_HBLANK = 204;
        constexpr int CYCLES_SCANLINE = 456;
        constexpr int SCANLINES_VISIBLE = 144;
        constexpr int SCANLINES_TOTAL = 154;

        //new: lookup table for decoding tile pixels
        //given 2 bytes, (low and high) from tile data, gives you all 8 color indices
        //index: [high][low][pixel] -> color index (0 - 3)
        extern uint8_t tileLUT[256][256][8];

        void buildLUT(); //call to build lookup table

        void initialize(GBState& state);
        void tick(GBState& state, int cycles);

        void renderScanline(GBState& state);
        void renderBackground(GBState& state);
        void renderWindow(GBState& state);
        void renderSprites(GBState& state);
        void checkLYC(GBState& state);
        uint8_t getColor(uint8_t palette, uint8_t colorNum);

    }
}

#endif
