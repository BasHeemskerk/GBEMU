#ifndef GB_CARTRIDGE_HPP
#define GB_CARTRIDGE_HPP

#include <cstdint>

namespace gb {

    struct GBState;

    namespace cartridge {

        void initialize(GBState& state);
        bool loadRom(GBState& state, const char* filePath);
        void cleanup(GBState& state);

        uint8_t read(GBState& state, uint16_t address);
        void write(GBState& state, uint16_t address, uint8_t value);

        uint8_t readRAM(GBState& state, uint16_t address);
        void writeRAM(GBState& state, uint16_t address, uint8_t value);

        const char* getTitle(GBState& state);

    }
}

#endif
