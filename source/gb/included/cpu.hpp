#ifndef GB_CPU_HPP
#define GB_CPU_HPP

#include <cstdint>

namespace gb {

    struct GBState;

    namespace cpu {

        // Flag bit positions in F register
        constexpr uint8_t FLAG_Z = 0x80;  // Zero flag
        constexpr uint8_t FLAG_N = 0x40;  // Subtract flag
        constexpr uint8_t FLAG_H = 0x20;  // Half carry flag
        constexpr uint8_t FLAG_C = 0x10;  // Carry flag

        // Helper functions for register pairs
        inline uint16_t getBC(GBState& state);
        inline uint16_t getDE(GBState& state);
        inline uint16_t getHL(GBState& state);
        inline uint16_t getAF(GBState& state);

        inline void setBC(GBState& state, uint16_t val);
        inline void setDE(GBState& state, uint16_t val);
        inline void setHL(GBState& state, uint16_t val);
        inline void setAF(GBState& state, uint16_t val);

        // Flag helpers
        inline bool getFlag(GBState& state, uint8_t flag);
        inline void setFlag(GBState& state, uint8_t flag, bool val);

        // Functions
        void initialize(GBState& state);
        int step(GBState& state);
        int executeCB(GBState& state, uint8_t opcode);

        // Memory access helpers
        uint8_t fetchByte(GBState& state);
        uint16_t fetchWord(GBState& state);
        void pushStack(GBState& state, uint16_t val);
        uint16_t popStack(GBState& state);

    }
}

#endif