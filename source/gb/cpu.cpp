#include "included/cpu.hpp"
#include "included/state.hpp"
#include "included/memory.hpp"

namespace gb {
    namespace cpu {

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

        inline uint8_t* getRegister(GBState& state, uint8_t index) {
            switch (index) {
                case 0: return &state.cpu.B;
                case 1: return &state.cpu.C;
                case 2: return &state.cpu.D;
                case 3: return &state.cpu.E;
                case 4: return &state.cpu.H;
                case 5: return &state.cpu.L;
                case 7: return &state.cpu.A;
                default: return nullptr;
            }
        }

        inline uint8_t rlc(GBState& state, uint8_t val) {
            uint8_t result = (val << 1) | (val >> 7);
            setFlag(state, FLAG_Z, result == 0);
            setFlag(state, FLAG_N, false);
            setFlag(state, FLAG_H, false);
            setFlag(state, FLAG_C, val & 0x80);
            return result;
        }

        inline uint8_t rrc(GBState& state, uint8_t val) {
            uint8_t result = (val >> 1) | (val << 7);
            setFlag(state, FLAG_Z, result == 0);
            setFlag(state, FLAG_N, false);
            setFlag(state, FLAG_H, false);
            setFlag(state, FLAG_C, val & 0x01);
            return result;
        }

        inline uint8_t rl(GBState& state, uint8_t val) {
            uint8_t carry = getFlag(state, FLAG_C) ? 1 : 0;
            uint8_t result = (val << 1) | carry;
            setFlag(state, FLAG_Z, result == 0);
            setFlag(state, FLAG_N, false);
            setFlag(state, FLAG_H, false);
            setFlag(state, FLAG_C, val & 0x80);
            return result;
        }

        inline uint8_t rr(GBState& state, uint8_t val) {
            uint8_t carry = getFlag(state, FLAG_C) ? 0x80 : 0;
            uint8_t result = (val >> 1) | carry;
            setFlag(state, FLAG_Z, result == 0);
            setFlag(state, FLAG_N, false);
            setFlag(state, FLAG_H, false);
            setFlag(state, FLAG_C, val & 0x01);
            return result;
        }

        inline uint8_t sla(GBState& state, uint8_t val) {
            uint8_t result = val << 1;
            setFlag(state, FLAG_Z, result == 0);
            setFlag(state, FLAG_N, false);
            setFlag(state, FLAG_H, false);
            setFlag(state, FLAG_C, val & 0x80);
            return result;
        }

        inline uint8_t sra(GBState& state, uint8_t val) {
            uint8_t result = (val >> 1) | (val & 0x80);
            setFlag(state, FLAG_Z, result == 0);
            setFlag(state, FLAG_N, false);
            setFlag(state, FLAG_H, false);
            setFlag(state, FLAG_C, val & 0x01);
            return result;
        }

        inline uint8_t swap(GBState& state, uint8_t val) {
            uint8_t result = ((val & 0x0F) << 4) | ((val & 0xF0) >> 4);
            setFlag(state, FLAG_Z, result == 0);
            setFlag(state, FLAG_N, false);
            setFlag(state, FLAG_H, false);
            setFlag(state, FLAG_C, false);
            return result;
        }

        inline uint8_t srl(GBState& state, uint8_t val) {
            uint8_t result = val >> 1;
            setFlag(state, FLAG_Z, result == 0);
            setFlag(state, FLAG_N, false);
            setFlag(state, FLAG_H, false);
            setFlag(state, FLAG_C, val & 0x01);
            return result;
        }

        inline void bit(GBState& state, uint8_t b, uint8_t val) {
            setFlag(state, FLAG_Z, !(val & (1 << b)));
            setFlag(state, FLAG_N, false);
            setFlag(state, FLAG_H, true);
        }

        inline uint8_t res(uint8_t b, uint8_t val) {
            return val & ~(1 << b);
        }

        inline uint8_t set(uint8_t b, uint8_t val) {
            return val | (1 << b);
        }

        // ═══════════════════════════════════════════════════════════════════
        // CB Prefix Opcode Execution
        // ═══════════════════════════════════════════════════════════════════

        int executeCB(GBState& state) {
            uint8_t opcode = fetchByte(state);

            uint8_t op = (opcode >> 6) & 0x03;
            uint8_t bitNum = (opcode >> 3) & 0x07;
            uint8_t reg = opcode & 0x07;

            int cycles = (reg == 6) ? 16 : 8;

            uint8_t val;
            if (reg == 6) {
                val = memory::read(state, getHL(state));
            } else {
                val = *getRegister(state, reg);
            }

            uint8_t result = val;

            if (op == 0) {
                switch (bitNum) {
                    case 0: result = rlc(state, val); break;
                    case 1: result = rrc(state, val); break;
                    case 2: result = rl(state, val); break;
                    case 3: result = rr(state, val); break;
                    case 4: result = sla(state, val); break;
                    case 5: result = sra(state, val); break;
                    case 6: result = swap(state, val); break;
                    case 7: result = srl(state, val); break;
                }

                if (reg == 6) {
                    memory::write(state, getHL(state), result);
                } else {
                    *getRegister(state, reg) = result;
                }
            }
            else if (op == 1) {
                bit(state, bitNum, val);
                cycles = (reg == 6) ? 12 : 8;
            }
            else if (op == 2) {
                result = res(bitNum, val);
                if (reg == 6) {
                    memory::write(state, getHL(state), result);
                } else {
                    *getRegister(state, reg) = result;
                }
            }
            else {
                result = set(bitNum, val);
                if (reg == 6) {
                    memory::write(state, getHL(state), result);
                } else {
                    *getRegister(state, reg) = result;
                }
            }

            return cycles;
        }

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

                case 0x50: cpu.D = cpu.B; cycles = 4; break;
                case 0x51: cpu.D = cpu.C; cycles = 4; break;
                case 0x52: cpu.D = cpu.D; cycles = 4; break;
                case 0x53: cpu.D = cpu.E; cycles = 4; break;
                case 0x54: cpu.D = cpu.H; cycles = 4; break;
                case 0x55: cpu.D = cpu.L; cycles = 4; break;
                case 0x56: cpu.D = memory::read(state, getHL(state)); cycles = 8; break;
                case 0x57: cpu.D = cpu.A; cycles = 4; break;

                case 0x58: cpu.E = cpu.B; cycles = 4; break;
                case 0x59: cpu.E = cpu.C; cycles = 4; break;
                case 0x5A: cpu.E = cpu.D; cycles = 4; break;
                case 0x5B: cpu.E = cpu.E; cycles = 4; break;
                case 0x5C: cpu.E = cpu.H; cycles = 4; break;
                case 0x5D: cpu.E = cpu.L; cycles = 4; break;
                case 0x5E: cpu.E = memory::read(state, getHL(state)); cycles = 8; break;
                case 0x5F: cpu.E = cpu.A; cycles = 4; break;

                case 0x60: cpu.H = cpu.B; cycles = 4; break;
                case 0x61: cpu.H = cpu.C; cycles = 4; break;
                case 0x62: cpu.H = cpu.D; cycles = 4; break;
                case 0x63: cpu.H = cpu.E; cycles = 4; break;
                case 0x64: cpu.H = cpu.H; cycles = 4; break;
                case 0x65: cpu.H = cpu.L; cycles = 4; break;
                case 0x66: cpu.H = memory::read(state, getHL(state)); cycles = 8; break;
                case 0x67: cpu.H = cpu.A; cycles = 4; break;

                case 0x68: cpu.L = cpu.B; cycles = 4; break;
                case 0x69: cpu.L = cpu.C; cycles = 4; break;
                case 0x6A: cpu.L = cpu.D; cycles = 4; break;
                case 0x6B: cpu.L = cpu.E; cycles = 4; break;
                case 0x6C: cpu.L = cpu.H; cycles = 4; break;
                case 0x6D: cpu.L = cpu.L; cycles = 4; break;
                case 0x6E: cpu.L = memory::read(state, getHL(state)); cycles = 8; break;
                case 0x6F: cpu.L = cpu.A; cycles = 4; break;

                case 0x70: memory::write(state, getHL(state), cpu.B); cycles = 8; break;
                case 0x71: memory::write(state, getHL(state), cpu.C); cycles = 8; break;
                case 0x72: memory::write(state, getHL(state), cpu.D); cycles = 8; break;
                case 0x73: memory::write(state, getHL(state), cpu.E); cycles = 8; break;
                case 0x74: memory::write(state, getHL(state), cpu.H); cycles = 8; break;
                case 0x75: memory::write(state, getHL(state), cpu.L); cycles = 8; break;
                case 0x76: cpu.halted = true; cycles = 4; break;  // HALT
                case 0x77: memory::write(state, getHL(state), cpu.A); cycles = 8; break;

                case 0x78: cpu.A = cpu.B; cycles = 4; break;
                case 0x79: cpu.A = cpu.C; cycles = 4; break;
                case 0x7A: cpu.A = cpu.D; cycles = 4; break;
                case 0x7B: cpu.A = cpu.E; cycles = 4; break;
                case 0x7C: cpu.A = cpu.H; cycles = 4; break;
                case 0x7D: cpu.A = cpu.L; cycles = 4; break;
                case 0x7E: cpu.A = memory::read(state, getHL(state)); cycles = 8; break;
                case 0x7F: cpu.A = cpu.A; cycles = 4; break;

                // ═══════════════════════════════════════════════════════════
                // 0x80 - 0x87: ADD A, r
                // ═══════════════════════════════════════════════════════════

                case 0x80:  // ADD A, B
                    {
                        uint16_t result = cpu.A + cpu.B;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) + (cpu.B & 0x0F)) > 0x0F);
                        setFlag(state, FLAG_C, result > 0xFF);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x81:  // ADD A, C
                    {
                        uint16_t result = cpu.A + cpu.C;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) + (cpu.C & 0x0F)) > 0x0F);
                        setFlag(state, FLAG_C, result > 0xFF);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x82:  // ADD A, D
                    {
                        uint16_t result = cpu.A + cpu.D;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) + (cpu.D & 0x0F)) > 0x0F);
                        setFlag(state, FLAG_C, result > 0xFF);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x83:  // ADD A, E
                    {
                        uint16_t result = cpu.A + cpu.E;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) + (cpu.E & 0x0F)) > 0x0F);
                        setFlag(state, FLAG_C, result > 0xFF);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x84:  // ADD A, H
                    {
                        uint16_t result = cpu.A + cpu.H;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) + (cpu.H & 0x0F)) > 0x0F);
                        setFlag(state, FLAG_C, result > 0xFF);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x85:  // ADD A, L
                    {
                        uint16_t result = cpu.A + cpu.L;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) + (cpu.L & 0x0F)) > 0x0F);
                        setFlag(state, FLAG_C, result > 0xFF);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x86:  // ADD A, (HL)
                    {
                        uint8_t val = memory::read(state, getHL(state));
                        uint16_t result = cpu.A + val;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) + (val & 0x0F)) > 0x0F);
                        setFlag(state, FLAG_C, result > 0xFF);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 8;
                    break;

                case 0x87:  // ADD A, A
                    {
                        uint16_t result = cpu.A + cpu.A;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) + (cpu.A & 0x0F)) > 0x0F);
                        setFlag(state, FLAG_C, result > 0xFF);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x88:  // ADC A, B
                    {
                        uint8_t carry = getFlag(state, FLAG_C) ? 1 : 0;
                        uint16_t result = cpu.A + cpu.B + carry;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) + (cpu.B & 0x0F) + carry) > 0x0F);
                        setFlag(state, FLAG_C, result > 0xFF);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x89:  // ADC A, C
                    {
                        uint8_t carry = getFlag(state, FLAG_C) ? 1 : 0;
                        uint16_t result = cpu.A + cpu.C + carry;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) + (cpu.C & 0x0F) + carry) > 0x0F);
                        setFlag(state, FLAG_C, result > 0xFF);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x8A:  // ADC A, D
                    {
                        uint8_t carry = getFlag(state, FLAG_C) ? 1 : 0;
                        uint16_t result = cpu.A + cpu.D + carry;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) + (cpu.D & 0x0F) + carry) > 0x0F);
                        setFlag(state, FLAG_C, result > 0xFF);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x8B:  // ADC A, E
                    {
                        uint8_t carry = getFlag(state, FLAG_C) ? 1 : 0;
                        uint16_t result = cpu.A + cpu.E + carry;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) + (cpu.E & 0x0F) + carry) > 0x0F);
                        setFlag(state, FLAG_C, result > 0xFF);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x8C:  // ADC A, H
                    {
                        uint8_t carry = getFlag(state, FLAG_C) ? 1 : 0;
                        uint16_t result = cpu.A + cpu.H + carry;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) + (cpu.H & 0x0F) + carry) > 0x0F);
                        setFlag(state, FLAG_C, result > 0xFF);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x8D:  // ADC A, L
                    {
                        uint8_t carry = getFlag(state, FLAG_C) ? 1 : 0;
                        uint16_t result = cpu.A + cpu.L + carry;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) + (cpu.L & 0x0F) + carry) > 0x0F);
                        setFlag(state, FLAG_C, result > 0xFF);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x8E:  // ADC A, (HL)
                    {
                        uint8_t val = memory::read(state, getHL(state));
                        uint8_t carry = getFlag(state, FLAG_C) ? 1 : 0;
                        uint16_t result = cpu.A + val + carry;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) + (val & 0x0F) + carry) > 0x0F);
                        setFlag(state, FLAG_C, result > 0xFF);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 8;
                    break;

                case 0x8F:  // ADC A, A
                    {
                        uint8_t carry = getFlag(state, FLAG_C) ? 1 : 0;
                        uint16_t result = cpu.A + cpu.A + carry;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) + (cpu.A & 0x0F) + carry) > 0x0F);
                        setFlag(state, FLAG_C, result > 0xFF);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                // ═══════════════════════════════════════════════════════════
                // 0x90 - 0x97: SUB A, r
                // ═══════════════════════════════════════════════════════════

                case 0x90:  // SUB B
                    {
                        int result = cpu.A - cpu.B;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, true);
                        setFlag(state, FLAG_H, (cpu.A & 0x0F) < (cpu.B & 0x0F));
                        setFlag(state, FLAG_C, result < 0);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x91:  // SUB C
                    {
                        int result = cpu.A - cpu.C;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, true);
                        setFlag(state, FLAG_H, (cpu.A & 0x0F) < (cpu.C & 0x0F));
                        setFlag(state, FLAG_C, result < 0);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x92:  // SUB D
                    {
                        int result = cpu.A - cpu.D;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, true);
                        setFlag(state, FLAG_H, (cpu.A & 0x0F) < (cpu.D & 0x0F));
                        setFlag(state, FLAG_C, result < 0);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x93:  // SUB E
                    {
                        int result = cpu.A - cpu.E;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, true);
                        setFlag(state, FLAG_H, (cpu.A & 0x0F) < (cpu.E & 0x0F));
                        setFlag(state, FLAG_C, result < 0);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x94:  // SUB H
                    {
                        int result = cpu.A - cpu.H;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, true);
                        setFlag(state, FLAG_H, (cpu.A & 0x0F) < (cpu.H & 0x0F));
                        setFlag(state, FLAG_C, result < 0);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x95:  // SUB L
                    {
                        int result = cpu.A - cpu.L;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, true);
                        setFlag(state, FLAG_H, (cpu.A & 0x0F) < (cpu.L & 0x0F));
                        setFlag(state, FLAG_C, result < 0);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x96:  // SUB (HL)
                    {
                        uint8_t val = memory::read(state, getHL(state));
                        int result = cpu.A - val;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, true);
                        setFlag(state, FLAG_H, (cpu.A & 0x0F) < (val & 0x0F));
                        setFlag(state, FLAG_C, result < 0);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 8;
                    break;

                case 0x97:  // SUB A
                    cpu.A = 0;
                    setFlag(state, FLAG_Z, true);
                    setFlag(state, FLAG_N, true);
                    setFlag(state, FLAG_H, false);
                    setFlag(state, FLAG_C, false);
                    cycles = 4;
                    break;

                // ═══════════════════════════════════════════════════════════
                // 0x98 - 0x9F: SBC A, r
                // ═══════════════════════════════════════════════════════════

                case 0x98:  // SBC A, B
                    {
                        uint8_t carry = getFlag(state, FLAG_C) ? 1 : 0;
                        int result = cpu.A - cpu.B - carry;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, true);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) - (cpu.B & 0x0F) - carry) < 0);
                        setFlag(state, FLAG_C, result < 0);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x99:  // SBC A, C
                    {
                        uint8_t carry = getFlag(state, FLAG_C) ? 1 : 0;
                        int result = cpu.A - cpu.C - carry;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, true);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) - (cpu.C & 0x0F) - carry) < 0);
                        setFlag(state, FLAG_C, result < 0);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x9A:  // SBC A, D
                    {
                        uint8_t carry = getFlag(state, FLAG_C) ? 1 : 0;
                        int result = cpu.A - cpu.D - carry;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, true);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) - (cpu.D & 0x0F) - carry) < 0);
                        setFlag(state, FLAG_C, result < 0);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x9B:  // SBC A, E
                    {
                        uint8_t carry = getFlag(state, FLAG_C) ? 1 : 0;
                        int result = cpu.A - cpu.E - carry;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, true);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) - (cpu.E & 0x0F) - carry) < 0);
                        setFlag(state, FLAG_C, result < 0);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x9C:  // SBC A, H
                    {
                        uint8_t carry = getFlag(state, FLAG_C) ? 1 : 0;
                        int result = cpu.A - cpu.H - carry;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, true);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) - (cpu.H & 0x0F) - carry) < 0);
                        setFlag(state, FLAG_C, result < 0);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x9D:  // SBC A, L
                    {
                        uint8_t carry = getFlag(state, FLAG_C) ? 1 : 0;
                        int result = cpu.A - cpu.L - carry;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, true);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) - (cpu.L & 0x0F) - carry) < 0);
                        setFlag(state, FLAG_C, result < 0);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x9E:  // SBC A, (HL)
                    {
                        uint8_t val = memory::read(state, getHL(state));
                        uint8_t carry = getFlag(state, FLAG_C) ? 1 : 0;
                        int result = cpu.A - val - carry;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, true);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) - (val & 0x0F) - carry) < 0);
                        setFlag(state, FLAG_C, result < 0);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 8;
                    break;

                case 0x9F:  // SBC A, A
                    {
                        uint8_t carry = getFlag(state, FLAG_C) ? 1 : 0;
                        int result = cpu.A - cpu.A - carry;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, true);
                        setFlag(state, FLAG_H, carry);
                        setFlag(state, FLAG_C, carry);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0xA0:  // AND B
                    cpu.A &= cpu.B;
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, true);
                    setFlag(state, FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xA1:  // AND C
                    cpu.A &= cpu.C;
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, true);
                    setFlag(state, FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xA2:  // AND D
                    cpu.A &= cpu.D;
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, true);
                    setFlag(state, FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xA3:  // AND E
                    cpu.A &= cpu.E;
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, true);
                    setFlag(state, FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xA4:  // AND H
                    cpu.A &= cpu.H;
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, true);
                    setFlag(state, FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xA5:  // AND L
                    cpu.A &= cpu.L;
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, true);
                    setFlag(state, FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xA6:  // AND (HL)
                    cpu.A &= memory::read(state, getHL(state));
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, true);
                    setFlag(state, FLAG_C, false);
                    cycles = 8;
                    break;

                case 0xA7:  // AND A
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, true);
                    setFlag(state, FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xA8:  // XOR B
                    cpu.A ^= cpu.B;
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, false);
                    setFlag(state, FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xA9:  // XOR C
                    cpu.A ^= cpu.C;
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, false);
                    setFlag(state, FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xAA:  // XOR D
                    cpu.A ^= cpu.D;
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, false);
                    setFlag(state, FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xAB:  // XOR E
                    cpu.A ^= cpu.E;
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, false);
                    setFlag(state, FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xAC:  // XOR H
                    cpu.A ^= cpu.H;
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, false);
                    setFlag(state, FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xAD:  // XOR L
                    cpu.A ^= cpu.L;
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, false);
                    setFlag(state, FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xAE:  // XOR (HL)
                    cpu.A ^= memory::read(state, getHL(state));
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, false);
                    setFlag(state, FLAG_C, false);
                    cycles = 8;
                    break;

                case 0xAF:  // XOR A
                    cpu.A = 0;
                    setFlag(state, FLAG_Z, true);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, false);
                    setFlag(state, FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xB0:  // OR B
                    cpu.A |= cpu.B;
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, false);
                    setFlag(state, FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xB1:  // OR C
                    cpu.A |= cpu.C;
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, false);
                    setFlag(state, FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xB2:  // OR D
                    cpu.A |= cpu.D;
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, false);
                    setFlag(state, FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xB3:  // OR E
                    cpu.A |= cpu.E;
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, false);
                    setFlag(state, FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xB4:  // OR H
                    cpu.A |= cpu.H;
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, false);
                    setFlag(state, FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xB5:  // OR L
                    cpu.A |= cpu.L;
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, false);
                    setFlag(state, FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xB6:  // OR (HL)
                    cpu.A |= memory::read(state, getHL(state));
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, false);
                    setFlag(state, FLAG_C, false);
                    cycles = 8;
                    break;

                case 0xB7:  // OR A
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, false);
                    setFlag(state, FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xB8:  // CP B
                    setFlag(state, FLAG_Z, cpu.A == cpu.B);
                    setFlag(state, FLAG_N, true);
                    setFlag(state, FLAG_H, (cpu.A & 0x0F) < (cpu.B & 0x0F));
                    setFlag(state, FLAG_C, cpu.A < cpu.B);
                    cycles = 4;
                    break;

                case 0xB9:  // CP C
                    setFlag(state, FLAG_Z, cpu.A == cpu.C);
                    setFlag(state, FLAG_N, true);
                    setFlag(state, FLAG_H, (cpu.A & 0x0F) < (cpu.C & 0x0F));
                    setFlag(state, FLAG_C, cpu.A < cpu.C);
                    cycles = 4;
                    break;

                case 0xBA:  // CP D
                    setFlag(state, FLAG_Z, cpu.A == cpu.D);
                    setFlag(state, FLAG_N, true);
                    setFlag(state, FLAG_H, (cpu.A & 0x0F) < (cpu.D & 0x0F));
                    setFlag(state, FLAG_C, cpu.A < cpu.D);
                    cycles = 4;
                    break;

                case 0xBB:  // CP E
                    setFlag(state, FLAG_Z, cpu.A == cpu.E);
                    setFlag(state, FLAG_N, true);
                    setFlag(state, FLAG_H, (cpu.A & 0x0F) < (cpu.E & 0x0F));
                    setFlag(state, FLAG_C, cpu.A < cpu.E);
                    cycles = 4;
                    break;

                case 0xBC:  // CP H
                    setFlag(state, FLAG_Z, cpu.A == cpu.H);
                    setFlag(state, FLAG_N, true);
                    setFlag(state, FLAG_H, (cpu.A & 0x0F) < (cpu.H & 0x0F));
                    setFlag(state, FLAG_C, cpu.A < cpu.H);
                    cycles = 4;
                    break;

                case 0xBD:  // CP L
                    setFlag(state, FLAG_Z, cpu.A == cpu.L);
                    setFlag(state, FLAG_N, true);
                    setFlag(state, FLAG_H, (cpu.A & 0x0F) < (cpu.L & 0x0F));
                    setFlag(state, FLAG_C, cpu.A < cpu.L);
                    cycles = 4;
                    break;

                case 0xBE:  // CP (HL)
                    {
                        uint8_t val = memory::read(state, getHL(state));
                        setFlag(state, FLAG_Z, cpu.A == val);
                        setFlag(state, FLAG_N, true);
                        setFlag(state, FLAG_H, (cpu.A & 0x0F) < (val & 0x0F));
                        setFlag(state, FLAG_C, cpu.A < val);
                    }
                    cycles = 8;
                    break;

                case 0xBF:  // CP A
                    setFlag(state, FLAG_Z, true);
                    setFlag(state, FLAG_N, true);
                    setFlag(state, FLAG_H, false);
                    setFlag(state, FLAG_C, false);
                    cycles = 4;
                    break;

                // ═══════════════════════════════════════════════════════════
                // 0xC0 - 0xCF
                // ═══════════════════════════════════════════════════════════

                case 0xC0:  // RET NZ
                    if (!getFlag(state, FLAG_Z)) {
                        cpu.PC = memory::read(state, cpu.SP) | (memory::read(state, cpu.SP + 1) << 8);
                        cpu.SP += 2;
                        cycles = 20;
                    } else {
                        cycles = 8;
                    }
                    break;

                case 0xC1:  // POP BC
                    cpu.C = memory::read(state, cpu.SP);
                    cpu.B = memory::read(state, cpu.SP + 1);
                    cpu.SP += 2;
                    cycles = 12;
                    break;

                case 0xC2:  // JP NZ, nn
                    {
                        uint16_t addr = fetchWord(state);
                        if (!getFlag(state, FLAG_Z)) {
                            cpu.PC = addr;
                            cycles = 16;
                        } else {
                            cycles = 12;
                        }
                    }
                    break;

                case 0xC3:  // JP nn
                    cpu.PC = fetchWord(state);
                    cycles = 16;
                    break;

                case 0xC4:  // CALL NZ, nn
                    {
                        uint16_t addr = fetchWord(state);
                        if (!getFlag(state, FLAG_Z)) {
                            cpu.SP -= 2;
                            memory::write(state, cpu.SP, cpu.PC & 0xFF);
                            memory::write(state, cpu.SP + 1, cpu.PC >> 8);
                            cpu.PC = addr;
                            cycles = 24;
                        } else {
                            cycles = 12;
                        }
                    }
                    break;

                case 0xC5:  // PUSH BC
                    cpu.SP -= 2;
                    memory::write(state, cpu.SP, cpu.C);
                    memory::write(state, cpu.SP + 1, cpu.B);
                    cycles = 16;
                    break;

                case 0xC6:  // ADD A, n
                    {
                        uint8_t val = fetchByte(state);
                        uint16_t result = cpu.A + val;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) + (val & 0x0F)) > 0x0F);
                        setFlag(state, FLAG_C, result > 0xFF);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 8;
                    break;

                case 0xC7:  // RST 00H
                    cpu.SP -= 2;
                    memory::write(state, cpu.SP, cpu.PC & 0xFF);
                    memory::write(state, cpu.SP + 1, cpu.PC >> 8);
                    cpu.PC = 0x0000;
                    cycles = 16;
                    break;

                case 0xC8:  // RET Z
                    if (getFlag(state, FLAG_Z)) {
                        cpu.PC = memory::read(state, cpu.SP) | (memory::read(state, cpu.SP + 1) << 8);
                        cpu.SP += 2;
                        cycles = 20;
                    } else {
                        cycles = 8;
                    }
                    break;

                case 0xC9:  // RET
                    cpu.PC = memory::read(state, cpu.SP) | (memory::read(state, cpu.SP + 1) << 8);
                    cpu.SP += 2;
                    cycles = 16;
                    break;

                case 0xCA:  // JP Z, nn
                    {
                        uint16_t addr = fetchWord(state);
                        if (getFlag(state, FLAG_Z)) {
                            cpu.PC = addr;
                            cycles = 16;
                        } else {
                            cycles = 12;
                        }
                    }
                    break;

                case 0xCB:  // CB prefix (handled separately)
                    cycles = executeCB(state);
                    break;

                case 0xCC:  // CALL Z, nn
                    {
                        uint16_t addr = fetchWord(state);
                        if (getFlag(state, FLAG_Z)) {
                            cpu.SP -= 2;
                            memory::write(state, cpu.SP, cpu.PC & 0xFF);
                            memory::write(state, cpu.SP + 1, cpu.PC >> 8);
                            cpu.PC = addr;
                            cycles = 24;
                        } else {
                            cycles = 12;
                        }
                    }
                    break;

                case 0xCD:  // CALL nn
                    {
                        uint16_t addr = fetchWord(state);
                        cpu.SP -= 2;
                        memory::write(state, cpu.SP, cpu.PC & 0xFF);
                        memory::write(state, cpu.SP + 1, cpu.PC >> 8);
                        cpu.PC = addr;
                    }
                    cycles = 24;
                    break;

                case 0xCE:  // ADC A, n
                    {
                        uint8_t val = fetchByte(state);
                        uint8_t carry = getFlag(state, FLAG_C) ? 1 : 0;
                        uint16_t result = cpu.A + val + carry;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) + (val & 0x0F) + carry) > 0x0F);
                        setFlag(state, FLAG_C, result > 0xFF);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 8;
                    break;

                case 0xCF:  // RST 08H
                    cpu.SP -= 2;
                    memory::write(state, cpu.SP, cpu.PC & 0xFF);
                    memory::write(state, cpu.SP + 1, cpu.PC >> 8);
                    cpu.PC = 0x0008;
                    cycles = 16;
                    break;

                // ═══════════════════════════════════════════════════════════
                // 0xD0 - 0xDF
                // ═══════════════════════════════════════════════════════════

                case 0xD0:  // RET NC
                    if (!getFlag(state, FLAG_C)) {
                        cpu.PC = memory::read(state, cpu.SP) | (memory::read(state, cpu.SP + 1) << 8);
                        cpu.SP += 2;
                        cycles = 20;
                    } else {
                        cycles = 8;
                    }
                    break;

                case 0xD1:  // POP DE
                    cpu.E = memory::read(state, cpu.SP);
                    cpu.D = memory::read(state, cpu.SP + 1);
                    cpu.SP += 2;
                    cycles = 12;
                    break;

                case 0xD2:  // JP NC, nn
                    {
                        uint16_t addr = fetchWord(state);
                        if (!getFlag(state, FLAG_C)) {
                            cpu.PC = addr;
                            cycles = 16;
                        } else {
                            cycles = 12;
                        }
                    }
                    break;

                // 0xD3 doesn't exist on GB

                case 0xD4:  // CALL NC, nn
                    {
                        uint16_t addr = fetchWord(state);
                        if (!getFlag(state, FLAG_C)) {
                            cpu.SP -= 2;
                            memory::write(state, cpu.SP, cpu.PC & 0xFF);
                            memory::write(state, cpu.SP + 1, cpu.PC >> 8);
                            cpu.PC = addr;
                            cycles = 24;
                        } else {
                            cycles = 12;
                        }
                    }
                    break;

                case 0xD5:  // PUSH DE
                    cpu.SP -= 2;
                    memory::write(state, cpu.SP, cpu.E);
                    memory::write(state, cpu.SP + 1, cpu.D);
                    cycles = 16;
                    break;

                case 0xD6:  // SUB n
                    {
                        uint8_t val = fetchByte(state);
                        int result = cpu.A - val;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, true);
                        setFlag(state, FLAG_H, (cpu.A & 0x0F) < (val & 0x0F));
                        setFlag(state, FLAG_C, result < 0);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 8;
                    break;

                case 0xD7:  // RST 10H
                    cpu.SP -= 2;
                    memory::write(state, cpu.SP, cpu.PC & 0xFF);
                    memory::write(state, cpu.SP + 1, cpu.PC >> 8);
                    cpu.PC = 0x0010;
                    cycles = 16;
                    break;

                case 0xD8:  // RET C
                    if (getFlag(state, FLAG_C)) {
                        cpu.PC = memory::read(state, cpu.SP) | (memory::read(state, cpu.SP + 1) << 8);
                        cpu.SP += 2;
                        cycles = 20;
                    } else {
                        cycles = 8;
                    }
                    break;

                case 0xD9:  // RETI
                    cpu.PC = memory::read(state, cpu.SP) | (memory::read(state, cpu.SP + 1) << 8);
                    cpu.SP += 2;
                    cpu.ime = true;
                    cycles = 16;
                    break;

                case 0xDA:  // JP C, nn
                    {
                        uint16_t addr = fetchWord(state);
                        if (getFlag(state, FLAG_C)) {
                            cpu.PC = addr;
                            cycles = 16;
                        } else {
                            cycles = 12;
                        }
                    }
                    break;

                // 0xDB doesn't exist on GB

                case 0xDC:  // CALL C, nn
                    {
                        uint16_t addr = fetchWord(state);
                        if (getFlag(state, FLAG_C)) {
                            cpu.SP -= 2;
                            memory::write(state, cpu.SP, cpu.PC & 0xFF);
                            memory::write(state, cpu.SP + 1, cpu.PC >> 8);
                            cpu.PC = addr;
                            cycles = 24;
                        } else {
                            cycles = 12;
                        }
                    }
                    break;

                // 0xDD doesn't exist on GB

                case 0xDE:  // SBC A, n
                    {
                        uint8_t val = fetchByte(state);
                        uint8_t carry = getFlag(state, FLAG_C) ? 1 : 0;
                        int result = cpu.A - val - carry;
                        setFlag(state, FLAG_Z, (result & 0xFF) == 0);
                        setFlag(state, FLAG_N, true);
                        setFlag(state, FLAG_H, ((cpu.A & 0x0F) - (val & 0x0F) - carry) < 0);
                        setFlag(state, FLAG_C, result < 0);
                        cpu.A = result & 0xFF;
                    }
                    cycles = 8;
                    break;

                case 0xDF:  // RST 18H
                    cpu.SP -= 2;
                    memory::write(state, cpu.SP, cpu.PC & 0xFF);
                    memory::write(state, cpu.SP + 1, cpu.PC >> 8);
                    cpu.PC = 0x0018;
                    cycles = 16;
                    break;

                case 0xE0:  // LDH (n), A - write to 0xFF00 + n
                    memory::write(state, 0xFF00 + fetchByte(state), cpu.A);
                    cycles = 12;
                    break;

                case 0xE1:  // POP HL
                    cpu.L = memory::read(state, cpu.SP);
                    cpu.H = memory::read(state, cpu.SP + 1);
                    cpu.SP += 2;
                    cycles = 12;
                    break;

                case 0xE2:  // LD (C), A - write to 0xFF00 + C
                    memory::write(state, 0xFF00 + cpu.C, cpu.A);
                    cycles = 8;
                    break;

                // 0xE3 doesn't exist on GB
                // 0xE4 doesn't exist on GB

                case 0xE5:  // PUSH HL
                    cpu.SP -= 2;
                    memory::write(state, cpu.SP, cpu.L);
                    memory::write(state, cpu.SP + 1, cpu.H);
                    cycles = 16;
                    break;

                case 0xE6:  // AND n
                    cpu.A &= fetchByte(state);
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, true);
                    setFlag(state, FLAG_C, false);
                    cycles = 8;
                    break;

                case 0xE7:  // RST 20H
                    cpu.SP -= 2;
                    memory::write(state, cpu.SP, cpu.PC & 0xFF);
                    memory::write(state, cpu.SP + 1, cpu.PC >> 8);
                    cpu.PC = 0x0020;
                    cycles = 16;
                    break;

                case 0xE8:  // ADD SP, n (signed)
                    {
                        int8_t offset = (int8_t)fetchByte(state);
                        uint16_t sp = cpu.SP;
                        setFlag(state, FLAG_Z, false);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, ((sp & 0x0F) + (offset & 0x0F)) > 0x0F);
                        setFlag(state, FLAG_C, ((sp & 0xFF) + (offset & 0xFF)) > 0xFF);
                        cpu.SP = sp + offset;
                    }
                    cycles = 16;
                    break;

                case 0xE9:  // JP HL
                    cpu.PC = getHL(state);
                    cycles = 4;
                    break;

                case 0xEA:  // LD (nn), A
                    memory::write(state, fetchWord(state), cpu.A);
                    cycles = 16;
                    break;

                // 0xEB doesn't exist on GB
                // 0xEC doesn't exist on GB
                // 0xED doesn't exist on GB

                case 0xEE:  // XOR n
                    cpu.A ^= fetchByte(state);
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, false);
                    setFlag(state, FLAG_C, false);
                    cycles = 8;
                    break;

                case 0xEF:  // RST 28H
                    cpu.SP -= 2;
                    memory::write(state, cpu.SP, cpu.PC & 0xFF);
                    memory::write(state, cpu.SP + 1, cpu.PC >> 8);
                    cpu.PC = 0x0028;
                    cycles = 16;
                    break;

                // ═══════════════════════════════════════════════════════════
                // 0xF0 - 0xFF
                // ═══════════════════════════════════════════════════════════

                case 0xF0:  // LDH A, (n) - read from 0xFF00 + n
                    cpu.A = memory::read(state, 0xFF00 + fetchByte(state));
                    cycles = 12;
                    break;

                case 0xF1:  // POP AF
                    cpu.F = memory::read(state, cpu.SP) & 0xF0;  // Lower 4 bits always 0
                    cpu.A = memory::read(state, cpu.SP + 1);
                    cpu.SP += 2;
                    cycles = 12;
                    break;

                case 0xF2:  // LD A, (C) - read from 0xFF00 + C
                    cpu.A = memory::read(state, 0xFF00 + cpu.C);
                    cycles = 8;
                    break;

                case 0xF3:  // DI
                    cpu.ime = false;
                    cycles = 4;
                    break;

                // 0xF4 doesn't exist on GB

                case 0xF5:  // PUSH AF
                    cpu.SP -= 2;
                    memory::write(state, cpu.SP, cpu.F);
                    memory::write(state, cpu.SP + 1, cpu.A);
                    cycles = 16;
                    break;

                case 0xF6:  // OR n
                    cpu.A |= fetchByte(state);
                    setFlag(state, FLAG_Z, cpu.A == 0);
                    setFlag(state, FLAG_N, false);
                    setFlag(state, FLAG_H, false);
                    setFlag(state, FLAG_C, false);
                    cycles = 8;
                    break;

                case 0xF7:  // RST 30H
                    cpu.SP -= 2;
                    memory::write(state, cpu.SP, cpu.PC & 0xFF);
                    memory::write(state, cpu.SP + 1, cpu.PC >> 8);
                    cpu.PC = 0x0030;
                    cycles = 16;
                    break;

                case 0xF8:  // LD HL, SP+n (signed)
                    {
                        int8_t offset = (int8_t)fetchByte(state);
                        uint16_t sp = cpu.SP;
                        setFlag(state, FLAG_Z, false);
                        setFlag(state, FLAG_N, false);
                        setFlag(state, FLAG_H, ((sp & 0x0F) + (offset & 0x0F)) > 0x0F);
                        setFlag(state, FLAG_C, ((sp & 0xFF) + (offset & 0xFF)) > 0xFF);
                        setHL(state, sp + offset);
                    }
                    cycles = 12;
                    break;

                case 0xF9:  // LD SP, HL
                    cpu.SP = getHL(state);
                    cycles = 8;
                    break;

                case 0xFA:  // LD A, (nn)
                    cpu.A = memory::read(state, fetchWord(state));
                    cycles = 16;
                    break;

                case 0xFB:  // EI
                    cpu.imeScheduled = true;  // Enable interrupts after next instruction
                    cycles = 4;
                    break;

                // 0xFC doesn't exist on GB
                // 0xFD doesn't exist on GB

                case 0xFE:  // CP n
                    {
                        uint8_t val = fetchByte(state);
                        setFlag(state, FLAG_Z, cpu.A == val);
                        setFlag(state, FLAG_N, true);
                        setFlag(state, FLAG_H, (cpu.A & 0x0F) < (val & 0x0F));
                        setFlag(state, FLAG_C, cpu.A < val);
                    }
                    cycles = 8;
                    break;

                case 0xFF:  // RST 38H
                    cpu.SP -= 2;
                    memory::write(state, cpu.SP, cpu.PC & 0xFF);
                    memory::write(state, cpu.SP + 1, cpu.PC >> 8);
                    cpu.PC = 0x0038;
                    cycles = 16;
                    break;

                default:
                    cycles = 4;
                    break;
            }

            return cycles;
        }

    }  // namespace cpu
}  // namespace gb