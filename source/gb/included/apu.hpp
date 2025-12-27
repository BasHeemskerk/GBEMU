#ifndef GB_APU_HPP
#define GB_APU_HPP

#include <cstdint>

namespace gb {

    struct GBState;

    namespace apu {

        constexpr int SAMPLE_RATE = 32768;
        constexpr int CYCLES_PER_SAMPLE = 128;
        constexpr int BUFFER_SIZE = 2048;

        extern const uint8_t dutyPatterns[4];

        void initialize(GBState& state);
        void tick(GBState& state, int cycles);
        void tickFrameSequencer(GBState& state);
        void generateSample(GBState& state);

        void tickChannel1(GBState& state);
        void tickChannel2(GBState& state);
        void tickChannel3(GBState& state);
        void tickChannel4(GBState& state);

        void writeRegister(GBState& state, uint8_t reg, uint8_t value);
        uint8_t readRegister(GBState& state, uint8_t reg);

        // Platform-specific audio (3DS)
        void initAudio();
        void exitAudio();

    }
}

#endif
