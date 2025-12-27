#ifndef GB_TIMER_HPP
#define GB_TIMER_HPP

#include <cstdint>

namespace gb{
    namespace timer{
        // internal counter for DIV register
        // DIV increments every 256 cycles (16384 Hz)
        extern uint16_t divCounter;

        //internal counter for tima
        extern uint16_t timaCounter;

        //functions
        void initialize();
        void tick(int cycles);
    }
}

#endif