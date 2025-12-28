#ifndef WRAPPER_GAMEBOY_HPP
#define WRAPPER_GAMEBOY_HPP

#include <cstdint>
#include <cstddef>

#include "../gb/included/state.hpp"

constexpr int GB_SCREEN_WIDTH = 160;
constexpr int GB_SCREEN_HEIGHT = 144;

constexpr int GB_SAMPLE_RATE = 32768;
constexpr int GB_AUDIO_BUFFER_SIZE = 2048;

class GameBoy {
public:
    GameBoy();
    ~GameBoy();

    void init();
    void reset();
    void runFrame();
    void step();

    bool loadROM(const char* filepath);
    bool isROMLoaded() const;
    const char* getROMTitle() const;

    struct Input {
        bool a;
        bool b;
        bool start;
        bool select;
        bool up;
        bool down;
        bool left;
        bool right;

        void clear() {
            a = b = start = select = false;
            up = down = left = right = false;
        }
    } input;

    uint8_t getInputState() const;
    void setInputState(uint8_t state);

    uint8_t* getFramebuffer();
    const uint8_t* getFramebuffer() const;

    bool isFrameReady() const;
    void clearFrameReady();

    int16_t* getAudioBuffer();
    int getAudioBufferPosition() const;
    void clearAudioBuffer();

    bool hasSRAM() const;
    bool saveSRAM(const char* filepath);
    bool loadSRAM(const char* filepath);

    bool loadOpcodeTable(const char* filepath);

private:
    gb::GBState state;
    bool romLoaded;

    void updateInput();
    void handleInterrupts();
};

#endif
