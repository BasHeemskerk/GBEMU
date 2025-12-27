#ifndef GB_CPU_HPP
#define GB_CPU_HPP

#include <cstdint>

namespace gb {

    struct GBState;

    namespace cpu {

        // Flag bit positions in F register
        constexpr uint8_t FLAG_Z = 0x80;
        constexpr uint8_t FLAG_N = 0x40;
        constexpr uint8_t FLAG_H = 0x20;
        constexpr uint8_t FLAG_C = 0x10;

        // Functions
        void initialize(GBState& state);
        int step(GBState& state);
        int executeCB(GBState& state);

    }
}

#endif
