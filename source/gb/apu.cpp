#include "included/apu.hpp"
#include "included/memory.hpp"
#include <3ds.h>
#include <cstring>

namespace gb{
    namespace apu{
        //audio buffer
        int16_t audioBuffer[BUFFER_SIZE * 2];
        int bufferPosition;

        //timing
        int sampleCycles;
        int frameSequencerCycles;
        int frameSequencerStep;

        //channels
        Channel1 ch1;
        Channel2 ch2;
        Channel3 ch3;
        Channel4 ch4;

        //master control
        bool masterEnable;
        int masterVolumeLeft;
        int masterVolumeRight;

        //panning
        bool ch1Left, ch1Right;
        bool ch2Left, ch2Right;
        bool ch3Left, ch3Right;
        bool ch4Left, ch4Right;

        //3ds audio
        ndspWaveBuf waveBuf[2];
        int16_t* audioBufferData[2];
        int currentBuffer = 0;

        //duty cycle patterns for square waves
        const uint8_t dutyPatterns[4] = {
            0b00000001,  // 12.5% duty
            0b00000011,  // 25% duty
            0b00001111,  // 50% duty
            0b11111100   // 75% duty
        };

        void initialize(){
            memset(audioBuffer, 0, sizeof(audioBuffer));
            bufferPosition = 0;
            sampleCycles = 0;
            frameSequencerCycles = 0;
            frameSequencerStep = 0;

            // reset channel 1
            ch1.enabled = false;
            ch1.frequency = 0;
            ch1.frequencyTimer = 0;
            ch1.dutyPosition = 0;
            ch1.duty = 0;
            ch1.volume = 0;
            ch1.envelopeTimer = 0;
            ch1.envelopePeriod = 0;
            ch1.envelopeIncrease = false;
            ch1.sweepTimer = 0;
            ch1.sweepPeriod = 0;
            ch1.sweepNegate = false;
            ch1.sweepShift = 0;
            ch1.shadowFrequency = 0;
            ch1.lengthCounter = 0;

            // reset channel 2
            ch2.enabled = false;
            ch2.frequency = 0;
            ch2.frequencyTimer = 0;
            ch2.dutyPosition = 0;
            ch2.duty = 0;
            ch2.volume = 0;
            ch2.envelopeTimer = 0;
            ch2.envelopePeriod = 0;
            ch2.envelopeIncrease = false;
            ch2.lengthCounter = 0;

            // reset channel 3
            ch3.enabled = false;
            ch3.dacEnabled = false;
            ch3.frequency = 0;
            ch3.frequencyTimer = 0;
            ch3.position = 0;
            ch3.volume = 0;
            ch3.lengthCounter = 0;

            // reset channel 4
            ch4.enabled = false;
            ch4.volume = 0;
            ch4.envelopeTimer = 0;
            ch4.envelopePeriod = 0;
            ch4.envelopeIncrease = false;
            ch4.frequencyTimer = 0;
            ch4.divisor = 0;
            ch4.shiftAmount = 0;
            ch4.widthMode = false;
            ch4.lfsr = 0x7FFF;
            ch4.lengthCounter = 0;

            // master control
            masterEnable = true;
            masterVolumeLeft = 7;
            masterVolumeRight = 7;

            // panning - all channels to both speakers
            ch1Left = ch1Right = true;
            ch2Left = ch2Right = true;
            ch3Left = ch3Right = true;
            ch4Left = ch4Right = true;
        }

        void initAudio() {
            Result rc = ndspInit();
            if (R_FAILED(rc)) {
                // audio init failed, disable audio
                masterEnable = false;
                return;
            }
            
            ndspSetOutputMode(NDSP_OUTPUT_STEREO);
            ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
            ndspChnSetRate(0, SAMPLE_RATE);
            ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);

            for (int i = 0; i < 2; i++) {
                audioBufferData[i] = (int16_t*)linearAlloc(BUFFER_SIZE * 2 * sizeof(int16_t));
                memset(audioBufferData[i], 0, BUFFER_SIZE * 2 * sizeof(int16_t));

                waveBuf[i].data_vaddr = audioBufferData[i];
                waveBuf[i].nsamples = BUFFER_SIZE;
                waveBuf[i].looping = false;
                waveBuf[i].status = NDSP_WBUF_FREE;
            }

            currentBuffer = 0;
        }

        void exitAudio() {
            // only exit if audio was initialized
            if (audioBufferData[0] != nullptr) {
                ndspExit();
                for (int i = 0; i < 2; i++) {
                    if (audioBufferData[i]) {
                        linearFree(audioBufferData[i]);
                        audioBufferData[i] = nullptr;
                    }
                }
            }
        }

        void tick(int cycles){
            if (!masterEnable){
                return;
            }

            frameSequencerCycles += cycles;
            while (frameSequencerCycles >= 8192){
                frameSequencerCycles -= 8192;
                tickFrameSequencer();
            }

            sampleCycles += cycles;
            while (sampleCycles >= CYCLES_PER_SAMPLE){
                sampleCycles -= CYCLES_PER_SAMPLE;
                generateSample();
            }
        }

        void tickFrameSequencer(){
            switch (frameSequencerStep) {
                case 0:
                case 4:
                    // length counter
                    if (ch1.lengthCounter > 0 && (memory::io[0x14] & 0x40)) {
                        ch1.lengthCounter--;
                        if (ch1.lengthCounter == 0) ch1.enabled = false;
                    }
                    if (ch2.lengthCounter > 0 && (memory::io[0x19] & 0x40)) {
                        ch2.lengthCounter--;
                        if (ch2.lengthCounter == 0) ch2.enabled = false;
                    }
                    if (ch3.lengthCounter > 0 && (memory::io[0x1E] & 0x40)) {
                        ch3.lengthCounter--;
                        if (ch3.lengthCounter == 0) ch3.enabled = false;
                    }
                    if (ch4.lengthCounter > 0 && (memory::io[0x23] & 0x40)) {
                        ch4.lengthCounter--;
                        if (ch4.lengthCounter == 0) ch4.enabled = false;
                    }
                    break;

                case 2:
                case 6:
                    // length counter
                    if (ch1.lengthCounter > 0 && (memory::io[0x14] & 0x40)) {
                        ch1.lengthCounter--;
                        if (ch1.lengthCounter == 0) ch1.enabled = false;
                    }
                    if (ch2.lengthCounter > 0 && (memory::io[0x19] & 0x40)) {
                        ch2.lengthCounter--;
                        if (ch2.lengthCounter == 0) ch2.enabled = false;
                    }
                    if (ch3.lengthCounter > 0 && (memory::io[0x1E] & 0x40)) {
                        ch3.lengthCounter--;
                        if (ch3.lengthCounter == 0) ch3.enabled = false;
                    }
                    if (ch4.lengthCounter > 0 && (memory::io[0x23] & 0x40)) {
                        ch4.lengthCounter--;
                        if (ch4.lengthCounter == 0) ch4.enabled = false;
                    }

                    // sweep (channel 1 only)
                    if (ch1.sweepPeriod > 0) {
                        ch1.sweepTimer--;
                        if (ch1.sweepTimer <= 0) {
                            ch1.sweepTimer = ch1.sweepPeriod;
                            if (ch1.sweepShift > 0) {
                                int newFreq = ch1.shadowFrequency >> ch1.sweepShift;
                                if (ch1.sweepNegate) {
                                    newFreq = ch1.shadowFrequency - newFreq;
                                } else {
                                    newFreq = ch1.shadowFrequency + newFreq;
                                }

                                if (newFreq > 2047) {
                                    ch1.enabled = false;
                                } else if (newFreq >= 0) {
                                    ch1.frequency = newFreq;
                                    ch1.shadowFrequency = newFreq;
                                }
                            }
                        }
                    }
                    break;

                case 7:
                    // envelope
                    if (ch1.envelopePeriod > 0) {
                        ch1.envelopeTimer--;
                        if (ch1.envelopeTimer <= 0) {
                            ch1.envelopeTimer = ch1.envelopePeriod;
                            if (ch1.envelopeIncrease && ch1.volume < 15) {
                                ch1.volume++;
                            } else if (!ch1.envelopeIncrease && ch1.volume > 0) {
                                ch1.volume--;
                            }
                        }
                    }
                    if (ch2.envelopePeriod > 0) {
                        ch2.envelopeTimer--;
                        if (ch2.envelopeTimer <= 0) {
                            ch2.envelopeTimer = ch2.envelopePeriod;
                            if (ch2.envelopeIncrease && ch2.volume < 15) {
                                ch2.volume++;
                            } else if (!ch2.envelopeIncrease && ch2.volume > 0) {
                                ch2.volume--;
                            }
                        }
                    }
                    if (ch4.envelopePeriod > 0) {
                        ch4.envelopeTimer--;
                        if (ch4.envelopeTimer <= 0) {
                            ch4.envelopeTimer = ch4.envelopePeriod;
                            if (ch4.envelopeIncrease && ch4.volume < 15) {
                                ch4.volume++;
                            } else if (!ch4.envelopeIncrease && ch4.volume > 0) {
                                ch4.volume--;
                            }
                        }
                    }
                    break;
            }

            frameSequencerStep = (frameSequencerStep + 1) & 7;

        }

        void tickChannel1(){
            ch1.frequencyTimer--;
            if (ch1.frequencyTimer <= 0){
                ch1.frequencyTimer = (2048 - ch1.frequency) * 4;
                ch1.dutyPosition = (ch1.dutyPosition + 1) & 7;
            }
        }

        void tickChannel2(){
            ch2.frequencyTimer--;
            if (ch2.frequencyTimer <= 0){
                ch2.frequencyTimer = (2048 - ch2.frequency) * 4;
                ch2.dutyPosition = (ch2.dutyPosition + 1) & 7;
            }
        }

        void tickChannel3(){
            ch3.frequencyTimer--;
            if (ch3.frequencyTimer <= 0){
                ch3.frequencyTimer = (2048 - ch3.frequency) * 2;
                ch3.position = (ch3.position + 1) & 31;
            }
        }

        void tickChannel4(){
            ch4.frequencyTimer--;
            if (ch4.frequencyTimer <= 0){
                //calculate timer reload
                int divisor = ch4.divisor == 0 ? 8 : ch4.divisor * 16;
                ch4.frequencyTimer = divisor << ch4.shiftAmount;

                //lfsr
                uint8_t xorResult = (ch4.lfsr & 1) ^ ((ch4.lfsr >> 1) & 1);
                ch4.lfsr = (ch4.lfsr >> 1) | (xorResult << 14);

                if (ch4.widthMode) {
                    ch4.lfsr &= ~(1 << 6);
                    ch4.lfsr |= xorResult << 6;
                }
            }
        }

        void generateSample(){
            int16_t left;
            int16_t right;

            //square with sweep
            if (ch1.enabled){
                tickChannel1();
                uint8_t duty = dutyPatterns[ch1.duty];
                int sample = (duty >> ch1.dutyPosition) & 1 ? ch1.volume : -ch1.volume;
                if (ch1Left){
                    left += sample;
                }
                if (ch1Right){
                    right += sample;
                }
            }

            //square
            if (ch2.enabled){
                tickChannel2();
                uint8_t duty = dutyPatterns[ch2.duty];
                int sample = (duty >> ch2.dutyPosition) & 1 ? ch2.volume : -ch2.volume;
                if (ch2Left){
                    left += sample;
                }
                if (ch2Right){
                    right += sample;
                }
            }

            //wave
            if (ch3.enabled & ch3.dacEnabled){
                tickChannel3();

                //wave ram is at 0xFF30-0xFF3F
                uint8_t waveByte = memory::io[0x30 + (ch3.position / 2)];
                uint8_t sample;
                if (ch3.position & 1){
                    sample = waveByte & 0x0F;
                } else {
                    sample = waveByte >> 4;
                }

                //apply volume shift
                if (ch3.volume > 0){
                    sample >>= (ch3.volume - 1);
                } else {
                    sample = 0;
                }

                int signedSample = sample - 0;
                if (ch3Left){
                    left += signedSample;
                }
                if (ch3Right){
                    right += signedSample;
                }
            }

            //noise
            if (ch4.enabled){
                tickChannel4();
                int sample = (ch4.lfsr & 1) ? -ch4.volume : ch4.volume;
                if (ch4Left){
                    left += sample;
                }
                if (ch4Right){
                    right += sample;
                }
            }

            //apply master volume
            left *= (masterVolumeLeft + 1) * 32;
            right *= (masterVolumeRight + 1) * 32;

            //clamp
            if (left > 32767) {
                left = 32767;
            }
            if (left < -32768) {
                left = -32768;
            }
            if (right > 32767) {
                right = 32767;
            }
            if (right < -32768) {
                right = -32768;
            }

            //store in buffer
            audioBuffer[bufferPosition * 2] = left;
            audioBuffer[bufferPosition * 2 + 1] = right;
            bufferPosition++;

            //flush on full buffer
            if (bufferPosition >= BUFFER_SIZE){
                flushBuffer();
                bufferPosition = 0;
            }
        }

        void flushBuffer(){
            //wait for free buffer
            while(waveBuf[currentBuffer].status != NDSP_WBUF_FREE){
                return;
            }

            //copy data
            memcpy(audioBufferData[currentBuffer], audioBuffer, BUFFER_SIZE * 2 * sizeof(int16_t));
            DSP_FlushDataCache(audioBufferData[currentBuffer], BUFFER_SIZE * 2 * sizeof(int16_t));

            //queue buffer
            ndspChnWaveBufAdd(0, &waveBuf[currentBuffer]);

            //switch the buffers
            currentBuffer = (currentBuffer + 1) % 2;
        }

        void writeRegister(uint8_t reg, uint8_t value) {
            switch (reg) {
                // NR10 - Channel 1 sweep
                case 0x10:
                    ch1.sweepPeriod = (value >> 4) & 0x07;
                    ch1.sweepNegate = value & 0x08;
                    ch1.sweepShift = value & 0x07;
                    break;

                // NR11 - Channel 1 duty and length
                case 0x11:
                    ch1.duty = (value >> 6) & 0x03;
                    ch1.lengthCounter = 64 - (value & 0x3F);
                    break;

                // NR12 - Channel 1 envelope
                case 0x12:
                    ch1.volume = (value >> 4) & 0x0F;
                    ch1.envelopeIncrease = value & 0x08;
                    ch1.envelopePeriod = value & 0x07;
                    ch1.envelopeTimer = ch1.envelopePeriod;
                    // DAC disabled if top 5 bits are 0
                    if ((value & 0xF8) == 0) {
                        ch1.enabled = false;
                    }
                    break;

                // NR13 - Channel 1 frequency low
                case 0x13:
                    ch1.frequency = (ch1.frequency & 0x700) | value;
                    break;

                // NR14 - Channel 1 frequency high + trigger
                case 0x14:
                    ch1.frequency = (ch1.frequency & 0xFF) | ((value & 0x07) << 8);
                    if (value & 0x80) {
                        // trigger
                        ch1.enabled = true;
                        if (ch1.lengthCounter == 0) ch1.lengthCounter = 64;
                        ch1.frequencyTimer = (2048 - ch1.frequency) * 4;
                        ch1.envelopeTimer = ch1.envelopePeriod;
                        ch1.volume = memory::io[0x12] >> 4;
                        ch1.shadowFrequency = ch1.frequency;
                        ch1.sweepTimer = ch1.sweepPeriod > 0 ? ch1.sweepPeriod : 8;
                    }
                    break;

                // NR21 - Channel 2 duty and length
                case 0x16:
                    ch2.duty = (value >> 6) & 0x03;
                    ch2.lengthCounter = 64 - (value & 0x3F);
                    break;

                // NR22 - Channel 2 envelope
                case 0x17:
                    ch2.volume = (value >> 4) & 0x0F;
                    ch2.envelopeIncrease = value & 0x08;
                    ch2.envelopePeriod = value & 0x07;
                    ch2.envelopeTimer = ch2.envelopePeriod;
                    if ((value & 0xF8) == 0) {
                        ch2.enabled = false;
                    }
                    break;

                // NR23 - Channel 2 frequency low
                case 0x18:
                    ch2.frequency = (ch2.frequency & 0x700) | value;
                    break;

                // NR24 - Channel 2 frequency high + trigger
                case 0x19:
                    ch2.frequency = (ch2.frequency & 0xFF) | ((value & 0x07) << 8);
                    if (value & 0x80) {
                        ch2.enabled = true;
                        if (ch2.lengthCounter == 0) ch2.lengthCounter = 64;
                        ch2.frequencyTimer = (2048 - ch2.frequency) * 4;
                        ch2.envelopeTimer = ch2.envelopePeriod;
                        ch2.volume = memory::io[0x17] >> 4;
                    }
                    break;

                // NR30 - Channel 3 enable
                case 0x1A:
                    ch3.dacEnabled = value & 0x80;
                    if (!ch3.dacEnabled) {
                        ch3.enabled = false;
                    }
                    break;

                // NR31 - Channel 3 length
                case 0x1B:
                    ch3.lengthCounter = 256 - value;
                    break;

                // NR32 - Channel 3 volume
                case 0x1C:
                    ch3.volume = (value >> 5) & 0x03;
                    break;

                // NR33 - Channel 3 frequency low
                case 0x1D:
                    ch3.frequency = (ch3.frequency & 0x700) | value;
                    break;

                // NR34 - Channel 3 frequency high + trigger
                case 0x1E:
                    ch3.frequency = (ch3.frequency & 0xFF) | ((value & 0x07) << 8);
                    if (value & 0x80) {
                        ch3.enabled = true;
                        if (ch3.lengthCounter == 0) ch3.lengthCounter = 256;
                        ch3.frequencyTimer = (2048 - ch3.frequency) * 2;
                        ch3.position = 0;
                    }
                    break;

                // NR41 - Channel 4 length
                case 0x20:
                    ch4.lengthCounter = 64 - (value & 0x3F);
                    break;

                // NR42 - Channel 4 envelope
                case 0x21:
                    ch4.volume = (value >> 4) & 0x0F;
                    ch4.envelopeIncrease = value & 0x08;
                    ch4.envelopePeriod = value & 0x07;
                    ch4.envelopeTimer = ch4.envelopePeriod;
                    if ((value & 0xF8) == 0) {
                        ch4.enabled = false;
                    }
                    break;

                // NR43 - Channel 4 frequency
                case 0x22:
                    ch4.shiftAmount = (value >> 4) & 0x0F;
                    ch4.widthMode = value & 0x08;
                    ch4.divisor = value & 0x07;
                    break;

                // NR44 - Channel 4 trigger
                case 0x23:
                    if (value & 0x80) {
                        ch4.enabled = true;
                        if (ch4.lengthCounter == 0) ch4.lengthCounter = 64;
                        int divisor = ch4.divisor == 0 ? 8 : ch4.divisor * 16;
                        ch4.frequencyTimer = divisor << ch4.shiftAmount;
                        ch4.envelopeTimer = ch4.envelopePeriod;
                        ch4.volume = memory::io[0x21] >> 4;
                        ch4.lfsr = 0x7FFF;
                    }
                    break;

                // NR50 - Master volume
                case 0x24:
                    masterVolumeLeft = (value >> 4) & 0x07;
                    masterVolumeRight = value & 0x07;
                    break;

                // NR51 - Channel panning
                case 0x25:
                    ch4Left = value & 0x80;
                    ch3Left = value & 0x40;
                    ch2Left = value & 0x20;
                    ch1Left = value & 0x10;
                    ch4Right = value & 0x08;
                    ch3Right = value & 0x04;
                    ch2Right = value & 0x02;
                    ch1Right = value & 0x01;
                    break;

                // NR52 - Master enable
                case 0x26:
                    masterEnable = value & 0x80;
                    if (!masterEnable) {
                        ch1.enabled = false;
                        ch2.enabled = false;
                        ch3.enabled = false;
                        ch4.enabled = false;
                    }
                    break;
            }
        }

        uint8_t readRegister(uint8_t reg){
            switch (reg){
                case 0x26:
                    {
                        uint8_t status = masterEnable ? 0x80 : 0x00;
                        if (ch1.enabled) status |= 0x01;
                        if (ch2.enabled) status |= 0x02;
                        if (ch3.enabled) status |= 0x04;
                        if (ch4.enabled) status |= 0x08;
                        return status | 0x70;
                    }
                default:
                    return memory::io[reg];
            }
        }
    }
}