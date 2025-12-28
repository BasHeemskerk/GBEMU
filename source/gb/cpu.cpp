#include "included/cpu.hpp"
#include "included/state.hpp"
#include "included/memory.hpp"
#include "included/opcode_parser.hpp"

namespace gb {
    namespace cpu {

        // Helper: set all flags at once
        static inline void setFlags(GBState& state, bool z, bool n, bool h, bool c) {
            state.cpu.F = (z ? FLAG_Z : 0) | (n ? FLAG_N : 0) | 
                          (h ? FLAG_H : 0) | (c ? FLAG_C : 0);
        }

        // Helper: fetch byte and increment PC
        static inline uint8_t fetchByte(GBState& state) {
            return memory::read(state, state.cpu.PC++);
        }

        // Helper: fetch word and increment PC
        static inline uint16_t fetchWord(GBState& state) {
            uint8_t lo = memory::read(state, state.cpu.PC++);
            uint8_t hi = memory::read(state, state.cpu.PC++);
            return (hi << 8) | lo;
        }

        // Helper: push word to stack
        static inline void pushWord(GBState& state, uint16_t val) {
            state.cpu.SP -= 2;
            memory::write(state, state.cpu.SP, val & 0xFF);
            memory::write(state, state.cpu.SP + 1, val >> 8);
        }

        // Helper: pop word from stack
        static inline uint16_t popWord(GBState& state) {
            uint16_t val = memory::read(state, state.cpu.SP) |
                          (memory::read(state, state.cpu.SP + 1) << 8);
            state.cpu.SP += 2;
            return val;
        }

        // Get 8-bit value from operand
        static uint8_t getValue8(GBState& state, Operand op) {
            switch (op) {
                case Operand::A: return state.cpu.A;
                case Operand::B: return state.cpu.B;
                case Operand::C: return state.cpu.C;
                case Operand::D: return state.cpu.D;
                case Operand::E: return state.cpu.E;
                case Operand::H: return state.cpu.H;
                case Operand::L: return state.cpu.L;
                case Operand::MEM_BC: return memory::read(state, state.cpu.BC);
                case Operand::MEM_DE: return memory::read(state, state.cpu.DE);
                case Operand::MEM_HL: return memory::read(state, state.cpu.HL);
                case Operand::MEM_HL_INC: return memory::read(state, state.cpu.HL++);
                case Operand::MEM_HL_DEC: return memory::read(state, state.cpu.HL--);
                case Operand::MEM_NN: return memory::read(state, fetchWord(state));
                case Operand::MEM_FF_N: return memory::read(state, 0xFF00 + fetchByte(state));
                case Operand::MEM_FF_C: return memory::read(state, 0xFF00 + state.cpu.C);
                case Operand::IMM8: return fetchByte(state);
                default: return 0;
            }
        }

        // Set 8-bit value to operand
        static void setValue8(GBState& state, Operand op, uint8_t val) {
            switch (op) {
                case Operand::A: state.cpu.A = val; break;
                case Operand::B: state.cpu.B = val; break;
                case Operand::C: state.cpu.C = val; break;
                case Operand::D: state.cpu.D = val; break;
                case Operand::E: state.cpu.E = val; break;
                case Operand::H: state.cpu.H = val; break;
                case Operand::L: state.cpu.L = val; break;
                case Operand::MEM_BC: memory::write(state, state.cpu.BC, val); break;
                case Operand::MEM_DE: memory::write(state, state.cpu.DE, val); break;
                case Operand::MEM_HL: memory::write(state, state.cpu.HL, val); break;
                case Operand::MEM_HL_INC: memory::write(state, state.cpu.HL++, val); break;
                case Operand::MEM_HL_DEC: memory::write(state, state.cpu.HL--, val); break;
                case Operand::MEM_NN: memory::write(state, fetchWord(state), val); break;
                case Operand::MEM_FF_N: memory::write(state, 0xFF00 + fetchByte(state), val); break;
                case Operand::MEM_FF_C: memory::write(state, 0xFF00 + state.cpu.C, val); break;
                default: break;
            }
        }

        // Get 16-bit value from operand
        static uint16_t getValue16(GBState& state, Operand op) {
            switch (op) {
                case Operand::AF: return state.cpu.AF;
                case Operand::BC: return state.cpu.BC;
                case Operand::DE: return state.cpu.DE;
                case Operand::HL: return state.cpu.HL;
                case Operand::SP: return state.cpu.SP;
                case Operand::IMM16: return fetchWord(state);
                default: return 0;
            }
        }

        // Set 16-bit value to operand
        static void setValue16(GBState& state, Operand op, uint16_t val) {
            switch (op) {
                case Operand::AF: state.cpu.AF = val & 0xFFF0; break;
                case Operand::BC: state.cpu.BC = val; break;
                case Operand::DE: state.cpu.DE = val; break;
                case Operand::HL: state.cpu.HL = val; break;
                case Operand::SP: state.cpu.SP = val; break;
                default: break;
            }
        }

        // Get RST vector address
        static uint16_t getRSTVector(Operand op) {
            switch (op) {
                case Operand::RST_00: return 0x00;
                case Operand::RST_08: return 0x08;
                case Operand::RST_10: return 0x10;
                case Operand::RST_18: return 0x18;
                case Operand::RST_20: return 0x20;
                case Operand::RST_28: return 0x28;
                case Operand::RST_30: return 0x30;
                case Operand::RST_38: return 0x38;
                default: return 0x00;
            }
        }

        // Get bit index from operand
        static uint8_t getBitIndex(Operand op) {
            switch (op) {
                case Operand::BIT_0: return 0;
                case Operand::BIT_1: return 1;
                case Operand::BIT_2: return 2;
                case Operand::BIT_3: return 3;
                case Operand::BIT_4: return 4;
                case Operand::BIT_5: return 5;
                case Operand::BIT_6: return 6;
                case Operand::BIT_7: return 7;
                default: return 0;
            }
        }

        // Execute a single opcode entry
        static int executeOp(GBState& state, const OpcodeEntry& entry) {
            auto& cpu = state.cpu;
            int cycles = entry.cycles;

            switch (entry.op) {
                case MicroOp::NOP:
                    break;

                case MicroOp::LD8: {
                    uint8_t val = getValue8(state, entry.src);
                    setValue8(state, entry.dst, val);
                    break;
                }

                case MicroOp::ST8: {
                    uint8_t val = getValue8(state, entry.src);
                    setValue8(state, entry.dst, val);
                    break;
                }

                case MicroOp::LD16: {
                    uint16_t val = getValue16(state, entry.src);
                    setValue16(state, entry.dst, val);
                    break;
                }

                case MicroOp::ST16: {
                    uint16_t addr = fetchWord(state);
                    memory::write(state, addr, cpu.SP & 0xFF);
                    memory::write(state, addr + 1, cpu.SP >> 8);
                    break;
                }

                case MicroOp::ADD8: {
                    uint8_t val = getValue8(state, entry.src);
                    int result = cpu.A + val;
                    setFlags(state, (result & 0xFF) == 0, false,
                            ((cpu.A & 0x0F) + (val & 0x0F)) > 0x0F, result > 0xFF);
                    cpu.A = result & 0xFF;
                    break;
                }

                case MicroOp::ADC8: {
                    uint8_t val = getValue8(state, entry.src);
                    int carry = (cpu.F & FLAG_C) ? 1 : 0;
                    int result = cpu.A + val + carry;
                    setFlags(state, (result & 0xFF) == 0, false,
                            ((cpu.A & 0x0F) + (val & 0x0F) + carry) > 0x0F, result > 0xFF);
                    cpu.A = result & 0xFF;
                    break;
                }

                case MicroOp::SUB8: {
                    uint8_t val = getValue8(state, entry.src);
                    int result = cpu.A - val;
                    setFlags(state, (result & 0xFF) == 0, true,
                            (cpu.A & 0x0F) < (val & 0x0F), cpu.A < val);
                    cpu.A = result & 0xFF;
                    break;
                }

                case MicroOp::SBC8: {
                    uint8_t val = getValue8(state, entry.src);
                    int carry = (cpu.F & FLAG_C) ? 1 : 0;
                    int result = cpu.A - val - carry;
                    setFlags(state, (result & 0xFF) == 0, true,
                            ((cpu.A & 0x0F) - (val & 0x0F) - carry) < 0, result < 0);
                    cpu.A = result & 0xFF;
                    break;
                }

                case MicroOp::INC8: {
                    uint8_t val = getValue8(state, entry.dst);
                    uint8_t result = val + 1;
                    cpu.F = (cpu.F & FLAG_C) | (result == 0 ? FLAG_Z : 0) |
                            ((val & 0x0F) == 0x0F ? FLAG_H : 0);
                    setValue8(state, entry.dst, result);
                    break;
                }

                case MicroOp::DEC8: {
                    uint8_t val = getValue8(state, entry.dst);
                    uint8_t result = val - 1;
                    cpu.F = (cpu.F & FLAG_C) | (result == 0 ? FLAG_Z : 0) | FLAG_N |
                            ((val & 0x0F) == 0x00 ? FLAG_H : 0);
                    setValue8(state, entry.dst, result);
                    break;
                }

                case MicroOp::AND8: {
                    cpu.A &= getValue8(state, entry.src);
                    setFlags(state, cpu.A == 0, false, true, false);
                    break;
                }

                case MicroOp::OR8: {
                    cpu.A |= getValue8(state, entry.src);
                    setFlags(state, cpu.A == 0, false, false, false);
                    break;
                }

                case MicroOp::XOR8: {
                    cpu.A ^= getValue8(state, entry.src);
                    setFlags(state, cpu.A == 0, false, false, false);
                    break;
                }

                case MicroOp::CP8: {
                    uint8_t val = getValue8(state, entry.src);
                    setFlags(state, cpu.A == val, true,
                            (cpu.A & 0x0F) < (val & 0x0F), cpu.A < val);
                    break;
                }

                case MicroOp::ADD16: {
                    uint16_t val = getValue16(state, entry.src);
                    uint32_t result = cpu.HL + val;
                    cpu.F = (cpu.F & FLAG_Z) |
                            (((cpu.HL & 0x0FFF) + (val & 0x0FFF)) > 0x0FFF ? FLAG_H : 0) |
                            (result > 0xFFFF ? FLAG_C : 0);
                    cpu.HL = result & 0xFFFF;
                    break;
                }

                case MicroOp::INC16: {
                    setValue16(state, entry.dst, getValue16(state, entry.dst) + 1);
                    break;
                }

                case MicroOp::DEC16: {
                    setValue16(state, entry.dst, getValue16(state, entry.dst) - 1);
                    break;
                }

                case MicroOp::ADDSP: {
                    int8_t offset = (int8_t)fetchByte(state);
                    uint16_t sp = cpu.SP;
                    setFlags(state, false, false,
                            ((sp & 0x0F) + (offset & 0x0F)) > 0x0F,
                            ((sp & 0xFF) + (offset & 0xFF)) > 0xFF);
                    cpu.SP = sp + offset;
                    break;
                }

                case MicroOp::LD_HL_SP_E: {
                    int8_t offset = (int8_t)fetchByte(state);
                    uint16_t sp = cpu.SP;
                    setFlags(state, false, false,
                            ((sp & 0x0F) + (offset & 0x0F)) > 0x0F,
                            ((sp & 0xFF) + (offset & 0xFF)) > 0xFF);
                    cpu.HL = sp + offset;
                    break;
                }

                case MicroOp::RLCA: {
                    uint8_t bit7 = cpu.A >> 7;
                    cpu.A = (cpu.A << 1) | bit7;
                    setFlags(state, false, false, false, bit7);
                    break;
                }

                case MicroOp::RRCA: {
                    uint8_t bit0 = cpu.A & 1;
                    cpu.A = (cpu.A >> 1) | (bit0 << 7);
                    setFlags(state, false, false, false, bit0);
                    break;
                }

                case MicroOp::RLA: {
                    uint8_t carry = (cpu.F & FLAG_C) ? 1 : 0;
                    uint8_t bit7 = cpu.A >> 7;
                    cpu.A = (cpu.A << 1) | carry;
                    setFlags(state, false, false, false, bit7);
                    break;
                }

                case MicroOp::RRA: {
                    uint8_t carry = (cpu.F & FLAG_C) ? 0x80 : 0;
                    uint8_t bit0 = cpu.A & 1;
                    cpu.A = (cpu.A >> 1) | carry;
                    setFlags(state, false, false, false, bit0);
                    break;
                }

                case MicroOp::RLC: {
                    uint8_t val = getValue8(state, entry.dst);
                    uint8_t result = (val << 1) | (val >> 7);
                    setFlags(state, result == 0, false, false, val & 0x80);
                    setValue8(state, entry.dst, result);
                    break;
                }

                case MicroOp::RRC: {
                    uint8_t val = getValue8(state, entry.dst);
                    uint8_t result = (val >> 1) | (val << 7);
                    setFlags(state, result == 0, false, false, val & 0x01);
                    setValue8(state, entry.dst, result);
                    break;
                }

                case MicroOp::RL: {
                    uint8_t val = getValue8(state, entry.dst);
                    uint8_t carry = (cpu.F & FLAG_C) ? 1 : 0;
                    uint8_t result = (val << 1) | carry;
                    setFlags(state, result == 0, false, false, val & 0x80);
                    setValue8(state, entry.dst, result);
                    break;
                }

                case MicroOp::RR: {
                    uint8_t val = getValue8(state, entry.dst);
                    uint8_t carry = (cpu.F & FLAG_C) ? 0x80 : 0;
                    uint8_t result = (val >> 1) | carry;
                    setFlags(state, result == 0, false, false, val & 0x01);
                    setValue8(state, entry.dst, result);
                    break;
                }

                case MicroOp::SLA: {
                    uint8_t val = getValue8(state, entry.dst);
                    uint8_t result = val << 1;
                    setFlags(state, result == 0, false, false, val & 0x80);
                    setValue8(state, entry.dst, result);
                    break;
                }

                case MicroOp::SRA: {
                    uint8_t val = getValue8(state, entry.dst);
                    uint8_t result = (val >> 1) | (val & 0x80);
                    setFlags(state, result == 0, false, false, val & 0x01);
                    setValue8(state, entry.dst, result);
                    break;
                }

                case MicroOp::SRL: {
                    uint8_t val = getValue8(state, entry.dst);
                    uint8_t result = val >> 1;
                    setFlags(state, result == 0, false, false, val & 0x01);
                    setValue8(state, entry.dst, result);
                    break;
                }

                case MicroOp::SWAP: {
                    uint8_t val = getValue8(state, entry.dst);
                    uint8_t result = ((val & 0x0F) << 4) | ((val & 0xF0) >> 4);
                    setFlags(state, result == 0, false, false, false);
                    setValue8(state, entry.dst, result);
                    break;
                }

                case MicroOp::BIT: {
                    uint8_t val = getValue8(state, entry.src);
                    uint8_t bit = getBitIndex(entry.dst);
                    cpu.F = (cpu.F & FLAG_C) | FLAG_H | (!(val & (1 << bit)) ? FLAG_Z : 0);
                    break;
                }

                case MicroOp::RES: {
                    uint8_t val = getValue8(state, entry.src);
                    uint8_t bit = getBitIndex(entry.dst);
                    setValue8(state, entry.src, val & ~(1 << bit));
                    break;
                }

                case MicroOp::SET: {
                    uint8_t val = getValue8(state, entry.src);
                    uint8_t bit = getBitIndex(entry.dst);
                    setValue8(state, entry.src, val | (1 << bit));
                    break;
                }

                case MicroOp::JP: {
                    cpu.PC = fetchWord(state);
                    break;
                }

                case MicroOp::JP_Z: {
                    uint16_t addr = fetchWord(state);
                    if (cpu.F & FLAG_Z) {
                        cpu.PC = addr;
                    } else {
                        cycles = entry.cyclesBranch;
                    }
                    break;
                }

                case MicroOp::JP_NZ: {
                    uint16_t addr = fetchWord(state);
                    if (!(cpu.F & FLAG_Z)) {
                        cpu.PC = addr;
                    } else {
                        cycles = entry.cyclesBranch;
                    }
                    break;
                }

                case MicroOp::JP_C: {
                    uint16_t addr = fetchWord(state);
                    if (cpu.F & FLAG_C) {
                        cpu.PC = addr;
                    } else {
                        cycles = entry.cyclesBranch;
                    }
                    break;
                }

                case MicroOp::JP_NC: {
                    uint16_t addr = fetchWord(state);
                    if (!(cpu.F & FLAG_C)) {
                        cpu.PC = addr;
                    } else {
                        cycles = entry.cyclesBranch;
                    }
                    break;
                }

                case MicroOp::JP_HL: {
                    cpu.PC = cpu.HL;
                    break;
                }

                case MicroOp::JR: {
                    int8_t offset = (int8_t)fetchByte(state);
                    cpu.PC += offset;
                    break;
                }

                case MicroOp::JR_Z: {
                    int8_t offset = (int8_t)fetchByte(state);
                    if (cpu.F & FLAG_Z) {
                        cpu.PC += offset;
                    } else {
                        cycles = entry.cyclesBranch;
                    }
                    break;
                }

                case MicroOp::JR_NZ: {
                    int8_t offset = (int8_t)fetchByte(state);
                    if (!(cpu.F & FLAG_Z)) {
                        cpu.PC += offset;
                    } else {
                        cycles = entry.cyclesBranch;
                    }
                    break;
                }

                case MicroOp::JR_C: {
                    int8_t offset = (int8_t)fetchByte(state);
                    if (cpu.F & FLAG_C) {
                        cpu.PC += offset;
                    } else {
                        cycles = entry.cyclesBranch;
                    }
                    break;
                }

                case MicroOp::JR_NC: {
                    int8_t offset = (int8_t)fetchByte(state);
                    if (!(cpu.F & FLAG_C)) {
                        cpu.PC += offset;
                    } else {
                        cycles = entry.cyclesBranch;
                    }
                    break;
                }

                case MicroOp::CALL: {
                    uint16_t addr = fetchWord(state);
                    pushWord(state, cpu.PC);
                    cpu.PC = addr;
                    break;
                }

                case MicroOp::CALL_Z: {
                    uint16_t addr = fetchWord(state);
                    if (cpu.F & FLAG_Z) {
                        pushWord(state, cpu.PC);
                        cpu.PC = addr;
                    } else {
                        cycles = entry.cyclesBranch;
                    }
                    break;
                }

                case MicroOp::CALL_NZ: {
                    uint16_t addr = fetchWord(state);
                    if (!(cpu.F & FLAG_Z)) {
                        pushWord(state, cpu.PC);
                        cpu.PC = addr;
                    } else {
                        cycles = entry.cyclesBranch;
                    }
                    break;
                }

                case MicroOp::CALL_C: {
                    uint16_t addr = fetchWord(state);
                    if (cpu.F & FLAG_C) {
                        pushWord(state, cpu.PC);
                        cpu.PC = addr;
                    } else {
                        cycles = entry.cyclesBranch;
                    }
                    break;
                }

                case MicroOp::CALL_NC: {
                    uint16_t addr = fetchWord(state);
                    if (!(cpu.F & FLAG_C)) {
                        pushWord(state, cpu.PC);
                        cpu.PC = addr;
                    } else {
                        cycles = entry.cyclesBranch;
                    }
                    break;
                }

                case MicroOp::RET: {
                    cpu.PC = popWord(state);
                    break;
                }

                case MicroOp::RET_Z: {
                    if (cpu.F & FLAG_Z) {
                        cpu.PC = popWord(state);
                    } else {
                        cycles = entry.cyclesBranch;
                    }
                    break;
                }

                case MicroOp::RET_NZ: {
                    if (!(cpu.F & FLAG_Z)) {
                        cpu.PC = popWord(state);
                    } else {
                        cycles = entry.cyclesBranch;
                    }
                    break;
                }

                case MicroOp::RET_C: {
                    if (cpu.F & FLAG_C) {
                        cpu.PC = popWord(state);
                    } else {
                        cycles = entry.cyclesBranch;
                    }
                    break;
                }

                case MicroOp::RET_NC: {
                    if (!(cpu.F & FLAG_C)) {
                        cpu.PC = popWord(state);
                    } else {
                        cycles = entry.cyclesBranch;
                    }
                    break;
                }

                case MicroOp::RETI: {
                    cpu.PC = popWord(state);
                    cpu.ime = true;
                    break;
                }

                case MicroOp::RST: {
                    pushWord(state, cpu.PC);
                    cpu.PC = getRSTVector(entry.dst);
                    break;
                }

                case MicroOp::PUSH: {
                    pushWord(state, getValue16(state, entry.dst));
                    break;
                }

                case MicroOp::POP: {
                    setValue16(state, entry.dst, popWord(state));
                    break;
                }

                case MicroOp::HALT: {
                    cpu.halted = true;
                    break;
                }

                case MicroOp::STOP: {
                    fetchByte(state);
                    break;
                }

                case MicroOp::DI: {
                    cpu.ime = false;
                    break;
                }

                case MicroOp::EI: {
                    cpu.imeScheduled = true;
                    break;
                }

                case MicroOp::DAA: {
                    int a = cpu.A;
                    if (!(cpu.F & FLAG_N)) {
                        if ((cpu.F & FLAG_H) || (a & 0x0F) > 9) a += 0x06;
                        if ((cpu.F & FLAG_C) || a > 0x9F) a += 0x60;
                    } else {
                        if (cpu.F & FLAG_H) a = (a - 6) & 0xFF;
                        if (cpu.F & FLAG_C) a -= 0x60;
                    }
                    cpu.F &= ~(FLAG_Z | FLAG_H);
                    if (a & 0x100) cpu.F |= FLAG_C;
                    cpu.A = a & 0xFF;
                    if (cpu.A == 0) cpu.F |= FLAG_Z;
                    break;
                }

                case MicroOp::CPL: {
                    cpu.A = ~cpu.A;
                    cpu.F |= FLAG_N | FLAG_H;
                    break;
                }

                case MicroOp::CCF: {
                    cpu.F = (cpu.F & FLAG_Z) | ((cpu.F & FLAG_C) ? 0 : FLAG_C);
                    break;
                }

                case MicroOp::SCF: {
                    cpu.F = (cpu.F & FLAG_Z) | FLAG_C;
                    break;
                }

                case MicroOp::CB: {
                    uint8_t cbOpcode = fetchByte(state);
                    const OpcodeEntry& cbEntry = state.opcodes.cb[cbOpcode];
                    return executeOp(state, cbEntry);
                }

                default:
                    break;
            }

            return cycles;
        }

        void initialize(GBState& state) {
            auto& cpu = state.cpu;

            cpu.AF = 0x01B0;
            cpu.BC = 0x0013;
            cpu.DE = 0x00D8;
            cpu.HL = 0x014D;

            cpu.SP = 0xFFFE;
            cpu.PC = 0x0100;

            cpu.halted = false;
            cpu.ime = false;
            cpu.imeScheduled = false;
        }

        int step(GBState& state) {
            auto& cpu = state.cpu;

            if (cpu.imeScheduled) {
                cpu.ime = true;
                cpu.imeScheduled = false;
            }

            if (cpu.halted) {
                return 4;
            }

            uint8_t opcode = fetchByte(state);
            const OpcodeEntry& entry = state.opcodes.main[opcode];
            
            return executeOp(state, entry);
        }

    }
}