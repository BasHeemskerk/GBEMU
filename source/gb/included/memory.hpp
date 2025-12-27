#ifndef GB_MEMORY_HPP
#define GB_MEMORY_HPP

#include <cstdint>

namespace gb {

    struct GBState;

    namespace memory {

        // IO register offsets
        constexpr uint8_t IO_JOYP = 0x00;
        constexpr uint8_t IO_DIV  = 0x04;
        constexpr uint8_t IO_TIMA = 0x05;
        constexpr uint8_t IO_TMA  = 0x06;
        constexpr uint8_t IO_TAC  = 0x07;
        constexpr uint8_t IO_IF   = 0x0F;
        constexpr uint8_t IO_LCDC = 0x40;
        constexpr uint8_t IO_STAT = 0x41;
        constexpr uint8_t IO_SCY  = 0x42;
        constexpr uint8_t IO_SCX  = 0x43;
        constexpr uint8_t IO_LY   = 0x44;
        constexpr uint8_t IO_LYC  = 0x45;
        constexpr uint8_t IO_DMA  = 0x46;
        constexpr uint8_t IO_BGP  = 0x47;
        constexpr uint8_t IO_OBP0 = 0x48;
        constexpr uint8_t IO_OBP1 = 0x49;
        constexpr uint8_t IO_WY   = 0x4A;
        constexpr uint8_t IO_WX   = 0x4B;

        void initialize(GBState& state);
        uint8_t read(GBState& state, uint16_t address);
        void write(GBState& state, uint16_t address, uint8_t value);
        void doDMA(GBState& state, uint8_t value);

    }
}

#endif
