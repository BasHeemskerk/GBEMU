#ifndef GB_STATE_HPP
#define GB_STATE_HPP

#include <cstdint>

namespace gb {

    // CPU state
    struct CPUState {
        uint8_t A, B, C, D, E, F, H, L;
        uint16_t SP, PC;
        bool ime;
        bool imeScheduled;
        bool halted;
    };

    // PPU state
    struct PPUState {
        uint8_t framebuffer[160 * 144];
        bool frameReady;
        int scanlineCycles;
    };

    // APU channel states
    struct Channel1State {
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

    struct Channel2State {
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

    struct Channel3State {
        bool enabled;
        bool dacEnabled;
        int frequency;
        int frequencyTimer;
        int position;
        int volume;
        int lengthCounter;
    };

    struct Channel4State {
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

    struct APUState {
        static constexpr int BUFFER_SIZE = 2048;
        int16_t audioBuffer[BUFFER_SIZE * 2];
        int bufferPosition;

        int sampleCycles;
        int frameSequencerCycles;
        int frameSequencerStep;

        Channel1State ch1;
        Channel2State ch2;
        Channel3State ch3;
        Channel4State ch4;

        bool masterEnable;
        int masterVolumeLeft;
        int masterVolumeRight;

        bool ch1Left, ch1Right;
        bool ch2Left, ch2Right;
        bool ch3Left, ch3Right;
        bool ch4Left, ch4Right;
    };

    // Timer state
    struct TimerState {
        int divCycles;
        int timaCycles;
    };

    // Joypad state
    struct JoypadState {
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
    };

    // Memory state
    struct MemoryState {
        uint8_t vram[0x2000];
        uint8_t wram[0x2000];
        uint8_t oam[0xA0];
        uint8_t io[0x80];
        uint8_t hram[0x7F];
        uint8_t ie;
    };

    // Cartridge state
    enum class MapperType {
        NONE,
        MBC1,
        MBC3,
        MBC5
    };

    struct CartridgeState {
        static constexpr int MAX_ROM_SIZE = 8 * 1024 * 1024;
        static constexpr int MAX_RAM_SIZE = 128 * 1024;

        uint8_t* rom;
        uint8_t* ram;

        int romSize;
        int ramSize;

        MapperType mapper;
        int romBank;
        int ramBank;
        bool ramEnabled;
        int mbcMode;

        char title[17];
        bool loaded;
    };

    // Complete GB state
    struct GBState {
        CPUState cpu;
        PPUState ppu;
        APUState apu;
        TimerState timer;
        JoypadState joypad;
        MemoryState memory;
        CartridgeState cartridge;
    };

}

#endif
