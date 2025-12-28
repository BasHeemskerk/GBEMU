#ifndef GB_OPCODE_PARSER_HPP
#define GB_OPCODE_PARSER_HPP

#include <cstdint>

namespace gb{
    //micro op types
    enum class MicroOp : uint8_t {
        NOP,

        //8bit loads
        LD8, //LD8, dst, src
        ST8, //ST8 (addr), src

        //16 bit loads
        LD16, //LD16 dst, src
        ST16, //ST16 (addr), src
        
        //8 bit arithmetic
        ADD8,
        ADC8,
        SUB8,
        SBC8,
        INC8,
        DEC8,
        AND8,
        OR8,
        XOR8,
        CP8,

        //16 bit arithmetic
        ADD16,
        INC16,
        DEC16,
        ADDSP,

        //rotates / shifts (a register)
        RLCA,
        RRCA,
        RLA,
        RRA,

        //rotates / shifts (general)
        RLC,
        RRC,
        RL,
        RR,
        SLA,
        SRA,
        SRL,
        SWAP,

        //bit operations
        BIT,
        RES,
        SET,

        //jumps
        JP,
        JP_Z,
        JP_NZ,
        JP_C,
        JP_NC,
        JR,
        JR_Z,
        JR_NZ,
        JR_C,
        JR_NC,
        JP_HL,

        //calls / returns
        CALL,
        CALL_Z,
        CALL_NZ,
        CALL_C,
        CALL_NC,
        RET,
        RET_Z,
        RET_NZ,
        RET_C,
        RET_NC,
        RETI,
        RST,

        //stack
        PUSH,
        POP,

        //misc
        HALT,
        STOP,
        DI,
        EI,
        DAA,
        CPL,
        CCF,
        SCF,
        LD_HL_SP_E,

        //cb prefix (handled seperate)
        CB
    };

    enum class Operand : uint8_t {
        NONE,

        //8 bit registers
        A, B, C, D, E, H, L, F,

        //16 bit registers
        AF, BC, DE, HL, SP, PC,

        //memory addressing
        MEM_BC,     // (BC)
        MEM_DE,     // (DE)
        MEM_HL,     // (HL)
        MEM_HL_INC, // (HL+)
        MEM_HL_DEC, // (HL-)
        MEM_NN,     // (nn)
        MEM_FF_N,   // (FF00+n)
        MEM_FF_C,   // (FF00+C)
        
        //immediates
        IMM8,       // n
        IMM16,      // nn
        IMM8_SIGNED,// e (signed offset)
        SP_PLUS_E,  //SP + signed immediate
        
        //bit index (0-7)
        BIT_0, BIT_1, BIT_2, BIT_3,
        BIT_4, BIT_5, BIT_6, BIT_7,
        
        //rst vectors
        RST_00, RST_08, RST_10, RST_18,
        RST_20, RST_28, RST_30, RST_38
    };

    struct OpcodeEntry{
        MicroOp op;
        Operand dst;
        Operand src;
        uint8_t cycles;
        uint8_t cyclesBranch;
    };

    struct OpcodeTable{
        char name[32];
        uint8_t version;
        OpcodeEntry main[256]; // 0x00-0xFF
        OpcodeEntry cb[256]; // CB 0x00-0xFF
        bool loaded;
    };

    namespace opcode_parser {
        //parse a .gb_opcode file and fill the table
        bool parse(const char* filepath, OpcodeTable& table);

        //init with built in defaults (fallback)
        void initDefaults(OpcodeTable& table);
    }
}

#endif