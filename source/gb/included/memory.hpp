#ifndef GB_MEMORY_HPP
#define GB_MEMORY_HPP

#include <cstdint>

namespace gb{
    namespace memory{
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

        extern uint8_t vram[0x2000]; //video ram - 8kb
        extern uint8_t wram[0x2000]; //work ram - 8b
        extern uint8_t hram[0x7F]; //high ram, often used for interrupt handlers - 127 bytes
        extern uint8_t oam[0xA0]; //object attribute memory - 160 bytes
        extern uint8_t io[0x80]; //IO registers - 128 bytes
        extern uint8_t ie; //interrupt enable register
        
        //important IO register addresses
        constexpr uint16_t IO_JOYP = 0x00; //joypad
        constexpr uint16_t IO_SB = 0x01; //serial data
        constexpr uint16_t IO_SC = 0x02; //serial control
        constexpr uint16_t IO_DIV = 0x04; //divider register
        constexpr uint16_t IO_TIMA = 0x05; //timer counter
        constexpr uint16_t IO_TMA = 0x06; //timer module
        constexpr uint16_t IO_TAC = 0x07; //timer control
        constexpr uint16_t IO_IF = 0x0F; //interrupt flag

        //audio registers
        constexpr uint16_t IO_NR10 = 0x10; //sound channel 1
        constexpr uint16_t IO_NR11 = 0x11; //sound channel 1 length / duty
        constexpr uint16_t IO_NR12 = 0x12; //sound channel 1 envelope
        constexpr uint16_t IO_NR13 = 0x13; //sound channel 1 frequency low
        constexpr uint16_t IO_NR14 = 0x14; //sound channel 1 frequency high

        constexpr uint16_t IO_LCDC  = 0x40;  // 0xFF40 - LCD Control
        constexpr uint16_t IO_STAT  = 0x41;  // 0xFF41 - LCD Status
        constexpr uint16_t IO_SCY   = 0x42;  // 0xFF42 - Scroll Y
        constexpr uint16_t IO_SCX   = 0x43;  // 0xFF43 - Scroll X
        constexpr uint16_t IO_LY    = 0x44;  // 0xFF44 - Current scanline
        constexpr uint16_t IO_LYC   = 0x45;  // 0xFF45 - Scanline compare
        constexpr uint16_t IO_DMA   = 0x46;  // 0xFF46 - DMA transfer
        constexpr uint16_t IO_BGP   = 0x47;  // 0xFF47 - Background palette
        constexpr uint16_t IO_OBP0  = 0x48;  // 0xFF48 - Object palette 0
        constexpr uint16_t IO_OBP1  = 0x49;  // 0xFF49 - Object palette 1
        constexpr uint16_t IO_WY    = 0x4A;  // 0xFF4A - Window Y
        constexpr uint16_t IO_WX    = 0x4B;  // 0xFF4B - Window X

        //functions
        void initialize();

        //read / write functions, route to correct memory region based on address
        uint8_t read(uint16_t address);
        void write(uint16_t address, uint8_t value);

        //direct access to IO for other components
        uint8_t readIO(uint8_t reg);
        void writeIO(uint8_t reg, uint8_t value);

    }
}

#endif