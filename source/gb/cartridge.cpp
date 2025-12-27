#include "included/cartridge.hpp"
#include <cstring>
#include <cstdio>
#include <3ds.h>

namespace gb{
    namespace cartridge{

        //memory arrays
        uint8_t rom[MAX_ROM_SIZE];
        uint8_t ram[MAX_RAM_SIZE];
        int romSize;
        int ramSize;

        //mapper state
        MapperType mapper;
        int romBank;
        int ramBank;
        bool ramEnabled;
        int mbcMode;

        //game info
        char title[17];

        void initialize(){
            memset(rom, 0, sizeof(rom));
            memset(ram, 0, sizeof(ram));
            romSize = 0;
            ramSize = 0;

            mapper = MapperType::NONE;
            romBank = 1;
            ramBank = 0;
            ramEnabled = false;
            mbcMode = 0;

            memset(title, 0, sizeof(title));
        }

        bool loadRom(const char* filePath){
            FILE* file = fopen(filePath, "rb");
            if (!file){
                printf("Failed to open ROM\n");
                return false;
            }

            //get file size
            fseek(file, 0, SEEK_END);
            romSize = ftell(file);
            fseek(file, 0, SEEK_SET);

            if (romSize > MAX_ROM_SIZE) {
                printf("ROM too large\n");
                fclose(file);
                return false;
            }

            //read rom data
            size_t bytesRead = fread(rom, 1, romSize, file);
            fclose(file);

            if (bytesRead != (size_t)romSize) {
                printf("Failed to read ROM\n");
                return false;
            }

            //extract title from header
            memcpy(title, &rom[0x0134], 16);
            title[16] = '\0';

            //determine the mapper type from header
            uint8_t cartType = rom[0x0147];
            switch (cartType){
                case 0x00:
                    mapper = MapperType::NONE;
                    break;
                case 0x01:
                case 0x02:
                case 0x03:
                    mapper = MapperType::MBC1;
                    break;
                case 0x0F:
                case 0x10:
                case 0x11:
                case 0x12:
                case 0x13:
                    mapper = MapperType::MBC3;
                    break;
                case 0x19:
                case 0x1A:
                case 0x1B:
                case 0x1C:
                case 0x1D:
                case 0x1E:
                    mapper = MapperType::MBC5;
                    break;
                default:
                    // treat unknown as no mapper
                    mapper = MapperType::NONE;
                    break;
            }

            //determine ram size from header
            uint8_t ramCode = rom[0x0149];
            switch (ramCode){
                case 0x00: ramSize = 0; break;
                case 0x01: ramSize = 2 * 1024; break;
                case 0x02: ramSize = 8 * 1024; break;
                case 0x03: ramSize = 32 * 1024; break;
                case 0x04: ramSize = 128 * 1024; break;
                case 0x05: ramSize = 64 * 1024; break;
                default: ramSize = 0; break;
            }

            printf("Loaded: %s\n", title);
            printf("ROM size: %d KB\n", romSize / 1024);
            printf("RAM size: %d KB\n", ramSize / 1024);

            return true;
        }

        uint8_t read(uint16_t address){
            if (address < 0x4000){
                //bank 0, always fixed
                return rom[address];
            } else {
                //bank 1 - n, switchable
                int bankOffset = romBank * 0x4000;
                return rom[bankOffset + (address - 0x4000)];
            }
        }

        void write(uint16_t address, uint8_t value){
            //writes to rom control the mbc
            switch (mapper){
                case MapperType::NONE:
                    //no mapper writes do nothing
                    break;
                case MapperType::MBC1:
                    if (address < 0x2000){
                        //ram enable
                        ramEnabled = ((value & 0x0f) == 0x0A);
                    }
                    else if (address < 0x4000){
                        //rom bank low 5 bits
                        int bank = value & 0x1F;
                        if (bank == 0){
                            bank = 1;
                        }
                        romBank = (romBank & 0x60) | bank;
                    }
                    else if (address < 0x6000){
                        //ram bank or rom bank high bits
                        if (mbcMode == 0){
                            romBank = (romBank & 0x1F) | ((value & 0x03) << 5);
                        } else {
                            ramBank = value & 0x03;
                        }
                    }
                    else {
                        //mode select
                        mbcMode = value & 0x01;
                    }
                    break;
                case MapperType::MBC3:
                    if (address < 0x2000){
                        ramEnabled = ((value & 0x0F) == 0x0A);
                    }
                    else if (address < 0x4000){
                        int bank = value & 0x7F;
                        if (bank == 0){
                            bank = 1;
                        }
                        romBank = bank;
                    }
                    else if (address < 0x6000){
                        romBank = value & 0x0F;
                    }
                    break;
                case MapperType::MBC5:
                    if (address < 0x2000){
                        ramEnabled = ((value & 0x0F) == 0x0A);
                    }
                    else if (address < 0x3000){
                        //low 8 bits of rom bank
                        romBank = (romBank & 100) | value;
                    }
                    else if (address < 0x4000){
                        //bit 9 of rombank
                        romBank = (romBank & 0xFF) | ((value & 0x01) << 8);
                    }
                    else if (address < 0x6000){
                        ramBank = value & 0x0F;
                    }
                    break;
            }
        }

        uint8_t readRAM(uint16_t address){
            if (!ramEnabled || ramSize == 0){
                return 0xFF;
            }

            int offset = (ramBank * 0x2000) + address;
            if (offset < ramSize){
                return ram[offset];
            }

            return 0xFF;
        }

        void writeRAM(uint16_t address, uint8_t value){
            if (!ramEnabled || ramSize == 0){
                return;
            }

            int offset = (ramBank * 0x2000) + address;
            if (offset < ramSize){
                ram[offset] = value;
            }
        }

        const char* getTitle(){
            return title;
        }
    }
}