#ifndef GB_GB_HPP
#define GB_GB_HPP

#include <cstdint>

namespace gb {

    struct GBState;

    // Global state accessor
    GBState& getState();

    // Initialize all subsystems
    void initialize();

    // Load a ROM file
    bool loadROM(const char* filepath);

    // Run one frame (until vblank)
    void runFrame();

    // Run a single CPU step
    void step();

    // Handle interrupts
    void handleInterrupts();

    // Accessors for external code
    uint8_t* getFramebuffer();
    bool isFrameReady();
    void setFrameReady(bool ready);

    int16_t* getAudioBuffer();
    int getAudioBufferPosition();
    void setAudioBufferPosition(int pos);

    const char* getROMTitle();
    int getRAMSize();

}  // namespace gb

#endif
