#ifndef GB_APU_HPP
#define GB_APU_HPP

#include <cstdint>

namespace gb{
    namespace apu{

        //audio sample rate
        constexpr int SAMPLE_RATE = 32768;

        //128 cycles per sample (4194308Hz / 32768 = 128)
        constexpr int CYCLES_PER_SAMPLE = 128;

        //buffer size (in samples)
        constexpr int BUFFER_SIZE = 2048;

        //audio buffer (stereo so 2x)
        extern int16_t audioBuffer[BUFFER_SIZE * 2];
        extern int bufferPosition;

        //cycle counter for sample timing
        extern int sampleCycles;

        //frame sequencer
        extern int frameSequencerCycles;
        extern int frameSequencerStep;

        //channel 1; square wave
        struct Channel1{
            bool enabled;
            int frequency;
            int frequencyTimer;
            int dutyPosition;
            int duty;
            int volume;
            int envelopeTimer;
            int envelopePeriod;
            bool envelopeIncrease;
            int sweepTimer;
            int sweepPeriod;
            bool sweepNegate;
            int sweepShift;
            int shadowFrequency;
            int lengthCounter;
        };

        //channel 2; square wave
        struct Channel2{
            bool enabled;
            int frequency;
            int frequencyTimer;
            int dutyPosition;
            int duty;
            int volume;
            int envelopeTimer;
            int envelopePeriod;
            bool envelopeIncrease;
            int lengthCounter;
        };

        //channel 3; wave channel
        struct Channel3{
            bool enabled;
            bool dacEnabled;
            int frequency;
            int frequencyTimer;
            int position;
            int volume;
            int lengthCounter;
        };

        //channel 4; noise channel
        struct Channel4 {
            bool enabled;
            int volume;
            int envelopeTimer;
            int envelopePeriod;
            bool envelopeIncrease;
            int frequencyTimer;
            int divisor;
            int shiftAmount;
            bool widthMode;
            uint16_t lfsr;
            int lengthCounter;
        };

        extern Channel1 ch1;
        extern Channel2 ch2;
        extern Channel3 ch3;
        extern Channel4 ch4;

        //master volume and enable
        extern bool masterEnable;
        extern int masterVolumeLeft;
        extern int masterVolumeRight;

        //channel panning
        extern bool ch1Left, ch1Right;
        extern bool ch2Left, ch2Right;
        extern bool ch3Left, ch3Right;
        extern bool ch4Left, ch4Right;

        //functions
        void initialize();
        void tick(int cycles);
        void tickFrameSequencer();
        void generateSample();

        //channel update functions
        void tickChannel1();
        void tickChannel2();
        void tickChannel3();
        void tickChannel4();

        //called when registers are written
        void writeRegister(uint8_t reg, uint8_t value);
        uint8_t readRegister(uint8_t reg);

        //3DS audio
        void initAudio();
        void exitAudio();
        void flushBuffer();
    }
}

#endif