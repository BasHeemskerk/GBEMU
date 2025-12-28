#include "included/opcode_parser.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace gb{
    namespace opcode_parser{
        
        //helper to trime whitespace
        static char* trim(char* str){
            while (*str == ' ' || *str == '\t') str++;
            char* end = str + strlen(str) - 1;
            while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
                *end = '\0';
                end--;
            }
            return str;
        }

        // Parse micro-op from string
        static MicroOp parseMicroOp(const char* str) {
            if (strcmp(str, "NOP") == 0) return MicroOp::NOP;
            if (strcmp(str, "LD8") == 0) return MicroOp::LD8;
            if (strcmp(str, "ST8") == 0) return MicroOp::ST8;
            if (strcmp(str, "LD16") == 0) return MicroOp::LD16;
            if (strcmp(str, "ST16") == 0) return MicroOp::ST16;
            if (strcmp(str, "ADD8") == 0) return MicroOp::ADD8;
            if (strcmp(str, "ADC8") == 0) return MicroOp::ADC8;
            if (strcmp(str, "SUB8") == 0) return MicroOp::SUB8;
            if (strcmp(str, "SBC8") == 0) return MicroOp::SBC8;
            if (strcmp(str, "INC8") == 0) return MicroOp::INC8;
            if (strcmp(str, "DEC8") == 0) return MicroOp::DEC8;
            if (strcmp(str, "AND8") == 0) return MicroOp::AND8;
            if (strcmp(str, "OR8") == 0) return MicroOp::OR8;
            if (strcmp(str, "XOR8") == 0) return MicroOp::XOR8;
            if (strcmp(str, "CP8") == 0) return MicroOp::CP8;
            if (strcmp(str, "ADD16") == 0) return MicroOp::ADD16;
            if (strcmp(str, "INC16") == 0) return MicroOp::INC16;
            if (strcmp(str, "DEC16") == 0) return MicroOp::DEC16;
            if (strcmp(str, "ADDSP") == 0) return MicroOp::ADDSP;
            if (strcmp(str, "RLCA") == 0) return MicroOp::RLCA;
            if (strcmp(str, "RRCA") == 0) return MicroOp::RRCA;
            if (strcmp(str, "RLA") == 0) return MicroOp::RLA;
            if (strcmp(str, "RRA") == 0) return MicroOp::RRA;
            if (strcmp(str, "RLC") == 0) return MicroOp::RLC;
            if (strcmp(str, "RRC") == 0) return MicroOp::RRC;
            if (strcmp(str, "RL") == 0) return MicroOp::RL;
            if (strcmp(str, "RR") == 0) return MicroOp::RR;
            if (strcmp(str, "SLA") == 0) return MicroOp::SLA;
            if (strcmp(str, "SRA") == 0) return MicroOp::SRA;
            if (strcmp(str, "SRL") == 0) return MicroOp::SRL;
            if (strcmp(str, "SWAP") == 0) return MicroOp::SWAP;
            if (strcmp(str, "BIT") == 0) return MicroOp::BIT;
            if (strcmp(str, "RES") == 0) return MicroOp::RES;
            if (strcmp(str, "SET") == 0) return MicroOp::SET;
            if (strcmp(str, "JP") == 0) return MicroOp::JP;
            if (strcmp(str, "JP_Z") == 0) return MicroOp::JP_Z;
            if (strcmp(str, "JP_NZ") == 0) return MicroOp::JP_NZ;
            if (strcmp(str, "JP_C") == 0) return MicroOp::JP_C;
            if (strcmp(str, "JP_NC") == 0) return MicroOp::JP_NC;
            if (strcmp(str, "JP_HL") == 0) return MicroOp::JP_HL;
            if (strcmp(str, "JR") == 0) return MicroOp::JR;
            if (strcmp(str, "JR_Z") == 0) return MicroOp::JR_Z;
            if (strcmp(str, "JR_NZ") == 0) return MicroOp::JR_NZ;
            if (strcmp(str, "JR_C") == 0) return MicroOp::JR_C;
            if (strcmp(str, "JR_NC") == 0) return MicroOp::JR_NC;
            if (strcmp(str, "CALL") == 0) return MicroOp::CALL;
            if (strcmp(str, "CALL_Z") == 0) return MicroOp::CALL_Z;
            if (strcmp(str, "CALL_NZ") == 0) return MicroOp::CALL_NZ;
            if (strcmp(str, "CALL_C") == 0) return MicroOp::CALL_C;
            if (strcmp(str, "CALL_NC") == 0) return MicroOp::CALL_NC;
            if (strcmp(str, "RET") == 0) return MicroOp::RET;
            if (strcmp(str, "RET_Z") == 0) return MicroOp::RET_Z;
            if (strcmp(str, "RET_NZ") == 0) return MicroOp::RET_NZ;
            if (strcmp(str, "RET_C") == 0) return MicroOp::RET_C;
            if (strcmp(str, "RET_NC") == 0) return MicroOp::RET_NC;
            if (strcmp(str, "RETI") == 0) return MicroOp::RETI;
            if (strcmp(str, "RST") == 0) return MicroOp::RST;
            if (strcmp(str, "PUSH") == 0) return MicroOp::PUSH;
            if (strcmp(str, "POP") == 0) return MicroOp::POP;
            if (strcmp(str, "HALT") == 0) return MicroOp::HALT;
            if (strcmp(str, "STOP") == 0) return MicroOp::STOP;
            if (strcmp(str, "DI") == 0) return MicroOp::DI;
            if (strcmp(str, "EI") == 0) return MicroOp::EI;
            if (strcmp(str, "DAA") == 0) return MicroOp::DAA;
            if (strcmp(str, "CPL") == 0) return MicroOp::CPL;
            if (strcmp(str, "CCF") == 0) return MicroOp::CCF;
            if (strcmp(str, "SCF") == 0) return MicroOp::SCF;
            if (strcmp(str, "CB") == 0) return MicroOp::CB;
            
            return MicroOp::NOP; // fallback
        }

        // Parse operand from string
        static Operand parseOperand(const char* str) {
            if (str == nullptr || strlen(str) == 0) return Operand::NONE;
            
            // Registers
            if (strcmp(str, "A") == 0) return Operand::A;
            if (strcmp(str, "B") == 0) return Operand::B;
            if (strcmp(str, "C") == 0) return Operand::C;
            if (strcmp(str, "D") == 0) return Operand::D;
            if (strcmp(str, "E") == 0) return Operand::E;
            if (strcmp(str, "H") == 0) return Operand::H;
            if (strcmp(str, "L") == 0) return Operand::L;
            if (strcmp(str, "F") == 0) return Operand::F;
            if (strcmp(str, "AF") == 0) return Operand::AF;
            if (strcmp(str, "BC") == 0) return Operand::BC;
            if (strcmp(str, "DE") == 0) return Operand::DE;
            if (strcmp(str, "HL") == 0) return Operand::HL;
            if (strcmp(str, "SP") == 0) return Operand::SP;
            if (strcmp(str, "PC") == 0) return Operand::PC;
            
            // Memory
            if (strcmp(str, "(BC)") == 0) return Operand::MEM_BC;
            if (strcmp(str, "(DE)") == 0) return Operand::MEM_DE;
            if (strcmp(str, "(HL)") == 0) return Operand::MEM_HL;
            if (strcmp(str, "(HL+)") == 0) return Operand::MEM_HL_INC;
            if (strcmp(str, "(HL-)") == 0) return Operand::MEM_HL_DEC;
            if (strcmp(str, "(nn)") == 0) return Operand::MEM_NN;
            if (strcmp(str, "(FF00+n)") == 0) return Operand::MEM_FF_N;
            if (strcmp(str, "(FF00+C)") == 0) return Operand::MEM_FF_C;
            
            // Immediates
            if (strcmp(str, "n") == 0) return Operand::IMM8;
            if (strcmp(str, "nn") == 0) return Operand::IMM16;
            if (strcmp(str, "e") == 0) return Operand::IMM8_SIGNED;
            if (strcmp(str, "SP+e") == 0) return Operand::SP_PLUS_E;
            
            // Bit indices
            if (strcmp(str, "0") == 0) return Operand::BIT_0;
            if (strcmp(str, "1") == 0) return Operand::BIT_1;
            if (strcmp(str, "2") == 0) return Operand::BIT_2;
            if (strcmp(str, "3") == 0) return Operand::BIT_3;
            if (strcmp(str, "4") == 0) return Operand::BIT_4;
            if (strcmp(str, "5") == 0) return Operand::BIT_5;
            if (strcmp(str, "6") == 0) return Operand::BIT_6;
            if (strcmp(str, "7") == 0) return Operand::BIT_7;
            
            // RST vectors
            if (strcmp(str, "00H") == 0) return Operand::RST_00;
            if (strcmp(str, "08H") == 0) return Operand::RST_08;
            if (strcmp(str, "10H") == 0) return Operand::RST_10;
            if (strcmp(str, "18H") == 0) return Operand::RST_18;
            if (strcmp(str, "20H") == 0) return Operand::RST_20;
            if (strcmp(str, "28H") == 0) return Operand::RST_28;
            if (strcmp(str, "30H") == 0) return Operand::RST_30;
            if (strcmp(str, "38H") == 0) return Operand::RST_38;
            
            return Operand::NONE;
        }

        //parse a single opcode line

        //format examples:
        //format: 0x00 | 4 | NOP
        //format: 0x00 | 12 | LD16 BC, nn
        static bool parseOpcodeLine(const char* line, uint8_t& opcode, OpcodeEntry& entry){
            char lineCopy[256];
            strncpy(lineCopy, line, 255);
            lineCopy[255] = '\0';

            //split by | 
            char* opcodeStr = strtok(lineCopy, "|");
            char* cyclesStr = strtok(nullptr, "|");
            char* instrStr = strtok(nullptr, "|");

            if (!opcodeStr || !cyclesStr || !instrStr){
                return false;
            }

            opcodeStr = trim(opcodeStr);
            cyclesStr = trim(cyclesStr);
            instrStr = trim(instrStr);

            //parse opcodes hex
            opcode = (uint8_t)strtol(opcodeStr, nullptr, 16);

            //parse cycles (might be 12 or 12/8)
            char* slashPos = strchr(cyclesStr, '/');
            if (slashPos){
                *slashPos = '\0';
                entry.cycles = (uint8_t)atoi(cyclesStr);
                entry.cyclesBranch = (uint8_t)atoi(slashPos + 1);
            } else {
                entry.cycles = (uint8_t)atoi(cyclesStr);
                entry.cyclesBranch = 0;
            }

            //parse instruction
            char instrCopy[128];
            strncpy(instrCopy, instrStr, 127);
            instrCopy[127] = '\0';

            //get micro op (first word)
            char* microOpStr = strtok(instrCopy, " ");
            if (!microOpStr){
                return false;
            }

            entry.op = parseMicroOp(microOpStr);
            entry.dst = Operand::NONE;
            entry.src = Operand::NONE;

            //get operands
            char* operandStr = strtok(nullptr, "");
            if (operandStr){
                operandStr = trim(operandStr);

                //split by comma
                char* dstStr = strtok(operandStr, ",");
                char* srcStr = strtok(nullptr, ",");

                if (dstStr){
                    entry.dst = parseOperand(trim(dstStr));
                }
                if (srcStr){
                    entry.src = parseOperand(trim(srcStr));
                }
            }

            return true;
        }

        bool parse(const char* filepath, OpcodeTable& table){
            FILE* file = fopen(filepath, "r");
            if (!file){
                return false;
            }

            //clear the table
            memset(&table, 0, sizeof(OpcodeTable));

            char line[256];
            bool inInfo = false;
            bool inMainTable = false;
            bool inCBTable = false;

            while (fgets(line, sizeof(line), file)){
                char* trimmed = trim(line);

                //skip empty lines and comments
                if (strlen(trimmed) == 0 || trimmed[0] == ';'){
                    continue;
                }

                //section markers
                if (strcmp(trimmed, "#GB_OPCODE_INFO") == 0) {
                    inInfo = true;
                    continue;
                }
                if (strcmp(trimmed, "#GB_OPCODE_INFO_END") == 0) {
                    inInfo = false;
                    continue;
                }
                if (strcmp(trimmed, "#GB_OPCODE_TABLE") == 0) {
                    inMainTable = true;
                    continue;
                }
                if (strcmp(trimmed, "#GB_OPCODE_TABLE_END") == 0) {
                    inMainTable = false;
                    continue;
                }
                if (strcmp(trimmed, "#GB_OPCODE_CB_TABLE") == 0) {
                    inCBTable = true;
                    continue;
                }
                if (strcmp(trimmed, "#GB_OPCODE_CB_TABLE_END") == 0) {
                    inCBTable = false;
                    continue;
                }

                // Parse info section
                if (inInfo) {
                    if (strncmp(trimmed, "NAME:", 5) == 0) {
                        strncpy(table.name, trim(trimmed + 5), 31);
                    } else if (strncmp(trimmed, "VERSION:", 8) == 0) {
                        table.version = (uint8_t)atoi(trim(trimmed + 8));
                    }
                    continue;
                }
                
                // Parse opcode lines
                if (inMainTable) {
                    uint8_t opcode;
                    OpcodeEntry entry;
                    if (parseOpcodeLine(trimmed, opcode, entry)) {
                        table.main[opcode] = entry;
                    }
                }
                
                if (inCBTable) {
                    uint8_t opcode;
                    OpcodeEntry entry;
                    if (parseOpcodeLine(trimmed, opcode, entry)) {
                        table.cb[opcode] = entry;
                    }
                }
            }

            fclose(file);
            table.loaded = true;
            return true;
        }

        void initDefaults(OpcodeTable& table){
            memset(&table, 0, sizeof(OpcodeTable));
            strcpy(table.name, "BUILTIN");
            table.version = 1;

            //set all to NOP
            for (int i = 0; i < 256; i++){
                table.main[i].op = MicroOp::NOP;
                table.main[i].cycles = 4;
                table.cb[i].op = MicroOp::NOP;
                table.cb[i].cycles = 8;
            }

            table.loaded = true;
        }
    }
}