#ifndef GB_TIMER_HPP
#define GB_TIMER_HPP

#include <cstdint>

namespace gb{

    struct GBState;

    namespace timer{
        //timer registers in IO
        constexpr uint8_t REG_DIV = 0x04; //divider
        constexpr uint8_t REG_TIMA = 0x05; //timer counter
        constexpr uint8_t REG_TMA = 0x06; //timer module
        constexpr uint8_t REG_TAC = 0x07; //timer control

        //functions
        void initialize(GBState& state);
        void tick(GBState& state, int cycles);
    }
}

#endif