#include "included/joypad.hpp"
#include "included/state.hpp"

namespace gb {
    namespace joypad {

        void initialize(GBState& state) {
            auto& jp = state.joypad;

            jp.buttonA = false;
            jp.buttonB = false;
            jp.buttonStart = false;
            jp.buttonSelect = false;
            jp.dpadUp = false;
            jp.dpadDown = false;
            jp.dpadLeft = false;
            jp.dpadRight = false;
            jp.selectButtons = false;
            jp.selectDpad = false;
        }

        uint8_t read(GBState& state) {
            auto& jp = state.joypad;
            uint8_t result = 0x0F;

            if (jp.selectButtons) {
                if (jp.buttonA)      result &= ~0x01;
                if (jp.buttonB)      result &= ~0x02;
                if (jp.buttonSelect) result &= ~0x04;
                if (jp.buttonStart)  result &= ~0x08;
            }

            if (jp.selectDpad) {
                if (jp.dpadRight) result &= ~0x01;
                if (jp.dpadLeft)  result &= ~0x02;
                if (jp.dpadUp)    result &= ~0x04;
                if (jp.dpadDown)  result &= ~0x08;
            }

            if (!jp.selectButtons) result |= 0x20;
            if (!jp.selectDpad)    result |= 0x10;

            return result;
        }

        void write(GBState& state, uint8_t value) {
            auto& jp = state.joypad;

            jp.selectButtons = !(value & 0x20);
            jp.selectDpad = !(value & 0x10);
        }

        void setButton(GBState& state, int button, bool pressed) {
            auto& jp = state.joypad;

            switch (button) {
                case BTN_A:      jp.buttonA = pressed; break;
                case BTN_B:      jp.buttonB = pressed; break;
                case BTN_SELECT: jp.buttonSelect = pressed; break;
                case BTN_START:  jp.buttonStart = pressed; break;
                case BTN_RIGHT:  jp.dpadRight = pressed; break;
                case BTN_LEFT:   jp.dpadLeft = pressed; break;
                case BTN_UP:     jp.dpadUp = pressed; break;
                case BTN_DOWN:   jp.dpadDown = pressed; break;
            }
        }

    }
}
