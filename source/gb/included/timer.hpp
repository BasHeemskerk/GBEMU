#ifndef GB_TIMER_HPP
#define GB_TIMER_HPP

#include <cstdint>

namespace gb {

    struct GBState;

    namespace timer {

        constexpr uint8_t REG_DIV = 0x04;
        constexpr uint8_t REG_TIMA = 0x05;
        constexpr uint8_t REG_TMA = 0x06;
        constexpr uint8_t REG_TAC = 0x07;

        void initialize(GBState& state);
        void tick(GBState& state, int cycles);

    }
}

#endif
