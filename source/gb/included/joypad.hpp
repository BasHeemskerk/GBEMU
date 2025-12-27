#ifndef GB_JOYPAD_HPP
#define GB_JOYPAD_HPP

#include <cstdint>

namespace gb{
    namespace joypad{
        // button states (0 = pressed, 1 = released)
        // Game Boy uses active-low logic
        extern bool buttonA;
        extern bool buttonB;
        extern bool buttonStart;
        extern bool buttonSelect;
        extern bool dpadUp;
        extern bool dpadDown;
        extern bool dpadLeft;
        extern bool dpadRight;

        //selection state from p1 register
        extern bool selectButtons;
        extern bool selectDpad;

        //functions
        void initialize();
        void update(); //called from main loop to read the 3ds buttons
        uint8_t read(); //returns p1 reg value
        void write(uint8_t value); //sets selection bits
    }
}

#endif