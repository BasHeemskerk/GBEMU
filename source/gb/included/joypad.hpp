#ifndef GB_JOYPAD_HPP
#define GB_JOYPAD_HPP

#include <cstdint>

namespace gb{

    struct GBState;

    namespace joypad{
        void initialize(GBState& state);
        uint8_t read(GBState& state);
        void write(GBState& state, uint8_t value);
    }
}

#endif