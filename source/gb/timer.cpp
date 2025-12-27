#include "included/timer.hpp"
#include "included/state.hpp"

namespace gb {
    namespace timer {

        static const int clockSelect[] = { 1024, 16, 64, 256 };

        void initialize(GBState& state) {
            state.timer.divCycles = 0;
            state.timer.timaCycles = 0;
        }

        void tick(GBState& state, int cycles) {
            auto& timer = state.timer;
            auto& io = state.memory.io;

            timer.divCycles += cycles;
            while (timer.divCycles >= 256) {
                timer.divCycles -= 256;
                io[REG_DIV]++;
            }

            if (!(io[REG_TAC] & 0x04)) {
                return;
            }

            int clockDivider = clockSelect[io[REG_TAC] & 0x03];

            timer.timaCycles += cycles;
            while (timer.timaCycles >= clockDivider) {
                timer.timaCycles -= clockDivider;
                io[REG_TIMA]++;

                if (io[REG_TIMA] == 0) {
                    io[REG_TIMA] = io[REG_TMA];
                    io[0x0F] |= 0x04;
                }
            }
        }

    }
}
