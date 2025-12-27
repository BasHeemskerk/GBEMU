#ifndef GB_GB_HPP
#define GB_GB_HPP

#include <cstdint>

namespace gb {

    // initialize all subsystems
    void initialize();

    // load a ROM file
    bool loadROM(const char* filepath);

    // run one frame (until vblank)
    void runFrame();

    // run a single CPU step
    void step();

    // handle interrupts
    void handleInterrupts();

}  // namespace gb

#endif