#ifndef GB_JOYPAD_HPP
#define GB_JOYPAD_HPP

#include <cstdint>

namespace gb {

    struct GBState;

    namespace joypad {

        void initialize(GBState& state);
        uint8_t read(GBState& state);
        void write(GBState& state, uint8_t value);

        // Set button states from external input
        void setButton(GBState& state, int button, bool pressed);

        // Button indices
        constexpr int BTN_A = 0;
        constexpr int BTN_B = 1;
        constexpr int BTN_SELECT = 2;
        constexpr int BTN_START = 3;
        constexpr int BTN_RIGHT = 4;
        constexpr int BTN_LEFT = 5;
        constexpr int BTN_UP = 6;
        constexpr int BTN_DOWN = 7;

    }
}

#endif
