#include "included/cpu.hpp"
#include "included/memory.hpp"

namespace gb{
    namespace cpu{
        //define all the registers
        uint8_t A;
        uint8_t F;
        uint8_t B;
        uint8_t C;
        uint8_t D;
        uint8_t E;
        uint8_t H;
        uint8_t L;

        uint16_t SP;
        uint16_t PC;

        bool halted;
        bool stopped;
        bool ime;
        bool imeScheduled;

        int cycles;

        void initialize(){
            //these are the values after the boot rom runs
            //we will skip the boot rom so set them directly
            A = 0x01;
            F = 0xB0;
            B = 0x00;
            C = 0x13;
            D = 0x00;
            E = 0xD8;
            H = 0x01;
            L = 0x4D;

            SP = 0xFFFE;
            PC = 0x0100; //game code starts from here

            halted = false;
            stopped = false;
            ime = false;
            imeScheduled = false;

            cycles = 0;
        }

        //helper function to read byte at PC and advance
        uint8_t fetchByte(){
            uint8_t val = memory::read(PC);
            PC++;
            return val;
        }

        //helper function to read 16bit value at PC (in little endian) and advance
        uint16_t fetchWord(){
            uint8_t low = memory::read(PC);
            PC++;
            uint8_t high = memory::read(PC);
            PC++;
            return (high << 8) | low;
        }

        //helper function to push 16bit value onto the stack
        void pushStack(uint16_t val){
            SP--;
            memory::write(SP, val >> 8); //high byte
            SP--;
            memory::write(SP, val & 0xFF); //low byte
        }

        //helper function to pop 16 bit value from the stack
        uint16_t popStack(){
            uint8_t low = memory::read(SP);
            SP++;
            uint8_t high = memory::read(SP);
            SP++;
            return (high << 8) | low;
        }

        //cb prefixed opcodes for bit operation
        int executeCB(uint8_t opcode){
            // CB opcodes follow a pattern:
            // 0x00-0x07: RLC (rotate left)
            // 0x08-0x0F: RRC (rotate right)
            // 0x10-0x17: RL (rotate left through carry)
            // 0x18-0x1F: RR (rotate right through carry)
            // 0x20-0x27: SLA (shift left arithmetic)
            // 0x28-0x2F: SRA (shift right arithmetic)
            // 0x30-0x37: SWAP (swap nibbles)
            // 0x38-0x3F: SRL (shift right logical)
            // 0x40-0x7F: BIT (test bit)
            // 0x80-0xBF: RES (reset bit)
            // 0xC0-0xFF: SET (set bit)
            //
            // Last 3 bits determine register:
            // 0=B, 1=C, 2=D, 3=E, 4=H, 5=L, 6=(HL), 7=A


            //get the target registers value
            uint8_t* reg = nullptr;
            uint8_t hlVal = 0;
            int regIndex = opcode & 0x07;
            bool isHL = (regIndex == 6);

            switch (regIndex){
                case 0: reg = &B; break;
                case 1: reg = &C; break;
                case 2: reg = &D; break;
                case 3: reg = &E; break;
                case 4: reg = &H; break;
                case 5: reg = &L; break;
                case 6: hlVal = memory::read(getHL()); break;
                case 7: reg = &A; break;
            }

            uint8_t val = isHL ? hlVal : *reg;
            uint8_t result = val;
            int cycles = isHL ? 16 : 8;

            uint8_t op = opcode >> 3;

            if (opcode < 0x40){
                switch (op){
                    case 0:  // RLC - rotate left, old bit 7 to carry
                        {
                            uint8_t bit7 = (val >> 7) & 1;
                            result = (val << 1) | bit7;
                            setFlag(FLAG_Z, result == 0);
                            setFlag(FLAG_N, false);
                            setFlag(FLAG_H, false);
                            setFlag(FLAG_C, bit7);
                        }
                        break;

                    case 1:  // RRC - rotate right, old bit 0 to carry
                        {
                            uint8_t bit0 = val & 1;
                            result = (val >> 1) | (bit0 << 7);
                            setFlag(FLAG_Z, result == 0);
                            setFlag(FLAG_N, false);
                            setFlag(FLAG_H, false);
                            setFlag(FLAG_C, bit0);
                        }
                        break;

                    case 2:  // RL - rotate left through carry
                        {
                            uint8_t bit7 = (val >> 7) & 1;
                            result = (val << 1) | (getFlag(FLAG_C) ? 1 : 0);
                            setFlag(FLAG_Z, result == 0);
                            setFlag(FLAG_N, false);
                            setFlag(FLAG_H, false);
                            setFlag(FLAG_C, bit7);
                        }
                        break;

                    case 3:  // RR - rotate right through carry
                        {
                            uint8_t bit0 = val & 1;
                            result = (val >> 1) | (getFlag(FLAG_C) ? 0x80 : 0);
                            setFlag(FLAG_Z, result == 0);
                            setFlag(FLAG_N, false);
                            setFlag(FLAG_H, false);
                            setFlag(FLAG_C, bit0);
                        }
                        break;

                    case 4:  // SLA - shift left arithmetic (bit 0 = 0)
                        {
                            uint8_t bit7 = (val >> 7) & 1;
                            result = val << 1;
                            setFlag(FLAG_Z, result == 0);
                            setFlag(FLAG_N, false);
                            setFlag(FLAG_H, false);
                            setFlag(FLAG_C, bit7);
                        }
                        break;

                    case 5:  // SRA - shift right arithmetic (bit 7 stays same)
                        {
                            uint8_t bit0 = val & 1;
                            uint8_t bit7 = val & 0x80;
                            result = (val >> 1) | bit7;
                            setFlag(FLAG_Z, result == 0);
                            setFlag(FLAG_N, false);
                            setFlag(FLAG_H, false);
                            setFlag(FLAG_C, bit0);
                        }
                        break;

                    case 6:  // SWAP - swap upper and lower nibbles
                        result = ((val & 0x0F) << 4) | ((val & 0xF0) >> 4);
                        setFlag(FLAG_Z, result == 0);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, false);
                        setFlag(FLAG_C, false);
                        break;

                    case 7:  // SRL - shift right logical (bit 7 = 0)
                        {
                            uint8_t bit0 = val & 1;
                            result = val >> 1;
                            setFlag(FLAG_Z, result == 0);
                            setFlag(FLAG_N, false);
                            setFlag(FLAG_H, false);
                            setFlag(FLAG_C, bit0);
                        }
                        break;
                }

                if (isHL){
                    memory::write(getHL(), result);
                } else {
                    *reg = result;
                }
            }
            else if (opcode < 0x80) {
                // BIT - test bit
                int bit = (opcode >> 3) & 0x07;
                setFlag(FLAG_Z, (val & (1 << bit)) == 0);
                setFlag(FLAG_N, false);
                setFlag(FLAG_H, true);
                // carry not affected
                cycles = isHL ? 12 : 8;  // BIT (HL) is 12 cycles, not 16
            }
            else if (opcode < 0xC0) {
                // RES - reset (clear) bit
                int bit = (opcode >> 3) & 0x07;
                result = val & ~(1 << bit);

                if (isHL) {
                    memory::write(getHL(), result);
                } else {
                    *reg = result;
                }
            }
            else {
                // SET - set bit
                int bit = (opcode >> 3) & 0x07;
                result = val | (1 << bit);

                if (isHL) {
                    memory::write(getHL(), result);
                } else {
                    *reg = result;
                }
            }

            return cycles;
        }

        int step(){
            cycles = 0;

            //handle scheduled ime enable
            if (imeScheduled){
                ime = true;
                imeScheduled = false;
            }

            //if halted burn 4 cycles
            if (halted){
                cycles = 4;
                return cycles;
            }

            //fetch opcode
            uint8_t opcode = fetchByte();

            //decode opcodes and execute
            switch (opcode){
                // 0x00 - 0x0F
                case 0x00: //NOP, do nothing
                    cycles = 4;
                    break;

                case 0x01: //LD BC, nn - load 16-bit immediate into BC
                    setBC(fetchWord());
                    cycles = 12;
                    break;

                case 0x02: //LD (BC), A - Store A at BC address
                    memory::write(getBC(), A);
                    cycles = 8;
                    break;
                
                case 0x03: //increment BC
                    setBC(getBC() + 1);
                    cycles = 8;
                    break;

                case 0x04: //increment B
                    B++;
                    setFlag(FLAG_Z, B == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, (B & 0x0F) == 0x00); //half carry if low nibble wrapped
                    cycles = 4;
                    break;

                case 0x05: //decrement B
                    B--;
                    setFlag(FLAG_Z, B == 0);
                    setFlag(FLAG_N, true);
                    setFlag(FLAG_H, (B & 0x0F) == 0x0F);  // half borrow if low nibble wrapped
                    cycles = 4;
                    break;

                case 0x06: //load immediate into B
                    B = fetchByte();
                    cycles = 8;
                    break;

                case 0x07: //rotate A left, old bit 7 to carry
                    {
                        uint8_t bit7 = (A >> 7) & 1;
                        A = (A << 1) | bit7;
                        setFlag(FLAG_Z, false);
                        setFlag(FLAG_H, false);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_C, bit7);
                    }
                    cycles = 4;
                    break;

                case 0x08: //store SP at address
                    {
                        uint16_t address = fetchWord();
                        memory::write(address, SP & 0xFF);
                        memory::write(address + 1, SP >> 8);
                    }
                    cycles = 20;
                    break;

                case 0x09: //add BC to HL
                    {
                        uint32_t result = getHL() + getBC();
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, ((getHL() & 0x0FFF) + (getBC() & 0x0FFF)) > 0x0FFF);
                        setFlag(FLAG_C, result > 0xFFFF);
                        setHL(result & 0xFFFF);
                    }
                    cycles = 8;
                    break;

                case 0x0A: //load from address BC into A
                    A = memory::read(getBC());
                    cycles = 8;
                    break;

                case 0x0B: //decrement BC
                    setBC(getBC() - 1);
                    cycles = 8;
                    break;

                case 0x0C: // increment C
                    C++;
                    setFlag(FLAG_Z, C == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, (C & 0x0F) == 0x00);
                    cycles = 4;
                    break;

                case 0x0D: //decrement C
                    C--;
                    setFlag(FLAG_Z, C == 0);
                    setFlag(FLAG_N, true);
                    setFlag(FLAG_H, (C & 0x0F) == 0x0F);
                    cycles = 4;
                    break;

                case 0x0E: //load immediate into C
                    C = fetchByte();
                    cycles = 8;
                    break;

                case 0x0F: // rotate A right, old bit 0 to carry
                    {
                        uint8_t bit0 = A & 1;
                        A = (A >> 1) | (bit0 << 7);
                        setFlag(FLAG_Z, false);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, false);
                        setFlag(FLAG_C, bit0);
                    }
                    cycles = 4;
                    break;


                //0x10 - 0x1F

                case 0x10: //STOP
                    stopped = true;
                    fetchByte();
                    cycles = 4;
                    break;

                case 0x11: //LD DE, nn
                    setDE(fetchWord());
                    cycles = 12;
                    break;

                case 0x12: //LD (DE), A
                    memory::write(getDE(), A);
                    cycles = 8;
                    break;

                case 0x13: //increment DE
                    setDE(getDE() + 1);
                    cycles = 8;
                    break;

                case 0x14: // increment D
                    D++;
                    setFlag(FLAG_Z, D == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, (D & 0x0F) == 0x00);
                    cycles = 4;
                    break;

                case 0x15: //decrement D
                    D--;
                    setFlag(FLAG_Z, D == 0);
                    setFlag(FLAG_N, true);
                    setFlag(FLAG_H, (D & 0x0F) == 0x0F);
                    cycles = 4;
                    break;

                case 0x16: //LD D, n
                    D = fetchByte();
                    cycles = 8;
                    break;

                case 0x17: //rotate A left through carry
                    {
                        uint8_t bit7 = (A >> 7) & 1;
                        A = (A << 1) | (getFlag(FLAG_C) ? 1 : 0);
                        setFlag(FLAG_Z, false);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, false);
                        setFlag(FLAG_C, bit7);
                    }
                    cycles = 4;
                    break;

                case 0x18: //relative jump
                    {
                        int8_t offset = (int8_t)fetchByte();
                        PC += offset;
                    }
                    cycles = 12;
                    break;

                case 0x19: //add HL to DE
                    {
                        uint32_t result = getHL() + getDE();
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, ((getHL() & 0x0FFF) + (getDE() & 0x0FFF)) > 0x0FFF);
                        setFlag(FLAG_C, result > 0xFFFF);
                        setHL(result & 0xFFFF);
                    }
                    cycles = 8;
                    break;

                case 0x1A: //LD A, (DE)
                    A = memory::read(getDE());
                    cycles = 8;
                    break;

                case 0x1B: //decrement DE
                    setDE(getDE() - 1);
                    cycles = 8;
                    break;

                case 0x1C:  // increment E
                    E++;
                    setFlag(FLAG_Z, E == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, (E & 0x0F) == 0x00);
                    cycles = 4;
                    break;

                case 0x1D:  // decrement E
                    E--;
                    setFlag(FLAG_Z, E == 0);
                    setFlag(FLAG_N, true);
                    setFlag(FLAG_H, (E & 0x0F) == 0x0F);
                    cycles = 4;
                    break;

                case 0x1E: // LD E, n
                    E = fetchByte();
                    cycles = 8;
                    break;

                case 0x1F:  // RRA - rotate A right through carry
                    {
                        uint8_t bit0 = A & 1;
                        A = (A >> 1) | (getFlag(FLAG_C) ? 0x80 : 0);
                        setFlag(FLAG_Z, false);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, false);
                        setFlag(FLAG_C, bit0);
                    }
                    cycles = 4;
                    break;

                // 0x20 - 0x2F

                case 0x20:  // JR NZ, n - jump relative if not zero
                    {
                        int8_t offset = (int8_t)fetchByte();
                        if (!getFlag(FLAG_Z)) {
                            PC += offset;
                            cycles = 12;
                        } else {
                            cycles = 8;
                        }
                    }
                    break;

                case 0x21:  // LD HL, nn
                    setHL(fetchWord());
                    cycles = 12;
                    break;

                case 0x22:  // LD (HL+), A - store A at HL, then increment HL
                    memory::write(getHL(), A);
                    setHL(getHL() + 1);
                    cycles = 8;
                    break;

                case 0x23:  // INC HL
                    setHL(getHL() + 1);
                    cycles = 8;
                    break;

                case 0x24:  // INC H
                    H++;
                    setFlag(FLAG_Z, H == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, (H & 0x0F) == 0x00);
                    cycles = 4;
                    break;

                case 0x25:  // DEC H
                    H--;
                    setFlag(FLAG_Z, H == 0);
                    setFlag(FLAG_N, true);
                    setFlag(FLAG_H, (H & 0x0F) == 0x0F);
                    cycles = 4;
                    break;

                case 0x26:  // LD H, n
                    H = fetchByte();
                    cycles = 8;
                    break;

                case 0x27:  // DAA - decimal adjust accumulator
                    {
                        // used for BCD (binary coded decimal) arithmetic
                        // adjusts A after add/sub to keep it as valid BCD
                        int a = A;
                        if (!getFlag(FLAG_N)) {
                            if (getFlag(FLAG_H) || (a & 0x0F) > 9) {
                                a += 0x06;
                            }
                            if (getFlag(FLAG_C) || a > 0x9F) {
                                a += 0x60;
                                setFlag(FLAG_C, true);
                            }
                        } else {
                            if (getFlag(FLAG_H)) {
                                a -= 0x06;
                            }
                            if (getFlag(FLAG_C)) {
                                a -= 0x60;
                            }
                        }
                        A = a & 0xFF;
                        setFlag(FLAG_Z, A == 0);
                        setFlag(FLAG_H, false);
                    }
                    cycles = 4;
                    break;

                case 0x28:  // JR Z, n - jump relative if zero
                    {
                        int8_t offset = (int8_t)fetchByte();
                        if (getFlag(FLAG_Z)) {
                            PC += offset;
                            cycles = 12;
                        } else {
                            cycles = 8;
                        }
                    }
                    break;

                case 0x29:  // ADD HL, HL
                    {
                        uint32_t result = getHL() + getHL();
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, ((getHL() & 0x0FFF) + (getHL() & 0x0FFF)) > 0x0FFF);
                        setFlag(FLAG_C, result > 0xFFFF);
                        setHL(result & 0xFFFF);
                    }
                    cycles = 8;
                    break;

                case 0x2A:  // LD A, (HL+) - load from HL into A, then increment HL
                    A = memory::read(getHL());
                    setHL(getHL() + 1);
                    cycles = 8;
                    break;

                case 0x2B:  // DEC HL
                    setHL(getHL() - 1);
                    cycles = 8;
                    break;

                case 0x2C:  // INC L
                    L++;
                    setFlag(FLAG_Z, L == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, (L & 0x0F) == 0x00);
                    cycles = 4;
                    break;

                case 0x2D:  // DEC L
                    L--;
                    setFlag(FLAG_Z, L == 0);
                    setFlag(FLAG_N, true);
                    setFlag(FLAG_H, (L & 0x0F) == 0x0F);
                    cycles = 4;
                    break;

                case 0x2E:  // LD L, n
                    L = fetchByte();
                    cycles = 8;
                    break;

                case 0x2F:  // CPL - complement A (flip all bits)
                    A = ~A;
                    setFlag(FLAG_N, true);
                    setFlag(FLAG_H, true);
                    cycles = 4;
                    break;

                // 0x30 - 0x3F

                case 0x30:  // JR NC, n - jump relative if no carry
                    {
                        int8_t offset = (int8_t)fetchByte();
                        if (!getFlag(FLAG_C)) {
                            PC += offset;
                            cycles = 12;
                        } else {
                            cycles = 8;
                        }
                    }
                    break;

                case 0x31:  // LD SP, nn
                    SP = fetchWord();
                    cycles = 12;
                    break;

                case 0x32:  // LD (HL-), A - store A at HL, then decrement HL
                    memory::write(getHL(), A);
                    setHL(getHL() - 1);
                    cycles = 8;
                    break;

                case 0x33:  // INC SP
                    SP++;
                    cycles = 8;
                    break;

                case 0x34:  // INC (HL) - increment value at address HL
                    {
                        uint8_t val = memory::read(getHL());
                        val++;
                        memory::write(getHL(), val);
                        setFlag(FLAG_Z, val == 0);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, (val & 0x0F) == 0x00);
                    }
                    cycles = 12;
                    break;

                case 0x35:  // DEC (HL) - decrement value at address HL
                    {
                        uint8_t val = memory::read(getHL());
                        val--;
                        memory::write(getHL(), val);
                        setFlag(FLAG_Z, val == 0);
                        setFlag(FLAG_N, true);
                        setFlag(FLAG_H, (val & 0x0F) == 0x0F);
                    }
                    cycles = 12;
                    break;

                case 0x36:  // LD (HL), n - store immediate at address HL
                    memory::write(getHL(), fetchByte());
                    cycles = 12;
                    break;

                case 0x37:  // SCF - set carry flag
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, false);
                    setFlag(FLAG_C, true);
                    cycles = 4;
                    break;

                case 0x38:  // JR C, n - jump relative if carry
                    {
                        int8_t offset = (int8_t)fetchByte();
                        if (getFlag(FLAG_C)) {
                            PC += offset;
                            cycles = 12;
                        } else {
                            cycles = 8;
                        }
                    }
                    break;

                case 0x39:  // ADD HL, SP
                    {
                        uint32_t result = getHL() + SP;
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, ((getHL() & 0x0FFF) + (SP & 0x0FFF)) > 0x0FFF);
                        setFlag(FLAG_C, result > 0xFFFF);
                        setHL(result & 0xFFFF);
                    }
                    cycles = 8;
                    break;

                case 0x3A:  // LD A, (HL-) - load from HL into A, then decrement HL
                    A = memory::read(getHL());
                    setHL(getHL() - 1);
                    cycles = 8;
                    break;

                case 0x3B:  // DEC SP
                    SP--;
                    cycles = 8;
                    break;

                case 0x3C:  // INC A
                    A++;
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, (A & 0x0F) == 0x00);
                    cycles = 4;
                    break;

                case 0x3D:  // DEC A
                    A--;
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, true);
                    setFlag(FLAG_H, (A & 0x0F) == 0x0F);
                    cycles = 4;
                    break;

                case 0x3E:  // LD A, n
                    A = fetchByte();
                    cycles = 8;
                    break;

                case 0x3F:  // CCF - complement carry flag
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, false);
                    setFlag(FLAG_C, !getFlag(FLAG_C));
                    cycles = 4;
                    break;

                // 0x40 - 0x7F: LD instructions
                // These move values between registers
                // Pattern: 0x40 + (dest * 8) + src
                // dest/src: B=0, C=1, D=2, E=3, H=4, L=5, (HL)=6, A=7

                case 0x40:  // LD B, B
                    B = B;
                    cycles = 4;
                    break;

                case 0x41:  // LD B, C
                    B = C;
                    cycles = 4;
                    break;

                case 0x42:  // LD B, D
                    B = D;
                    cycles = 4;
                    break;

                case 0x43:  // LD B, E
                    B = E;
                    cycles = 4;
                    break;

                case 0x44:  // LD B, H
                    B = H;
                    cycles = 4;
                    break;

                case 0x45:  // LD B, L
                    B = L;
                    cycles = 4;
                    break;

                case 0x46:  // LD B, (HL)
                    B = memory::read(getHL());
                    cycles = 8;
                    break;

                case 0x47:  // LD B, A
                    B = A;
                    cycles = 4;
                    break;

                case 0x48:  // LD C, B
                    C = B;
                    cycles = 4;
                    break;

                case 0x49:  // LD C, C
                    C = C;
                    cycles = 4;
                    break;

                case 0x4A:  // LD C, D
                    C = D;
                    cycles = 4;
                    break;

                case 0x4B:  // LD C, E
                    C = E;
                    cycles = 4;
                    break;

                case 0x4C:  // LD C, H
                    C = H;
                    cycles = 4;
                    break;

                case 0x4D:  // LD C, L
                    C = L;
                    cycles = 4;
                    break;

                case 0x4E:  // LD C, (HL)
                    C = memory::read(getHL());
                    cycles = 8;
                    break;

                case 0x4F:  // LD C, A
                    C = A;
                    cycles = 4;
                    break;

                case 0x50:  // LD D, B
                    D = B;
                    cycles = 4;
                    break;

                case 0x51:  // LD D, C
                    D = C;
                    cycles = 4;
                    break;

                case 0x52:  // LD D, D
                    D = D;
                    cycles = 4;
                    break;

                case 0x53:  // LD D, E
                    D = E;
                    cycles = 4;
                    break;

                case 0x54:  // LD D, H
                    D = H;
                    cycles = 4;
                    break;

                case 0x55:  // LD D, L
                    D = L;
                    cycles = 4;
                    break;

                case 0x56:  // LD D, (HL)
                    D = memory::read(getHL());
                    cycles = 8;
                    break;

                case 0x57:  // LD D, A
                    D = A;
                    cycles = 4;
                    break;

                case 0x58:  // LD E, B
                    E = B;
                    cycles = 4;
                    break;

                case 0x59:  // LD E, C
                    E = C;
                    cycles = 4;
                    break;

                case 0x5A:  // LD E, D
                    E = D;
                    cycles = 4;
                    break;

                case 0x5B:  // LD E, E
                    E = E;
                    cycles = 4;
                    break;

                case 0x5C:  // LD E, H
                    E = H;
                    cycles = 4;
                    break;

                case 0x5D:  // LD E, L
                    E = L;
                    cycles = 4;
                    break;

                case 0x5E:  // LD E, (HL)
                    E = memory::read(getHL());
                    cycles = 8;
                    break;

                case 0x5F:  // LD E, A
                    E = A;
                    cycles = 4;
                    break;

                case 0x60:  // LD H, B
                    H = B;
                    cycles = 4;
                    break;

                case 0x61:  // LD H, C
                    H = C;
                    cycles = 4;
                    break;

                case 0x62:  // LD H, D
                    H = D;
                    cycles = 4;
                    break;

                case 0x63:  // LD H, E
                    H = E;
                    cycles = 4;
                    break;

                case 0x64:  // LD H, H
                    H = H;
                    cycles = 4;
                    break;

                case 0x65:  // LD H, L
                    H = L;
                    cycles = 4;
                    break;

                case 0x66:  // LD H, (HL)
                    H = memory::read(getHL());
                    cycles = 8;
                    break;

                case 0x67:  // LD H, A
                    H = A;
                    cycles = 4;
                    break;

                case 0x68:  // LD L, B
                    L = B;
                    cycles = 4;
                    break;

                case 0x69:  // LD L, C
                    L = C;
                    cycles = 4;
                    break;

                case 0x6A:  // LD L, D
                    L = D;
                    cycles = 4;
                    break;

                case 0x6B:  // LD L, E
                    L = E;
                    cycles = 4;
                    break;

                case 0x6C:  // LD L, H
                    L = H;
                    cycles = 4;
                    break;

                case 0x6D:  // LD L, L
                    L = L;
                    cycles = 4;
                    break;

                case 0x6E:  // LD L, (HL)
                    L = memory::read(getHL());
                    cycles = 8;
                    break;

                case 0x6F:  // LD L, A
                    L = A;
                    cycles = 4;
                    break;

                case 0x70:  // LD (HL), B
                    memory::write(getHL(), B);
                    cycles = 8;
                    break;

                case 0x71:  // LD (HL), C
                    memory::write(getHL(), C);
                    cycles = 8;
                    break;

                case 0x72:  // LD (HL), D
                    memory::write(getHL(), D);
                    cycles = 8;
                    break;

                case 0x73:  // LD (HL), E
                    memory::write(getHL(), E);
                    cycles = 8;
                    break;

                case 0x74:  // LD (HL), H
                    memory::write(getHL(), H);
                    cycles = 8;
                    break;

                case 0x75:  // LD (HL), L
                    memory::write(getHL(), L);
                    cycles = 8;
                    break;

                case 0x76:  // HALT - stop CPU until interrupt
                    halted = true;
                    cycles = 4;
                    break;

                case 0x77:  // LD (HL), A
                    memory::write(getHL(), A);
                    cycles = 8;
                    break;

                case 0x78:  // LD A, B
                    A = B;
                    cycles = 4;
                    break;

                case 0x79:  // LD A, C
                    A = C;
                    cycles = 4;
                    break;

                case 0x7A:  // LD A, D
                    A = D;
                    cycles = 4;
                    break;

                case 0x7B:  // LD A, E
                    A = E;
                    cycles = 4;
                    break;

                case 0x7C:  // LD A, H
                    A = H;
                    cycles = 4;
                    break;

                case 0x7D:  // LD A, L
                    A = L;
                    cycles = 4;
                    break;

                case 0x7E:  // LD A, (HL)
                    A = memory::read(getHL());
                    cycles = 8;
                    break;

                case 0x7F:  // LD A, A
                    A = A;
                    cycles = 4;
                    break;

                // 0x80 - 0xBF: ALU operations
                // All operate on A register with another value
                // Pattern: 0x80 + (operation * 8) + src
                // Operations: ADD=0, ADC=1, SUB=2, SBC=3, AND=4, XOR=5, OR=6, CP=7
                // src: B=0, C=1, D=2, E=3, H=4, L=5, (HL)=6, A=7

                // ADD A, r - add to A
                case 0x80:  // ADD A, B
                    {
                        uint16_t result = A + B;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, ((A & 0x0F) + (B & 0x0F)) > 0x0F);
                        setFlag(FLAG_C, result > 0xFF);
                        A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x81:  // ADD A, C
                    {
                        uint16_t result = A + C;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, ((A & 0x0F) + (C & 0x0F)) > 0x0F);
                        setFlag(FLAG_C, result > 0xFF);
                        A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x82:  // ADD A, D
                    {
                        uint16_t result = A + D;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, ((A & 0x0F) + (D & 0x0F)) > 0x0F);
                        setFlag(FLAG_C, result > 0xFF);
                        A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x83:  // ADD A, E
                    {
                        uint16_t result = A + E;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, ((A & 0x0F) + (E & 0x0F)) > 0x0F);
                        setFlag(FLAG_C, result > 0xFF);
                        A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x84:  // ADD A, H
                    {
                        uint16_t result = A + H;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, ((A & 0x0F) + (H & 0x0F)) > 0x0F);
                        setFlag(FLAG_C, result > 0xFF);
                        A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x85:  // ADD A, L
                    {
                        uint16_t result = A + L;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, ((A & 0x0F) + (L & 0x0F)) > 0x0F);
                        setFlag(FLAG_C, result > 0xFF);
                        A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x86:  // ADD A, (HL)
                    {
                        uint8_t val = memory::read(getHL());
                        uint16_t result = A + val;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, ((A & 0x0F) + (val & 0x0F)) > 0x0F);
                        setFlag(FLAG_C, result > 0xFF);
                        A = result & 0xFF;
                    }
                    cycles = 8;
                    break;

                case 0x87:  // ADD A, A
                    {
                        uint16_t result = A + A;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, ((A & 0x0F) + (A & 0x0F)) > 0x0F);
                        setFlag(FLAG_C, result > 0xFF);
                        A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                // ADC A, r - add with carry
                case 0x88:  // ADC A, B
                    {
                        uint8_t carry = getFlag(FLAG_C) ? 1 : 0;
                        uint16_t result = A + B + carry;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, ((A & 0x0F) + (B & 0x0F) + carry) > 0x0F);
                        setFlag(FLAG_C, result > 0xFF);
                        A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x89:  // ADC A, C
                    {
                        uint8_t carry = getFlag(FLAG_C) ? 1 : 0;
                        uint16_t result = A + C + carry;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, ((A & 0x0F) + (C & 0x0F) + carry) > 0x0F);
                        setFlag(FLAG_C, result > 0xFF);
                        A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x8A:  // ADC A, D
                    {
                        uint8_t carry = getFlag(FLAG_C) ? 1 : 0;
                        uint16_t result = A + D + carry;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, ((A & 0x0F) + (D & 0x0F) + carry) > 0x0F);
                        setFlag(FLAG_C, result > 0xFF);
                        A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x8B:  // ADC A, E
                    {
                        uint8_t carry = getFlag(FLAG_C) ? 1 : 0;
                        uint16_t result = A + E + carry;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, ((A & 0x0F) + (E & 0x0F) + carry) > 0x0F);
                        setFlag(FLAG_C, result > 0xFF);
                        A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x8C:  // ADC A, H
                    {
                        uint8_t carry = getFlag(FLAG_C) ? 1 : 0;
                        uint16_t result = A + H + carry;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, ((A & 0x0F) + (H & 0x0F) + carry) > 0x0F);
                        setFlag(FLAG_C, result > 0xFF);
                        A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x8D:  // ADC A, L
                    {
                        uint8_t carry = getFlag(FLAG_C) ? 1 : 0;
                        uint16_t result = A + L + carry;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, ((A & 0x0F) + (L & 0x0F) + carry) > 0x0F);
                        setFlag(FLAG_C, result > 0xFF);
                        A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x8E:  // ADC A, (HL)
                    {
                        uint8_t val = memory::read(getHL());
                        uint8_t carry = getFlag(FLAG_C) ? 1 : 0;
                        uint16_t result = A + val + carry;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, ((A & 0x0F) + (val & 0x0F) + carry) > 0x0F);
                        setFlag(FLAG_C, result > 0xFF);
                        A = result & 0xFF;
                    }
                    cycles = 8;
                    break;

                case 0x8F:  // ADC A, A
                    {
                        uint8_t carry = getFlag(FLAG_C) ? 1 : 0;
                        uint16_t result = A + A + carry;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, ((A & 0x0F) + (A & 0x0F) + carry) > 0x0F);
                        setFlag(FLAG_C, result > 0xFF);
                        A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                // SUB A, r - subtract from A
                case 0x90:  // SUB B
                    {
                        uint8_t val = B;
                        setFlag(FLAG_Z, A == val);
                        setFlag(FLAG_N, true);
                        setFlag(FLAG_H, (A & 0x0F) < (val & 0x0F));
                        setFlag(FLAG_C, A < val);
                        A -= val;
                    }
                    cycles = 4;
                    break;

                case 0x91:  // SUB C
                    {
                        uint8_t val = C;
                        setFlag(FLAG_Z, A == val);
                        setFlag(FLAG_N, true);
                        setFlag(FLAG_H, (A & 0x0F) < (val & 0x0F));
                        setFlag(FLAG_C, A < val);
                        A -= val;
                    }
                    cycles = 4;
                    break;

                case 0x92:  // SUB D
                    {
                        uint8_t val = D;
                        setFlag(FLAG_Z, A == val);
                        setFlag(FLAG_N, true);
                        setFlag(FLAG_H, (A & 0x0F) < (val & 0x0F));
                        setFlag(FLAG_C, A < val);
                        A -= val;
                    }
                    cycles = 4;
                    break;

                case 0x93:  // SUB E
                    {
                        uint8_t val = E;
                        setFlag(FLAG_Z, A == val);
                        setFlag(FLAG_N, true);
                        setFlag(FLAG_H, (A & 0x0F) < (val & 0x0F));
                        setFlag(FLAG_C, A < val);
                        A -= val;
                    }
                    cycles = 4;
                    break;

                case 0x94:  // SUB H
                    {
                        uint8_t val = H;
                        setFlag(FLAG_Z, A == val);
                        setFlag(FLAG_N, true);
                        setFlag(FLAG_H, (A & 0x0F) < (val & 0x0F));
                        setFlag(FLAG_C, A < val);
                        A -= val;
                    }
                    cycles = 4;
                    break;

                case 0x95:  // SUB L
                    {
                        uint8_t val = L;
                        setFlag(FLAG_Z, A == val);
                        setFlag(FLAG_N, true);
                        setFlag(FLAG_H, (A & 0x0F) < (val & 0x0F));
                        setFlag(FLAG_C, A < val);
                        A -= val;
                    }
                    cycles = 4;
                    break;

                case 0x96:  // SUB (HL)
                    {
                        uint8_t val = memory::read(getHL());
                        setFlag(FLAG_Z, A == val);
                        setFlag(FLAG_N, true);
                        setFlag(FLAG_H, (A & 0x0F) < (val & 0x0F));
                        setFlag(FLAG_C, A < val);
                        A -= val;
                    }
                    cycles = 8;
                    break;

                case 0x97:  // SUB A
                    {
                        setFlag(FLAG_Z, true);
                        setFlag(FLAG_N, true);
                        setFlag(FLAG_H, false);
                        setFlag(FLAG_C, false);
                        A = 0;
                    }
                    cycles = 4;
                    break;

                // SBC A, r - subtract with carry
                case 0x98:  // SBC A, B
                    {
                        uint8_t carry = getFlag(FLAG_C) ? 1 : 0;
                        int result = A - B - carry;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, true);
                        setFlag(FLAG_H, ((A & 0x0F) - (B & 0x0F) - carry) < 0);
                        setFlag(FLAG_C, result < 0);
                        A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x99:  // SBC A, C
                    {
                        uint8_t carry = getFlag(FLAG_C) ? 1 : 0;
                        int result = A - C - carry;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, true);
                        setFlag(FLAG_H, ((A & 0x0F) - (C & 0x0F) - carry) < 0);
                        setFlag(FLAG_C, result < 0);
                        A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x9A:  // SBC A, D
                    {
                        uint8_t carry = getFlag(FLAG_C) ? 1 : 0;
                        int result = A - D - carry;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, true);
                        setFlag(FLAG_H, ((A & 0x0F) - (D & 0x0F) - carry) < 0);
                        setFlag(FLAG_C, result < 0);
                        A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x9B:  // SBC A, E
                    {
                        uint8_t carry = getFlag(FLAG_C) ? 1 : 0;
                        int result = A - E - carry;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, true);
                        setFlag(FLAG_H, ((A & 0x0F) - (E & 0x0F) - carry) < 0);
                        setFlag(FLAG_C, result < 0);
                        A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x9C:  // SBC A, H
                    {
                        uint8_t carry = getFlag(FLAG_C) ? 1 : 0;
                        int result = A - H - carry;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, true);
                        setFlag(FLAG_H, ((A & 0x0F) - (H & 0x0F) - carry) < 0);
                        setFlag(FLAG_C, result < 0);
                        A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x9D:  // SBC A, L
                    {
                        uint8_t carry = getFlag(FLAG_C) ? 1 : 0;
                        int result = A - L - carry;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, true);
                        setFlag(FLAG_H, ((A & 0x0F) - (L & 0x0F) - carry) < 0);
                        setFlag(FLAG_C, result < 0);
                        A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                case 0x9E:  // SBC A, (HL)
                    {
                        uint8_t val = memory::read(getHL());
                        uint8_t carry = getFlag(FLAG_C) ? 1 : 0;
                        int result = A - val - carry;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, true);
                        setFlag(FLAG_H, ((A & 0x0F) - (val & 0x0F) - carry) < 0);
                        setFlag(FLAG_C, result < 0);
                        A = result & 0xFF;
                    }
                    cycles = 8;
                    break;

                case 0x9F:  // SBC A, A
                    {
                        uint8_t carry = getFlag(FLAG_C) ? 1 : 0;
                        int result = A - A - carry;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, true);
                        setFlag(FLAG_H, carry);
                        setFlag(FLAG_C, carry);
                        A = result & 0xFF;
                    }
                    cycles = 4;
                    break;

                // AND A, r - bitwise AND with A
                case 0xA0:  // AND B
                    A &= B;
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, true);
                    setFlag(FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xA1:  // AND C
                    A &= C;
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, true);
                    setFlag(FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xA2:  // AND D
                    A &= D;
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, true);
                    setFlag(FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xA3:  // AND E
                    A &= E;
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, true);
                    setFlag(FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xA4:  // AND H
                    A &= H;
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, true);
                    setFlag(FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xA5:  // AND L
                    A &= L;
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, true);
                    setFlag(FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xA6:  // AND (HL)
                    A &= memory::read(getHL());
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, true);
                    setFlag(FLAG_C, false);
                    cycles = 8;
                    break;

                case 0xA7:  // AND A
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, true);
                    setFlag(FLAG_C, false);
                    cycles = 4;
                    break;

                // XOR A, r - bitwise XOR with A
                case 0xA8:  // XOR B
                    A ^= B;
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, false);
                    setFlag(FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xA9:  // XOR C
                    A ^= C;
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, false);
                    setFlag(FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xAA:  // XOR D
                    A ^= D;
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, false);
                    setFlag(FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xAB:  // XOR E
                    A ^= E;
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, false);
                    setFlag(FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xAC:  // XOR H
                    A ^= H;
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, false);
                    setFlag(FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xAD:  // XOR L
                    A ^= L;
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, false);
                    setFlag(FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xAE:  // XOR (HL)
                    A ^= memory::read(getHL());
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, false);
                    setFlag(FLAG_C, false);
                    cycles = 8;
                    break;

                case 0xAF:  // XOR A - common way to set A to 0
                    A = 0;
                    setFlag(FLAG_Z, true);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, false);
                    setFlag(FLAG_C, false);
                    cycles = 4;
                    break;

                // OR A, r - bitwise OR with A
                case 0xB0:  // OR B
                    A |= B;
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, false);
                    setFlag(FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xB1:  // OR C
                    A |= C;
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, false);
                    setFlag(FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xB2:  // OR D
                    A |= D;
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, false);
                    setFlag(FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xB3:  // OR E
                    A |= E;
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, false);
                    setFlag(FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xB4:  // OR H
                    A |= H;
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, false);
                    setFlag(FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xB5:  // OR L
                    A |= L;
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, false);
                    setFlag(FLAG_C, false);
                    cycles = 4;
                    break;

                case 0xB6:  // OR (HL)
                    A |= memory::read(getHL());
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, false);
                    setFlag(FLAG_C, false);
                    cycles = 8;
                    break;

                case 0xB7:  // OR A
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, false);
                    setFlag(FLAG_C, false);
                    cycles = 4;
                    break;

                // CP A, r - compare (subtract but don't store result)
                case 0xB8:  // CP B
                    setFlag(FLAG_Z, A == B);
                    setFlag(FLAG_N, true);
                    setFlag(FLAG_H, (A & 0x0F) < (B & 0x0F));
                    setFlag(FLAG_C, A < B);
                    cycles = 4;
                    break;

                case 0xB9:  // CP C
                    setFlag(FLAG_Z, A == C);
                    setFlag(FLAG_N, true);
                    setFlag(FLAG_H, (A & 0x0F) < (C & 0x0F));
                    setFlag(FLAG_C, A < C);
                    cycles = 4;
                    break;

                case 0xBA:  // CP D
                    setFlag(FLAG_Z, A == D);
                    setFlag(FLAG_N, true);
                    setFlag(FLAG_H, (A & 0x0F) < (D & 0x0F));
                    setFlag(FLAG_C, A < D);
                    cycles = 4;
                    break;

                case 0xBB:  // CP E
                    setFlag(FLAG_Z, A == E);
                    setFlag(FLAG_N, true);
                    setFlag(FLAG_H, (A & 0x0F) < (E & 0x0F));
                    setFlag(FLAG_C, A < E);
                    cycles = 4;
                    break;

                case 0xBC:  // CP H
                    setFlag(FLAG_Z, A == H);
                    setFlag(FLAG_N, true);
                    setFlag(FLAG_H, (A & 0x0F) < (H & 0x0F));
                    setFlag(FLAG_C, A < H);
                    cycles = 4;
                    break;

                case 0xBD:  // CP L
                    setFlag(FLAG_Z, A == L);
                    setFlag(FLAG_N, true);
                    setFlag(FLAG_H, (A & 0x0F) < (L & 0x0F));
                    setFlag(FLAG_C, A < L);
                    cycles = 4;
                    break;

                case 0xBE:  // CP (HL)
                    {
                        uint8_t val = memory::read(getHL());
                        setFlag(FLAG_Z, A == val);
                        setFlag(FLAG_N, true);
                        setFlag(FLAG_H, (A & 0x0F) < (val & 0x0F));
                        setFlag(FLAG_C, A < val);
                    }
                    cycles = 8;
                    break;

                case 0xBF:  // CP A
                    setFlag(FLAG_Z, true);
                    setFlag(FLAG_N, true);
                    setFlag(FLAG_H, false);
                    setFlag(FLAG_C, false);
                    cycles = 4;
                    break;

                // 0xC0 - 0xFF: Control flow and misc

                case 0xC0:  // RET NZ - return if not zero
                    if (!getFlag(FLAG_Z)) {
                        PC = popStack();
                        cycles = 20;
                    } else {
                        cycles = 8;
                    }
                    break;

                case 0xC1:  // POP BC
                    setBC(popStack());
                    cycles = 12;
                    break;

                case 0xC2:  // JP NZ, nn - jump if not zero
                    {
                        uint16_t addr = fetchWord();
                        if (!getFlag(FLAG_Z)) {
                            PC = addr;
                            cycles = 16;
                        } else {
                            cycles = 12;
                        }
                    }
                    break;

                case 0xC3:  // JP nn - unconditional jump
                    PC = fetchWord();
                    cycles = 16;
                    break;

                case 0xC4:  // CALL NZ, nn - call if not zero
                    {
                        uint16_t addr = fetchWord();
                        if (!getFlag(FLAG_Z)) {
                            pushStack(PC);
                            PC = addr;
                            cycles = 24;
                        } else {
                            cycles = 12;
                        }
                    }
                    break;

                case 0xC5:  // PUSH BC
                    pushStack(getBC());
                    cycles = 16;
                    break;

                case 0xC6:  // ADD A, n - add immediate to A
                    {
                        uint8_t val = fetchByte();
                        uint16_t result = A + val;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, ((A & 0x0F) + (val & 0x0F)) > 0x0F);
                        setFlag(FLAG_C, result > 0xFF);
                        A = result & 0xFF;
                    }
                    cycles = 8;
                    break;

                case 0xC7:  // RST 00 - restart (call to address 0x00)
                    pushStack(PC);
                    PC = 0x0000;
                    cycles = 16;
                    break;

                case 0xC8:  // RET Z - return if zero
                    if (getFlag(FLAG_Z)) {
                        PC = popStack();
                        cycles = 20;
                    } else {
                        cycles = 8;
                    }
                    break;

                case 0xC9:  // RET - unconditional return
                    PC = popStack();
                    cycles = 16;
                    break;

                case 0xCA:  // JP Z, nn - jump if zero
                    {
                        uint16_t addr = fetchWord();
                        if (getFlag(FLAG_Z)) {
                            PC = addr;
                            cycles = 16;
                        } else {
                            cycles = 12;
                        }
                    }
                    break;

                case 0xCB:  // CB prefix - extended opcodes
                    // we'll handle this separately
                    {
                        uint8_t cbOpcode = fetchByte();
                        cycles = executeCB(cbOpcode);
                    }
                    break;

                case 0xCC:  // CALL Z, nn - call if zero
                    {
                        uint16_t addr = fetchWord();
                        if (getFlag(FLAG_Z)) {
                            pushStack(PC);
                            PC = addr;
                            cycles = 24;
                        } else {
                            cycles = 12;
                        }
                    }
                    break;

                case 0xCD:  // CALL nn - unconditional call
                    {
                        uint16_t addr = fetchWord();
                        pushStack(PC);
                        PC = addr;
                    }
                    cycles = 24;
                    break;

                case 0xCE:  // ADC A, n - add immediate with carry
                    {
                        uint8_t val = fetchByte();
                        uint8_t carry = getFlag(FLAG_C) ? 1 : 0;
                        uint16_t result = A + val + carry;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, ((A & 0x0F) + (val & 0x0F) + carry) > 0x0F);
                        setFlag(FLAG_C, result > 0xFF);
                        A = result & 0xFF;
                    }
                    cycles = 8;
                    break;

                case 0xCF:  // RST 08
                    pushStack(PC);
                    PC = 0x0008;
                    cycles = 16;
                    break;

                case 0xD0:  // RET NC - return if no carry
                    if (!getFlag(FLAG_C)) {
                        PC = popStack();
                        cycles = 20;
                    } else {
                        cycles = 8;
                    }
                    break;

                case 0xD1:  // POP DE
                    setDE(popStack());
                    cycles = 12;
                    break;

                case 0xD2:  // JP NC, nn - jump if no carry
                    {
                        uint16_t addr = fetchWord();
                        if (!getFlag(FLAG_C)) {
                            PC = addr;
                            cycles = 16;
                        } else {
                            cycles = 12;
                        }
                    }
                    break;

                // 0xD3 doesn't exist on Game Boy

                case 0xD4:  // CALL NC, nn - call if no carry
                    {
                        uint16_t addr = fetchWord();
                        if (!getFlag(FLAG_C)) {
                            pushStack(PC);
                            PC = addr;
                            cycles = 24;
                        } else {
                            cycles = 12;
                        }
                    }
                    break;

                case 0xD5:  // PUSH DE
                    pushStack(getDE());
                    cycles = 16;
                    break;

                case 0xD6:  // SUB n - subtract immediate
                    {
                        uint8_t val = fetchByte();
                        setFlag(FLAG_Z, A == val);
                        setFlag(FLAG_N, true);
                        setFlag(FLAG_H, (A & 0x0F) < (val & 0x0F));
                        setFlag(FLAG_C, A < val);
                        A -= val;
                    }
                    cycles = 8;
                    break;

                case 0xD7:  // RST 10
                    pushStack(PC);
                    PC = 0x0010;
                    cycles = 16;
                    break;

                case 0xD8:  // RET C - return if carry
                    if (getFlag(FLAG_C)) {
                        PC = popStack();
                        cycles = 20;
                    } else {
                        cycles = 8;
                    }
                    break;

                case 0xD9:  // RETI - return and enable interrupts
                    PC = popStack();
                    ime = true;
                    cycles = 16;
                    break;

                case 0xDA:  // JP C, nn - jump if carry
                    {
                        uint16_t addr = fetchWord();
                        if (getFlag(FLAG_C)) {
                            PC = addr;
                            cycles = 16;
                        } else {
                            cycles = 12;
                        }
                    }
                    break;

                // 0xDB doesn't exist on Game Boy

                case 0xDC:  // CALL C, nn - call if carry
                    {
                        uint16_t addr = fetchWord();
                        if (getFlag(FLAG_C)) {
                            pushStack(PC);
                            PC = addr;
                            cycles = 24;
                        } else {
                            cycles = 12;
                        }
                    }
                    break;

                // 0xDD doesn't exist on Game Boy

                case 0xDE:  // SBC A, n - subtract immediate with carry
                    {
                        uint8_t val = fetchByte();
                        uint8_t carry = getFlag(FLAG_C) ? 1 : 0;
                        int result = A - val - carry;
                        setFlag(FLAG_Z, (result & 0xFF) == 0);
                        setFlag(FLAG_N, true);
                        setFlag(FLAG_H, ((A & 0x0F) - (val & 0x0F) - carry) < 0);
                        setFlag(FLAG_C, result < 0);
                        A = result & 0xFF;
                    }
                    cycles = 8;
                    break;

                case 0xDF:  // RST 18
                    pushStack(PC);
                    PC = 0x0018;
                    cycles = 16;
                    break;

                case 0xE0:  // LD (0xFF00 + n), A - store A to high RAM
                    memory::write(0xFF00 + fetchByte(), A);
                    cycles = 12;
                    break;

                case 0xE1:  // POP HL
                    setHL(popStack());
                    cycles = 12;
                    break;

                case 0xE2:  // LD (0xFF00 + C), A - store A to high RAM at C
                    memory::write(0xFF00 + C, A);
                    cycles = 8;
                    break;

                // 0xE3 doesn't exist on Game Boy
                // 0xE4 doesn't exist on Game Boy

                case 0xE5:  // PUSH HL
                    pushStack(getHL());
                    cycles = 16;
                    break;

                case 0xE6:  // AND n - AND immediate
                    A &= fetchByte();
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, true);
                    setFlag(FLAG_C, false);
                    cycles = 8;
                    break;

                case 0xE7:  // RST 20
                    pushStack(PC);
                    PC = 0x0020;
                    cycles = 16;
                    break;

                case 0xE8:  // ADD SP, n - add signed immediate to SP
                    {
                        int8_t val = (int8_t)fetchByte();
                        setFlag(FLAG_Z, false);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, ((SP & 0x0F) + (val & 0x0F)) > 0x0F);
                        setFlag(FLAG_C, ((SP & 0xFF) + (val & 0xFF)) > 0xFF);
                        SP += val;
                    }
                    cycles = 16;
                    break;

                case 0xE9:  // JP HL - jump to address in HL
                    PC = getHL();
                    cycles = 4;
                    break;

                case 0xEA:  // LD (nn), A - store A at address
                    memory::write(fetchWord(), A);
                    cycles = 16;
                    break;

                // 0xEB doesn't exist on Game Boy
                // 0xEC doesn't exist on Game Boy
                // 0xED doesn't exist on Game Boy

                case 0xEE:  // XOR n - XOR immediate
                    A ^= fetchByte();
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, false);
                    setFlag(FLAG_C, false);
                    cycles = 8;
                    break;

                case 0xEF:  // RST 28
                    pushStack(PC);
                    PC = 0x0028;
                    cycles = 16;
                    break;

                case 0xF0:  // LD A, (0xFF00 + n) - load A from high RAM
                    A = memory::read(0xFF00 + fetchByte());
                    cycles = 12;
                    break;

                case 0xF1:  // POP AF
                    setAF(popStack());
                    cycles = 12;
                    break;

                case 0xF2:  // LD A, (0xFF00 + C) - load A from high RAM at C
                    A = memory::read(0xFF00 + C);
                    cycles = 8;
                    break;

                case 0xF3:  // DI - disable interrupts
                    ime = false;
                    cycles = 4;
                    break;

                // 0xF4 doesn't exist on Game Boy

                case 0xF5:  // PUSH AF
                    pushStack(getAF());
                    cycles = 16;
                    break;

                case 0xF6:  // OR n - OR immediate
                    A |= fetchByte();
                    setFlag(FLAG_Z, A == 0);
                    setFlag(FLAG_N, false);
                    setFlag(FLAG_H, false);
                    setFlag(FLAG_C, false);
                    cycles = 8;
                    break;

                case 0xF7:  // RST 30
                    pushStack(PC);
                    PC = 0x0030;
                    cycles = 16;
                    break;

                case 0xF8:  // LD HL, SP + n - load SP plus signed offset into HL
                    {
                        int8_t val = (int8_t)fetchByte();
                        setFlag(FLAG_Z, false);
                        setFlag(FLAG_N, false);
                        setFlag(FLAG_H, ((SP & 0x0F) + (val & 0x0F)) > 0x0F);
                        setFlag(FLAG_C, ((SP & 0xFF) + (val & 0xFF)) > 0xFF);
                        setHL(SP + val);
                    }
                    cycles = 12;
                    break;

                case 0xF9:  // LD SP, HL - copy HL to SP
                    SP = getHL();
                    cycles = 8;
                    break;

                case 0xFA:  // LD A, (nn) - load A from address
                    A = memory::read(fetchWord());
                    cycles = 16;
                    break;

                case 0xFB:  // EI - enable interrupts (delayed by one instruction)
                    imeScheduled = true;
                    cycles = 4;
                    break;

                // 0xFC doesn't exist on Game Boy
                // 0xFD doesn't exist on Game Boy

                case 0xFE:  // CP n - compare immediate
                    {
                        uint8_t val = fetchByte();
                        setFlag(FLAG_Z, A == val);
                        setFlag(FLAG_N, true);
                        setFlag(FLAG_H, (A & 0x0F) < (val & 0x0F));
                        setFlag(FLAG_C, A < val);
                    }
                    cycles = 8;
                    break;

                case 0xFF:  // RST 38
                    pushStack(PC);
                    PC = 0x0038;
                    cycles = 16;
                    break;

                default:
                    cycles = 4;
                    break;
            }

            return cycles;
        }
    }
}