#ifndef GB_CPU_HPP
#define GB_CPU_HPP

#include <cstdint>

namespace gb{
    namespace cpu{
        
        //the main gameboy registers
        //gb has 8bit registers that can pair into 16bit
        //AF, BC, DE, HL
        //A = accumulator register (main math register)
        //F = flags register (zero, subtract, half carry and carry)
        //B, C, D, E, H and L are general purpose
        extern uint8_t A;
        extern uint8_t F;
        extern uint8_t B;
        extern uint8_t C;
        extern uint8_t D;
        extern uint8_t E;
        extern uint8_t H;
        extern uint8_t L;

        //16bit registers
        extern uint16_t SP; //stack pointer register
        extern uint16_t PC; //program counter

        //CPU state
        extern bool halted; //HALT instruction pauses the CPU until interrupt
        extern bool stopped; //STOP instruction (deep sleep)
        extern bool ime; //interrupt master enable
        extern bool imeScheduled; //IME gets enabled after the next instruction

        //cycle counter
        //tracks how many cycles the last instruction took
        //other components like the PPU and timer need this for synchronization
        extern int cycles;

        //flag bit positions in F register
        /*F Register: 7 N H C 0 0 0 0
                      7 6 5 4 3 2 1 0
        */
       constexpr uint8_t FLAG_Z = 0x80; //zero flag
       constexpr uint8_t FLAG_N = 0x40; //subtract flag
       constexpr uint8_t FLAG_H = 0x20; //half carry flag
       constexpr uint8_t FLAG_C = 0x10; //carry flag

       //helper functions for register pairs
       //BC, DE, HL are often used as 16bit values
       inline uint16_t getBC() { return (B << 8) | C; }
       inline uint16_t getDE() { return (D << 8) | E; }
       inline uint16_t getHL() { return (H << 8) | L; }
       inline uint16_t getAF() { return (A << 8)| (F & 0xF0); } //low 4 bits of F are always 0

       inline void setBC(uint16_t val) { B = val >> 8; C = val & 0xFF; }
       inline void setDE(uint16_t val) { D = val >> 8; E = val & 0xFF; }
       inline void setHL(uint16_t val) { H = val >> 8; L = val & 0xFF; }
       inline void setAF(uint16_t val) { A = val >> 8; F = val & 0xF0; }

       //flag helpers
       inline bool getFlag(uint8_t flag) { return F & flag; }
       inline void setFlag(uint8_t flag, bool val){
        if (val){
            F |= flag;
        } else {
            F &= ~flag;
        }
       }

       //functions
       void initialize();
       int executeCB(uint8_t opcode);
       int step();

    }
}

#endif