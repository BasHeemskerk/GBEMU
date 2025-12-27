#include "included/cartridge.hpp"
#include "included/state.hpp"
#include <cstring>
#include <cstdio>

namespace gb {
    namespace cartridge {

        void initialize(GBState& state) {
            auto& cart = state.cartridge;

            cart.rom = nullptr;
            cart.ram = nullptr;
            cart.romSize = 0;
            cart.ramSize = 0;
            cart.mapper = MapperType::NONE;
            cart.romBank = 1;
            cart.ramBank = 0;
            cart.ramEnabled = false;
            cart.mbcMode = 0;
            cart.loaded = false;
            memset(cart.title, 0, sizeof(cart.title));
        }

        void cleanup(GBState& state) {
            auto& cart = state.cartridge;

            if (cart.rom) {
                delete[] cart.rom;
                cart.rom = nullptr;
            }
            if (cart.ram) {
                delete[] cart.ram;
                cart.ram = nullptr;
            }

            cart.loaded = false;
        }

        bool loadRom(GBState& state, const char* filePath) {
            auto& cart = state.cartridge;

            cleanup(state);

            FILE* file = fopen(filePath, "rb");
            if (!file) {
                return false;
            }

            fseek(file, 0, SEEK_END);
            cart.romSize = ftell(file);
            fseek(file, 0, SEEK_SET);

            cart.rom = new uint8_t[cart.romSize];
            fread(cart.rom, 1, cart.romSize, file);
            fclose(file);

            memset(cart.title, 0, sizeof(cart.title));
            for (int i = 0; i < 16; i++) {
                char c = cart.rom[0x134 + i];
                if (c >= 32 && c < 127) {
                    cart.title[i] = c;
                } else {
                    cart.title[i] = '\0';
                    break;
                }
            }

            uint8_t cartType = cart.rom[0x147];
            switch (cartType) {
                case 0x00:
                    cart.mapper = MapperType::NONE;
                    break;
                case 0x01: case 0x02: case 0x03:
                    cart.mapper = MapperType::MBC1;
                    break;
                case 0x0F: case 0x10: case 0x11: case 0x12: case 0x13:
                    cart.mapper = MapperType::MBC3;
                    break;
                case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E:
                    cart.mapper = MapperType::MBC5;
                    break;
                default:
                    cart.mapper = MapperType::NONE;
                    break;
            }

            uint8_t ramType = cart.rom[0x149];
            switch (ramType) {
                case 0x00: cart.ramSize = 0; break;
                case 0x01: cart.ramSize = 2 * 1024; break;
                case 0x02: cart.ramSize = 8 * 1024; break;
                case 0x03: cart.ramSize = 32 * 1024; break;
                case 0x04: cart.ramSize = 128 * 1024; break;
                case 0x05: cart.ramSize = 64 * 1024; break;
                default: cart.ramSize = 0; break;
            }

            if (cart.ramSize > 0) {
                cart.ram = new uint8_t[cart.ramSize];
                memset(cart.ram, 0, cart.ramSize);
            }

            cart.romBank = 1;
            cart.ramBank = 0;
            cart.ramEnabled = false;
            cart.mbcMode = 0;
            cart.loaded = true;

            return true;
        }

        uint8_t read(GBState& state, uint16_t address) {
            auto& cart = state.cartridge;

            if (!cart.rom) {
                return 0xFF;
            }

            if (address < 0x4000) {
                return cart.rom[address];
            }

            if (address < 0x8000) {
                int bankOffset = cart.romBank * 0x4000;
                return cart.rom[bankOffset + (address - 0x4000)];
            }

            return 0xFF;
        }

        void write(GBState& state, uint16_t address, uint8_t value) {
            auto& cart = state.cartridge;

            switch (cart.mapper) {
                case MapperType::MBC1:
                    if (address < 0x2000) {
                        cart.ramEnabled = ((value & 0x0F) == 0x0A);
                    }
                    else if (address < 0x4000) {
                        int bank = value & 0x1F;
                        if (bank == 0) bank = 1;
                        cart.romBank = (cart.romBank & 0x60) | bank;
                    }
                    else if (address < 0x6000) {
                        if (cart.mbcMode == 0) {
                            cart.romBank = (cart.romBank & 0x1F) | ((value & 0x03) << 5);
                        } else {
                            cart.ramBank = value & 0x03;
                        }
                    }
                    else if (address < 0x8000) {
                        cart.mbcMode = value & 0x01;
                    }
                    break;

                case MapperType::MBC3:
                    if (address < 0x2000) {
                        cart.ramEnabled = ((value & 0x0F) == 0x0A);
                    }
                    else if (address < 0x4000) {
                        int bank = value & 0x7F;
                        if (bank == 0) bank = 1;
                        cart.romBank = bank;
                    }
                    else if (address < 0x6000) {
                        cart.ramBank = value & 0x03;
                    }
                    break;

                case MapperType::MBC5:
                    if (address < 0x2000) {
                        cart.ramEnabled = ((value & 0x0F) == 0x0A);
                    }
                    else if (address < 0x3000) {
                        cart.romBank = (cart.romBank & 0x100) | value;
                    }
                    else if (address < 0x4000) {
                        cart.romBank = (cart.romBank & 0xFF) | ((value & 0x01) << 8);
                    }
                    else if (address < 0x6000) {
                        cart.ramBank = value & 0x0F;
                    }
                    break;

                default:
                    break;
            }
        }

        uint8_t readRAM(GBState& state, uint16_t address) {
            auto& cart = state.cartridge;

            if (!cart.ramEnabled || !cart.ram) {
                return 0xFF;
            }

            int offset = (cart.ramBank * 0x2000) + (address - 0xA000);
            if (offset < cart.ramSize) {
                return cart.ram[offset];
            }

            return 0xFF;
        }

        void writeRAM(GBState& state, uint16_t address, uint8_t value) {
            auto& cart = state.cartridge;

            if (!cart.ramEnabled || !cart.ram) {
                return;
            }

            int offset = (cart.ramBank * 0x2000) + (address - 0xA000);
            if (offset < cart.ramSize) {
                cart.ram[offset] = value;
            }
        }

        const char* getTitle(GBState& state) {
            return state.cartridge.title;
        }

    }
}
