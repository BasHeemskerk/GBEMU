#ifndef GB_MEMORY_HPP
#define GB_MEMORY_HPP

#include <cstdint>

namespace gb{

    struct GBState;

    namespace memory{

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

        // ===========================================
        // Game Boy Memory Map (64KB address space)
        // ===========================================
        // 0x0000 - 0x3FFF : ROM Bank 0 (16KB) - fixed, always present
        // 0x4000 - 0x7FFF : ROM Bank 1-N (16KB) - switchable via mapper
        // 0x8000 - 0x9FFF : Video RAM (8KB) - tile data and maps
        // 0xA000 - 0xBFFF : External RAM (8KB) - cartridge RAM, battery backed
        // 0xC000 - 0xDFFF : Work RAM (8KB) - general purpose
        // 0xE000 - 0xFDFF : Echo RAM - mirror of 0xC000-0xDDFF, don't use
        // 0xFE00 - 0xFE9F : OAM (160 bytes) - sprite attribute table
        // 0xFEA0 - 0xFEFF : Unusable - Nintendo says don't touch
        // 0xFF00 - 0xFF7F : I/O Registers - hardware control
        // 0xFF80 - 0xFFFE : High RAM (127 bytes) - fast RAM
        // 0xFFFF          : Interrupt Enable register
    }
}

#endif