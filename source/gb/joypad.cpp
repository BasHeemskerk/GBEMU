#include "included/joypad.hpp"
#include "included/memory.hpp"
#include <3ds.h>

namespace gb{
    namespace joypad{
        bool buttonA;
        bool buttonB;
        bool buttonStart;
        bool buttonSelect;
        bool dpadUp;
        bool dpadDown;
        bool dpadLeft;
        bool dpadRight;

        bool selectButtons;
        bool selectDpad;

        void initialize(){
            //all buttons released, active low, so true = released
            buttonA = true;
            buttonB = true;
            buttonStart = true;
            buttonSelect = true;
            dpadUp = true;
            dpadDown = true;
            dpadLeft = true;
            dpadRight = true;

            selectButtons = false;
            selectDpad = false;
        }

        void update(){
            hidScanInput();
            u32 held = hidKeysHeld();

            // map 3DS buttons to Game Boy (active-low: false = pressed)
            buttonA = !(held & KEY_A);
            buttonB = !(held & KEY_B);
            buttonStart = !(held & KEY_START);
            buttonSelect = !(held & KEY_SELECT);
            dpadUp = !(held & KEY_DUP);
            dpadDown = !(held & KEY_DDOWN);
            dpadLeft = !(held & KEY_DLEFT);
            dpadRight = !(held & KEY_DRIGHT);

            // check if any button was pressed - request joypad interrupt
            if (!buttonA || !buttonB || !buttonStart || !buttonSelect ||
                !dpadUp || !dpadDown || !dpadLeft || !dpadRight) {
                memory::io[memory::IO_IF] |= 0x10;  // bit 4 = joypad interrupt
            }
        }

        uint8_t read() {
            uint8_t result = 0xFF;

            if (selectButtons) {
                // bit 5 low = buttons selected
                result &= ~0x20;

                if (!buttonA)      result &= ~0x01;
                if (!buttonB)      result &= ~0x02;
                if (!buttonSelect) result &= ~0x04;
                if (!buttonStart)  result &= ~0x08;
            }

            if (selectDpad) {
                // bit 4 low = d-pad selected
                result &= ~0x10;

                if (!dpadRight) result &= ~0x01;
                if (!dpadLeft)  result &= ~0x02;
                if (!dpadUp)    result &= ~0x04;
                if (!dpadDown)  result &= ~0x08;
            }

            return result;
        }

        void write(uint8_t value) {
            // bits 4-5 select which group to read
            // 0 = selected, 1 = not selected (active-low)
            selectButtons = !(value & 0x20);
            selectDpad = !(value & 0x10);
        }
    }
}