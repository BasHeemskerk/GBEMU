#include "included/timer.hpp"
#include "included/memory.hpp"

namespace gb{
    namespace timer{
        uint16_t divCounter;
        uint16_t timaCounter;

        void initialize(){
            divCounter = 0;
            timaCounter = 0;
        }

        void tick(int cycles){
            // increments every 256 cycles
            divCounter += cycles;
            
            while (divCounter >= 256){
                divCounter -= 256;
                memory::io[memory::IO_DIV]++;
            }

            // check if timer is enabled (bit 2 of TAC)
            uint8_t tac = memory::io[memory::IO_TAC];
            if (!(tac & 0x04)){
                return;
            }

            // get timer frequency from TAC bits 0-1
            // 00 = 4096 Hz   (every 1024 cycles)
            // 01 = 262144 Hz (every 16 cycles)
            // 10 = 65536 Hz  (every 64 cycles)
            // 11 = 16384 Hz  (every 256 cycles)
            int threshold;
            switch (tac & 0x03){
                case 0: threshold = 1024; break;
                case 1: threshold = 16; break;
                case 2: threshold = 64; break;
                case 3: threshold = 256; break;
                default: threshold = 1024; break;
            }

            //update time
            timaCounter += cycles;
            while (timaCounter >= threshold){
                timaCounter -= threshold;

                memory::io[memory::IO_TIMA]++;

                //check for overflow
                if (memory::io[memory::IO_TIMA] == 0){
                    //reload from tma
                    memory::io[memory::IO_TIMA] = memory::io[memory::IO_TMA];

                    //request timer interrupt (bit 2 of IF)
                    memory::io[memory::IO_IF] |= 0x04;
                }
            }
        }
    }
}