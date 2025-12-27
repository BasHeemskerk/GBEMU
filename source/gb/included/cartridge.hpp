#ifndef GB_CARTRIDGE_HPP
#define GB_CARTRIDGE_HPP

#include <cstdint>

namespace gb{
    namespace cartridge{
        //mapper types
        enum class MapperType{
            NONE,
            MBC1,
            MBC3,
            MBC5
        };

        //rom and ram storage
        constexpr int MAX_ROM_SIZE = 8 * 1024 * 1024;
        constexpr int MAX_RAM_SIZE = 128 * 1024;

        extern uint8_t rom[MAX_ROM_SIZE];
        extern uint8_t ram[MAX_RAM_SIZE];
        extern int romSize;
        extern int ramSize;

        //mapper state
        extern MapperType mapper;
        extern int romBank;
        extern int ramBank;
        extern bool ramEnabled;
        extern int mbcMode;

        //game info
        extern char title[17];

        //functions
        void initialize();
        bool loadRom(const char* filePath);
        uint8_t read(uint16_t address);
        void write(uint16_t address, uint8_t value);
        uint8_t readRAM(uint16_t address);
        void writeRAM(uint16_t address, uint8_t value);
        const char* getTitle();
    }
}

#endif