#include "included/cpu.hpp"
#include "included/state.hpp"
#include "included/memory.hpp"

namespace gb {
    namespace cpu {

        // ═══════════════════════════════════════════════════════════════════
        // REGISTER PAIR HELPERS
        // ═══════════════════════════════════════════════════════════════════

        static inline uint16_t getBC(GBState& state) { 
            return (state.cpu.B << 8) | state.cpu.C; 
        }
        static inline uint16_t getDE(GBState& state) { 
            return (state.cpu.D << 8) | state.cpu.E; 
        }
        static inline uint16_t getHL(GBState& state) { 
            return (state.cpu.H << 8) | state.cpu.L; 
        }
        static inline uint16_t getAF(GBState& state) { 
            return (state.cpu.A << 8) | (state.cpu.F & 0xF0); 
        }

        static inline void setBC(GBState& state, uint16_t val) { 
            state.cpu.B = val >> 8; 
            state.cpu.C = val & 0xFF; 
        }
        static inline void setDE(GBState& state, uint16_t val) { 
            state.cpu.D = val >> 8; 
            state.cpu.E = val & 0xFF; 
        }
        static inline void setHL(GBState& state, uint16_t val) { 
            state.cpu.H = val >> 8; 
            state.cpu.L = val & 0xFF; 
        }
        static inline void setAF(GBState& state, uint16_t val) { 
            state.cpu.A = val >> 8; 
            state.cpu.F = val & 0xF0; 
        }

        // ═══════════════════════════════════════════════════════════════════
        // FLAG HELPERS
        // ═══════════════════════════════════════════════════════════════════

        static inline bool getFlag(GBState& state, uint8_t flag) { 
            return state.cpu.F & flag; 
        }

        static inline void setFlag(GBState& state, uint8_t flag, bool val) {
            if (val) {
                state.cpu.F |= flag;
            } else {
                state.cpu.F &= ~flag;
            }
        }

        // ═══════════════════════════════════════════════════════════════════
        // MEMORY ACCESS HELPERS
        // ═══════════════════════════════════════════════════════════════════

        static inline uint8_t fetchByte(GBState& state) {
            uint8_t val = memory::read(state, state.cpu.PC);
            state.cpu.PC++;
            return val;
        }

        static inline uint16_t fetchWord(GBState& state) {
            uint8_t low = memory::read(state, state.cpu.PC);
            state.cpu.PC++;
            uint8_t high = memory::read(state, state.cpu.PC);
            state.cpu.PC++;
            return (high << 8) | low;
        }

        static inline void pushStack(GBState& state, uint16_t val) {
            state.cpu.SP--;
            memory::write(state, state.cpu.SP, val >> 8);
            state.cpu.SP--;
            memory::write(state, state.cpu.SP, val & 0xFF);
        }

        static inline uint16_t popStack(GBState& state) {
            uint8_t low = memory::read(state, state.cpu.SP);
            state.cpu.SP++;
            uint8_t high = memory::read(state, state.cpu.SP);
            state.cpu.SP++;
            return (high << 8) | low;
        }

        // ═══════════════════════════════════════════════════════════════════
        // INITIALIZATION
        // ═══════════════════════════════════════════════════════════════════

        void initialize(GBState& state) {
            auto& cpu = state.cpu;

            cpu.A = 0x01;
            cpu.F = 0xB0;
            cpu.B = 0x00;
            cpu.C = 0x13;
            cpu.D = 0x00;
            cpu.E = 0xD8;
            cpu.H = 0x01;
            cpu.L = 0x4D;

            cpu.SP = 0xFFFE;
            cpu.PC = 0x0100;

            cpu.halted = false;
            cpu.ime = false;
            cpu.imeScheduled = false;
        }

        // ═══════════════════════════════════════════════════════════════════
        // CB PREFIX OPCODES
        // ═══════════════════════════════════════════════════════════════════

        int executeCB(GBState& state, uint8_t opcode) {
            auto& cpu = state.cpu;

            uint8_t* reg = nullptr;
            uint8_t hlVal = 0;
            int regIndex = opcode & 0x07;
            bool isHL = (regIndex == 6);

            switch (regIndex) {
                case 0: reg = &cpu.B; break;
                case 1: reg = &cpu.C; break;
                case 2: reg = &cpu.D; break;
                case 3: reg = &cpu.E; break;
                case 4: reg = &cpu.H; break;
                case 5: reg = &cpu.L; break;
                case 6: hlVal = memory::read(state, getHL(state)); break;
                case 7: reg = &cpu.A; break;
            }

            uint8_t val = isHL ? hlVal : *reg;
            uint8_t result = val;
            int cycles = isHL ? 16 : 8;

            uint8_t op = opcode >> 3;

            if (opcode < 0x40) {
                switch (op) {
                    case 0:  // RLC
                        {
                            uint8_t bit7 = (val >> 7) & 1;
                            result = (val << 1) | bit7;
                            setFlag(state, FLAG_Z, result == 0);
                            setFlag(state, FLAG_N, false);
                            setFlag(state, FLAG_H, false);
                            setFlag(state, FLAG_C, bit7);
                        }
                        break;

                    case 1:  // RRC
                        {
                            uint8_t bit0 = val & 1;
                            result = (val >> 1) | (bit0 << 7);
                            setFlag(state, FLAG_Z, result == 0);
                            setFlag(state, FLAG_N, false);
                            setFlag(state, FLAG_H, false);
                            setFlag(state, FLAG_C, bit0);
                        }
                        break;

                    case 2:  // RL
                        {
                            uint8_t bit7 = (val >> 7) & 1;
                            result = (val << 1) | (getFlag(state, FLAG_C) ? 1 : 0);
                            setFlag(state, FLAG_Z, result == 0);
                            setFlag(state, FLAG_N, false);
                            setFlag(state, FLAG_H, false);
                            setFlag(state, FLAG_C, bit7);
                        }
                        break;

                    case 3:  // RR
                        {
                            uint8_t bit0 = val & 1;
                            result = (val >> 1) | (getFlag(state, FLAG_C) ? 0x80 : 0);
                            setFlag(state, FLAG_Z, result == 0);
                            setFlag(state, FLAG_N, false);
                            setFlag(state, FLAG_H, false);
                            setFlag(state, FLAG_C, bit0);
                        }
                        break;

                    case 4:  // SLA
                        {
                            uint8_t bit7 = (val >> 7) & 1;
                            result = val << 1;
                            setFlag(state, FLAG_Z, result == 0);
                            setFlag(state, FLAG_N, false);
                            setFlag(state, FLAG_H, false);
                            setFlag(state, FLAG_C, bit7);
                        }
                        break;

                    case 5:  // SRA
                        {
                            uint8_t bit0 = val & 1;
                            uint8_t bit7 = val & 0x80;
                            result = (val >> 1) | bit7;
                            setFlag(state, FLAG_Z, result == 0);
                            setFlag(state, FLAG_N, false);
                            setFlag(state, FLAG_H, false);
                            setFlag(state, FLAG_C, bit0);
                        }
                        break;

                    case 6:  // SWAP
                        result = ((val & 0x0F) << 4) | ((val & 0xF0) >> 4);
                        setFlag(state, FLAG_Z, result == 0);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, false);
                        setFlag(state, FLAG_C, false);
                        break;

                    case 7:  // SRL
                        {
                            uint8_t bit0 = val & 1;
                            result = val >> 1;
                            setFlag(state, FLAG_Z, result == 0);
                            setFlag(state, FLAG_N, false);
                            setFlag(state, FLAG_H, false);
                            setFlag(state, FLAG_C, bit0);
                        }
                        break;
                }

                if (isHL) {
                    memory::write(state, getHL(state), result);
                } else {
                    *reg = result;
                }
            }
            else if (opcode < 0x80) {
                // BIT
                int bit = (opcode >> 3) & 0x07;
                setFlag(state, FLAG_Z, (val & (1 << bit)) == 0);
                setFlag(state, FLAG_N, false);
                setFlag(state, FLAG_H, true);
                cycles = isHL ? 12 : 8;
            }
            else if (opcode < 0xC0) {
                // RES
                int bit = (opcode >> 3) & 0x07;
                result = val & ~(1 << bit);

                if (isHL) {
                    memory::write(state, getHL(state), result);
                } else {
                    *reg = result;
                }
            }
            else {
                // SET
                int bit = (opcode >> 3) & 0x07;
                result = val | (1 << bit);

                if (isHL) {
                    memory::write(state, getHL(state), result);
                } else {
                    *reg = result;
                }
            }

            return cycles;
        }

        // ═══════════════════════════════════════════════════════════════════
        // MAIN STEP FUNCTION
        // ═══════════════════════════════════════════════════════════════════

        int step(GBState& state) {
            auto& cpu = state.cpu;
            int cycles = 0;

            if (cpu.imeScheduled) {
                cpu.ime = true;
                cpu.imeScheduled = false;
            }

            if (cpu.halted) {
                return 4;
            }

            uint8_t opcode = fetchByte(state);

            switch (opcode) {

                case 0x00:  // NOP
                    cycles = 4;
                    break;

                case 0x01:  // LD BC, nn
                    setBC(state, fetchWord(state));
                    cycles = 12;
                    break;

                case 0x02:  // LD (BC), A
                    memory::write(state, getBC(state), cpu.A);
                    cycles = 8;
                    break;

                case 0x03:  // INC BC
                    setBC(state, getBC(state) + 1);
                    cycles = 8;
                    break;

                case 0x04:  // INC B
                    cpu.B++;
                    setFlag(state, FLAG_Z, cpu.B == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, (cpu.B & 0x0F) == 0x00);
                    cycles = 4;
                    break;

                case 0x05:  // DEC B
                    cpu.B--;
                    setFlag(state, FLAG_Z, cpu.B == 0);
                    setFlag(state, FLAG_N, true);
                    setFlag(state, FLAG_H, (cpu.B & 0x0F) == 0x0F);
                    cycles = 4;
                    break;

                case 0x06:  // LD B, n
                    cpu.B = fetchByte(state);
                    cycles = 8;
                    break;

                case 0x07:  // RLCA
                    {
                        uint8_t bit7 = (cpu.A >> 7) & 1;
                        cpu.A = (cpu.A << 1) | bit7;
                        setFlag(state, FLAG_Z, false);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, false);
                        setFlag(state, FLAG_C, bit7);
                    }
                    cycles = 4;
                    break;

                case 0x08:  // LD (nn), SP
                    {
                        uint16_t addr = fetchWord(state);
                        memory::write(state, addr, cpu.SP & 0xFF);
                        memory::write(state, addr + 1, cpu.SP >> 8);
                    }
                    cycles = 20;
                    break;

                case 0x09:  // ADD HL, BC
                    {
                        uint32_t result = getHL(state) + getBC(state);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, ((getHL(state) & 0x0FFF) + (getBC(state) & 0x0FFF)) > 0x0FFF);
                        setFlag(state, FLAG_C, result > 0xFFFF);
                        setHL(state, result & 0xFFFF);
                    }
                    cycles = 8;
                    break;

                case 0x0A:  // LD A, (BC)
                    cpu.A = memory::read(state, getBC(state));
                    cycles = 8;
                    break;

                case 0x0B:  // DEC BC
                    setBC(state, getBC(state) - 1);
                    cycles = 8;
                    break;

                case 0x0C:  // INC C
                    cpu.C++;
                    setFlag(state, FLAG_Z, cpu.C == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, (cpu.C & 0x0F) == 0x00);
                    cycles = 4;
                    break;

                case 0x0D:  // DEC C
                    cpu.C--;
                    setFlag(state, FLAG_Z, cpu.C == 0);
                    setFlag(state, FLAG_N, true);
                    setFlag(state, FLAG_H, (cpu.C & 0x0F) == 0x0F);
                    cycles = 4;
                    break;

                case 0x0E:  // LD C, n
                    cpu.C = fetchByte(state);
                    cycles = 8;
                    break;

                case 0x0F:  // RRCA
                    {
                        uint8_t bit0 = cpu.A & 1;
                        cpu.A = (cpu.A >> 1) | (bit0 << 7);
                        setFlag(state, FLAG_Z, false);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, false);
                        setFlag(state, FLAG_C, bit0);
                    }
                    cycles = 4;
                    break;

                case 0x10:  // STOP
                    fetchByte(state);
                    cycles = 4;
                    break;

                case 0x11:  // LD DE, nn
                    setDE(state, fetchWord(state));
                    cycles = 12;
                    break;

                case 0x12:  // LD (DE), A
                    memory::write(state, getDE(state), cpu.A);
                    cycles = 8;
                    break;

                case 0x13:  // INC DE
                    setDE(state, getDE(state) + 1);
                    cycles = 8;
                    break;

                case 0x14:  // INC D
                    cpu.D++;
                    setFlag(state, FLAG_Z, cpu.D == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, (cpu.D & 0x0F) == 0x00);
                    cycles = 4;
                    break;

                case 0x15:  // DEC D
                    cpu.D--;
                    setFlag(state, FLAG_Z, cpu.D == 0);
                    setFlag(state, FLAG_N, true);
                    setFlag(state, FLAG_H, (cpu.D & 0x0F) == 0x0F);
                    cycles = 4;
                    break;

                case 0x16:  // LD D, n
                    cpu.D = fetchByte(state);
                    cycles = 8;
                    break;

                case 0x17:  // RLA
                    {
                        uint8_t bit7 = (cpu.A >> 7) & 1;
                        cpu.A = (cpu.A << 1) | (getFlag(state, FLAG_C) ? 1 : 0);
                        setFlag(state, FLAG_Z, false);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, false);
                        setFlag(state, FLAG_C, bit7);
                    }
                    cycles = 4;
                    break;

                case 0x18:  // JR n
                    {
                        int8_t offset = (int8_t)fetchByte(state);
                        cpu.PC += offset;
                    }
                    cycles = 12;
                    break;

                case 0x19:  // ADD HL, DE
                    {
                        uint32_t result = getHL(state) + getDE(state);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, ((getHL(state) & 0x0FFF) + (getDE(state) & 0x0FFF)) > 0x0FFF);
                        setFlag(state, FLAG_C, result > 0xFFFF);
                        setHL(state, result & 0xFFFF);
                    }
                    cycles = 8;
                    break;

                case 0x1A:  // LD A, (DE)
                    cpu.A = memory::read(state, getDE(state));
                    cycles = 8;
                    break;

                case 0x1B:  // DEC DE
                    setDE(state, getDE(state) - 1);
                    cycles = 8;
                    break;

                case 0x1C:  // INC E
                    cpu.E++;
                    setFlag(state, FLAG_Z, cpu.E == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, (cpu.E & 0x0F) == 0x00);
                    cycles = 4;
                    break;

                case 0x1D:  // DEC E
                    cpu.E--;
                    setFlag(state, FLAG_Z, cpu.E == 0);
                    setFlag(state, FLAG_N, true);
                    setFlag(state, FLAG_H, (cpu.E & 0x0F) == 0x0F);
                    cycles = 4;
                    break;

                case 0x1E:  // LD E, n
                    cpu.E = fetchByte(state);
                    cycles = 8;
                    break;

                case 0x1F:  // RRA
                    {
                        uint8_t bit0 = cpu.A & 1;
                        cpu.A = (cpu.A >> 1) | (getFlag(state, FLAG_C) ? 0x80 : 0);
                        setFlag(state, FLAG_Z, false);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, false);
                        setFlag(state, FLAG_C, bit0);
                    }
                    cycles = 4;
                    break;

                case 0x20:  // JR NZ, n
                    {
                        int8_t offset = (int8_t)fetchByte(state);
                        if (!getFlag(state, FLAG_Z)) {
                            cpu.PC += offset;
                            cycles = 12;
                        } else {
                            cycles = 8;
                        }
                    }
                    break;

                case 0x21:  // LD HL, nn
                    setHL(state, fetchWord(state));
                    cycles = 12;
                    break;

                case 0x22:  // LD (HL+), A
                    memory::write(state, getHL(state), cpu.A);
                    setHL(state, getHL(state) + 1);
                    cycles = 8;
                    break;

                case 0x23:  // INC HL
                    setHL(state, getHL(state) + 1);
                    cycles = 8;
                    break;

                case 0x24:  // INC H
                    cpu.H++;
                    setFlag(state, FLAG_Z, cpu.H == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, (cpu.H & 0x0F) == 0x00);
                    cycles = 4;
                    break;

                case 0x25:  // DEC H
                    cpu.H--;
                    setFlag(state, FLAG_Z, cpu.H == 0);
                    setFlag(state, FLAG_N, true);
                    setFlag(state, FLAG_H, (cpu.H & 0x0F) == 0x0F);
                    cycles = 4;
                    break;

                case 0x26:  // LD H, n
                    cpu.H = fetchByte(state);
                    cycles = 8;
                    break;

                case 0x27:  // DAA
                    {
                        int a = cpu.A;
                        if (!getFlag(state, FLAG_N)) {
                            if (getFlag(state, FLAG_H) || (a & 0x0F) > 9) {
                                a += 0x06;
                            }
                            if (getFlag(state, FLAG_C) || a > 0x9F) {
                                a += 0x60;
                                setFlag(state, FLAG_C, true);
                            }
                        } else {
                            if (getFlag(state, FLAG_H)) {
                                a -= 0x06;
                            }
                            if (getFlag(state, FLAG_C)) {
                                a -= 0x60;
                            }
                        }
                        cpu.A = a & 0xFF;
                        setFlag(state, FLAG_Z, cpu.A == 0);
                        setFlag(state, FLAG_H, false);
                    }
                    cycles = 4;
                    break;

                case 0x28:  // JR Z, n
                    {
                        int8_t offset = (int8_t)fetchByte(state);
                        if (getFlag(state, FLAG_Z)) {
                            cpu.PC += offset;
                            cycles = 12;
                        } else {
                            cycles = 8;
                        }
                    }
                    break;

                case 0x29:  // ADD HL, HL
                    {
                        uint32_t hl = getHL(state);
                        uint32_t result = hl + hl;
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, ((hl & 0x0FFF) + (hl & 0x0FFF)) > 0x0FFF);
                        setFlag(state, FLAG_C, result > 0xFFFF);
                        setHL(state, result & 0xFFFF);
                    }
                    cycles = 8;
                    break;

                case 0x2A:  // LD A, (HL+)
                    cpu.A = memory::read(state, getHL(state));
                    setHL(state, getHL(state) + 1);
                    cycles = 8;
                    break;

                case 0x2B:  // DEC HL
                    setHL(state, getHL(state) - 1);
                    cycles = 8;
                    break;

                case 0x2C:  // INC L
                    cpu.L++;
                    setFlag(state, FLAG_Z, cpu.L == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, (cpu.L & 0x0F) == 0x00);
                    cycles = 4;
                    break;

                case 0x2D:  // DEC L
                    cpu.L--;
                    setFlag(state, FLAG_Z, cpu.L == 0);
                    setFlag(state, FLAG_N, true);
                    setFlag(state, FLAG_H, (cpu.L & 0x0F) == 0x0F);
                    cycles = 4;
                    break;

                case 0x2E:  // LD L, n
                    cpu.L = fetchByte(state);
                    cycles = 8;
                    break;

                case 0x2F:  // CPL
                    cpu.A = ~cpu.A;
                    setFlag(state, FLAG_N, true);
                    setFlag(state, FLAG_H, true);
                    cycles = 4;
                    break;

                case 0x30:  // JR NC, n
                    {
                        int8_t offset = (int8_t)fetchByte(state);
                        if (!getFlag(state, FLAG_C)) {
                            cpu.PC += offset;
                            cycles = 12;
                        } else {
                            cycles = 8;
                        }
                    }
                    break;

                case 0x31:  // LD SP, nn
                    cpu.SP = fetchWord(state);
                    cycles = 12;
                    break;

                case 0x32:  // LD (HL-), A
                    memory::write(state, getHL(state), cpu.A);
                    setHL(state, getHL(state) - 1);
                    cycles = 8;
                    break;

                case 0x33:  // INC SP
                    cpu.SP++;
                    cycles = 8;
                    break;

                case 0x34:  // INC (HL)
                    {
                        uint8_t val = memory::read(state, getHL(state));
                        val++;
                        memory::write(state, getHL(state), val);
                        setFlag(state, FLAG_Z, val == 0);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, (val & 0x0F) == 0x00);
                    }
                    cycles = 12;
                    break;

                case 0x35:  // DEC (HL)
                    {
                        uint8_t val = memory::read(state, getHL(state));
                        val--;
                        memory::write(state, getHL(state), val);
                        setFlag(state, FLAG_Z, val == 0);
                        setFlag(state, FLAG_N, true);
                        setFlag(state, FLAG_H, (val & 0x0F) == 0x0F);
                    }
                    cycles = 12;
                    break;

                case 0x36:  // LD (HL), n
                    memory::write(state, getHL(state), fetchByte(state));
                    cycles = 12;
                    break;

                case 0x37:  // SCF
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, false);
                    setFlag(state, FLAG_C, true);
                    cycles = 4;
                    break;

                case 0x38:  // JR C, n
                    {
                        int8_t offset = (int8_t)fetchByte(state);
                        if (getFlag(state, FLAG_C)) {
                            cpu.PC += offset;
                            cycles = 12;
                        } else {
                            cycles = 8;
                        }
                    }
                    break;

                case 0x39:  // ADD HL, SP
                    {
                        uint32_t result = getHL(state) + cpu.SP;
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, ((getHL(state) & 0x0FFF) + (cpu.SP & 0x0FFF)) > 0x0FFF);
                        setFlag(state, FLAG_C, result > 0xFFFF);
                        setHL(state, result & 0xFFFF);
                    }
                    cycles = 8;
                    break;

                case 0x3A:  // LD A, (HL-)
                    cpu.A = memory::read(state, getHL(state));
                    setHL(state, getHL(state) - 1);
                    cycles = 8;
                    break;

                case 0x3B:  // DEC SP
                    cpu.SP--;
                    cycles = 8;
                    break;

                case 0x3C:  // INC A
                    cpu.A++;
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, (cpu.A & 0x0F) == 0x00);
                    cycles = 4;
                    break;

                case 0x3D:  // DEC A
                    cpu.A--;
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, true);
                    setFlag(state, FLAG_H, (cpu.A & 0x0F) == 0x0F);
                    cycles = 4;
                    break;

                case 0x3E:  // LD A, n
                    cpu.A = fetchByte(state);
                    cycles = 8;
                    break;

                case 0x3F:  // CCF
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, false);
                    setFlag(state, FLAG_C, !getFlag(state, FLAG_C));
                    cycles = 4;
                    break;

                case 0x40: cpu.B = cpu.B; cycles = 4; break;
                case 0x41: cpu.B = cpu.C; cycles = 4; break;
                case 0x42: cpu.B = cpu.D; cycles = 4; break;
                case 0x43: cpu.B = cpu.E; cycles = 4; break;
                case 0x44: cpu.B = cpu.H; cycles = 4; break;
                case 0x45: cpu.B = cpu.L; cycles = 4; break;
                case 0x46: cpu.B = memory::read(state, getHL(state)); cycles = 8; break;
                case 0x47: cpu.B = cpu.A; cycles = 4; break;

                case 0x48: cpu.C = cpu.B; cycles = 4; break;
                case 0x49: cpu.C = cpu.C; cycles = 4; break;
                case 0x4A: cpu.C = cpu.D; cycles = 4; break;
                case 0x4B: cpu.C = cpu.E; cycles = 4; break;
                case 0x4C: cpu.C = cpu.H; cycles = 4; break;
                case 0x4D: cpu.C = cpu.L; cycles = 4; break;
                case 0x4E: cpu.C = memory::read(state, getHL(state)); cycles = 8; break;
                case 0x4F: cpu.C = cpu.A; cycles = 4; break;

                

                default:
                    cycles = 4;
                    break;
            }

            return cycles;
        }

    }  // namespace cpu
}  // namespace gb