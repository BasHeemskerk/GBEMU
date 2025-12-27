#include "included/apu.hpp"
#include "included/state.hpp"
#include <cstring>

namespace gb {
    namespace apu {

        const uint8_t dutyPatterns[4] = {
            0b00000001,
            0b00000011,
            0b00001111,
            0b11111100
        };

        void initialize(GBState& state) {
            auto& apu = state.apu;

            memset(apu.audioBuffer, 0, sizeof(apu.audioBuffer));
            apu.bufferPosition = 0;
            apu.sampleCycles = 0;
            apu.frameSequencerCycles = 0;
            apu.frameSequencerStep = 0;

            auto& ch1 = apu.ch1;
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

            auto& ch2 = apu.ch2;
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

            auto& ch3 = apu.ch3;
            ch3.enabled = false;
            ch3.dacEnabled = false;
            ch3.frequency = 0;
            ch3.frequencyTimer = 0;
            ch3.position = 0;
            ch3.volume = 0;
            ch3.lengthCounter = 0;

            auto& ch4 = apu.ch4;
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

            apu.masterEnable = true;
            apu.masterVolumeLeft = 7;
            apu.masterVolumeRight = 7;

            apu.ch1Left = apu.ch1Right = true;
            apu.ch2Left = apu.ch2Right = true;
            apu.ch3Left = apu.ch3Right = true;
            apu.ch4Left = apu.ch4Right = true;
        }

        void tick(GBState& state, int cycles) {
            auto& apu = state.apu;

            if (!apu.masterEnable) {
                return;
            }

            apu.frameSequencerCycles += cycles;
            while (apu.frameSequencerCycles >= 8192) {
                apu.frameSequencerCycles -= 8192;
                tickFrameSequencer(state);
            }

            apu.sampleCycles += cycles;
            while (apu.sampleCycles >= CYCLES_PER_SAMPLE) {
                apu.sampleCycles -= CYCLES_PER_SAMPLE;
                generateSample(state);
            }
        }

        void tickFrameSequencer(GBState& state) {
            auto& apu = state.apu;
            auto& io = state.memory.io;
            auto& ch1 = apu.ch1;
            auto& ch2 = apu.ch2;
            auto& ch3 = apu.ch3;
            auto& ch4 = apu.ch4;

            switch (apu.frameSequencerStep) {
                case 0:
                case 4:
                    if (ch1.lengthCounter > 0 && (io[0x14] & 0x40)) {
                        ch1.lengthCounter--;
                        if (ch1.lengthCounter == 0) ch1.enabled = false;
                    }
                    if (ch2.lengthCounter > 0 && (io[0x19] & 0x40)) {
                        ch2.lengthCounter--;
                        if (ch2.lengthCounter == 0) ch2.enabled = false;
                    }
                    if (ch3.lengthCounter > 0 && (io[0x1E] & 0x40)) {
                        ch3.lengthCounter--;
                        if (ch3.lengthCounter == 0) ch3.enabled = false;
                    }
                    if (ch4.lengthCounter > 0 && (io[0x23] & 0x40)) {
                        ch4.lengthCounter--;
                        if (ch4.lengthCounter == 0) ch4.enabled = false;
                    }
                    break;

                case 2:
                case 6:
                    if (ch1.lengthCounter > 0 && (io[0x14] & 0x40)) {
                        ch1.lengthCounter--;
                        if (ch1.lengthCounter == 0) ch1.enabled = false;
                    }
                    if (ch2.lengthCounter > 0 && (io[0x19] & 0x40)) {
                        ch2.lengthCounter--;
                        if (ch2.lengthCounter == 0) ch2.enabled = false;
                    }
                    if (ch3.lengthCounter > 0 && (io[0x1E] & 0x40)) {
                        ch3.lengthCounter--;
                        if (ch3.lengthCounter == 0) ch3.enabled = false;
                    }
                    if (ch4.lengthCounter > 0 && (io[0x23] & 0x40)) {
                        ch4.lengthCounter--;
                        if (ch4.lengthCounter == 0) ch4.enabled = false;
                    }

                    if (ch1.sweepPeriod > 0) {
                        ch1.sweepTimer--;
                        if (ch1.sweepTimer <= 0) {
                            ch1.sweepTimer = ch1.sweepPeriod > 0 ? ch1.sweepPeriod : 8;

                            if (ch1.sweepPeriod > 0) {
                                int newFreq = ch1.shadowFrequency >> ch1.sweepShift;
                                if (ch1.sweepNegate) {
                                    newFreq = ch1.shadowFrequency - newFreq;
                                } else {
                                    newFreq = ch1.shadowFrequency + newFreq;
                                }

                                if (newFreq > 2047) {
                                    ch1.enabled = false;
                                } else if (ch1.sweepShift > 0) {
                                    ch1.shadowFrequency = newFreq;
                                    ch1.frequency = newFreq;
                                }
                            }
                        }
                    }
                    break;

                case 7:
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

            apu.frameSequencerStep = (apu.frameSequencerStep + 1) & 7;
        }

        void tickChannel1(GBState& state) {
            auto& ch1 = state.apu.ch1;

            ch1.frequencyTimer--;
            if (ch1.frequencyTimer <= 0) {
                ch1.frequencyTimer = (2048 - ch1.frequency) * 4;
                ch1.dutyPosition = (ch1.dutyPosition + 1) & 7;
            }
        }

        void tickChannel2(GBState& state) {
            auto& ch2 = state.apu.ch2;

            ch2.frequencyTimer--;
            if (ch2.frequencyTimer <= 0) {
                ch2.frequencyTimer = (2048 - ch2.frequency) * 4;
                ch2.dutyPosition = (ch2.dutyPosition + 1) & 7;
            }
        }

        void tickChannel3(GBState& state) {
            auto& ch3 = state.apu.ch3;

            ch3.frequencyTimer--;
            if (ch3.frequencyTimer <= 0) {
                ch3.frequencyTimer = (2048 - ch3.frequency) * 2;
                ch3.position = (ch3.position + 1) & 31;
            }
        }

        void tickChannel4(GBState& state) {
            auto& ch4 = state.apu.ch4;

            ch4.frequencyTimer--;
            if (ch4.frequencyTimer <= 0) {
                int divisor = ch4.divisor == 0 ? 8 : ch4.divisor * 16;
                ch4.frequencyTimer = divisor << ch4.shiftAmount;

                uint8_t bit = (ch4.lfsr & 1) ^ ((ch4.lfsr >> 1) & 1);
                ch4.lfsr = (ch4.lfsr >> 1) | (bit << 14);

                if (ch4.widthMode) {
                    ch4.lfsr &= ~0x40;
                    ch4.lfsr |= bit << 6;
                }
            }
        }

        void generateSample(GBState& state) {
            auto& apu = state.apu;
            auto& mem = state.memory;

            tickChannel1(state);
            tickChannel2(state);
            tickChannel3(state);
            tickChannel4(state);

            int16_t left = 0;
            int16_t right = 0;

            if (apu.ch1.enabled) {
                uint8_t duty = dutyPatterns[apu.ch1.duty];
                int sample = (duty >> apu.ch1.dutyPosition) & 1 ? apu.ch1.volume : -apu.ch1.volume;
                if (apu.ch1Left) left += sample;
                if (apu.ch1Right) right += sample;
            }

            if (apu.ch2.enabled) {
                uint8_t duty = dutyPatterns[apu.ch2.duty];
                int sample = (duty >> apu.ch2.dutyPosition) & 1 ? apu.ch2.volume : -apu.ch2.volume;
                if (apu.ch2Left) left += sample;
                if (apu.ch2Right) right += sample;
            }

            if (apu.ch3.enabled && apu.ch3.dacEnabled) {
                uint8_t sampleByte = mem.io[0x30 + (apu.ch3.position / 2)];
                uint8_t sample4bit = (apu.ch3.position & 1) ? (sampleByte & 0x0F) : (sampleByte >> 4);

                int shift = 0;
                switch (apu.ch3.volume) {
                    case 0: shift = 4; break;
                    case 1: shift = 0; break;
                    case 2: shift = 1; break;
                    case 3: shift = 2; break;
                }

                int sample = (sample4bit >> shift) - 8;
                if (apu.ch3Left) left += sample;
                if (apu.ch3Right) right += sample;
            }

            if (apu.ch4.enabled) {
                int sample = (apu.ch4.lfsr & 1) ? -apu.ch4.volume : apu.ch4.volume;
                if (apu.ch4Left) left += sample;
                if (apu.ch4Right) right += sample;
            }

            left = (left * apu.masterVolumeLeft * 64);
            right = (right * apu.masterVolumeRight * 64);

            if (apu.bufferPosition < APUState::BUFFER_SIZE) {
                apu.audioBuffer[apu.bufferPosition * 2] = left;
                apu.audioBuffer[apu.bufferPosition * 2 + 1] = right;
                apu.bufferPosition++;
            }
        }

        void writeRegister(GBState& state, uint8_t reg, uint8_t value) {
            auto& apu = state.apu;
            auto& io = state.memory.io;
            auto& ch1 = apu.ch1;
            auto& ch2 = apu.ch2;
            auto& ch3 = apu.ch3;
            auto& ch4 = apu.ch4;

            io[reg] = value;

            switch (reg) {
                case 0x10:
                    ch1.sweepPeriod = (value >> 4) & 0x07;
                    ch1.sweepNegate = value & 0x08;
                    ch1.sweepShift = value & 0x07;
                    break;

                case 0x11:
                    ch1.duty = (value >> 6) & 0x03;
                    ch1.lengthCounter = 64 - (value & 0x3F);
                    break;

                case 0x12:
                    ch1.volume = (value >> 4) & 0x0F;
                    ch1.envelopeIncrease = value & 0x08;
                    ch1.envelopePeriod = value & 0x07;
                    ch1.envelopeTimer = ch1.envelopePeriod;
                    if ((value & 0xF8) == 0) {
                        ch1.enabled = false;
                    }
                    break;

                case 0x13:
                    ch1.frequency = (ch1.frequency & 0x700) | value;
                    break;

                case 0x14:
                    ch1.frequency = (ch1.frequency & 0xFF) | ((value & 0x07) << 8);
                    if (value & 0x80) {
                        ch1.enabled = true;
                        if (ch1.lengthCounter == 0) ch1.lengthCounter = 64;
                        ch1.frequencyTimer = (2048 - ch1.frequency) * 4;
                        ch1.envelopeTimer = ch1.envelopePeriod;
                        ch1.volume = io[0x12] >> 4;
                        ch1.shadowFrequency = ch1.frequency;
                        ch1.sweepTimer = ch1.sweepPeriod > 0 ? ch1.sweepPeriod : 8;
                    }
                    break;

                case 0x16:
                    ch2.duty = (value >> 6) & 0x03;
                    ch2.lengthCounter = 64 - (value & 0x3F);
                    break;

                case 0x17:
                    ch2.volume = (value >> 4) & 0x0F;
                    ch2.envelopeIncrease = value & 0x08;
                    ch2.envelopePeriod = value & 0x07;
                    ch2.envelopeTimer = ch2.envelopePeriod;
                    if ((value & 0xF8) == 0) {
                        ch2.enabled = false;
                    }
                    break;

                case 0x18:
                    ch2.frequency = (ch2.frequency & 0x700) | value;
                    break;

                case 0x19:
                    ch2.frequency = (ch2.frequency & 0xFF) | ((value & 0x07) << 8);
                    if (value & 0x80) {
                        ch2.enabled = true;
                        if (ch2.lengthCounter == 0) ch2.lengthCounter = 64;
                        ch2.frequencyTimer = (2048 - ch2.frequency) * 4;
                        ch2.envelopeTimer = ch2.envelopePeriod;
                        ch2.volume = io[0x17] >> 4;
                    }
                    break;

                case 0x1A:
                    ch3.dacEnabled = value & 0x80;
                    if (!ch3.dacEnabled) {
                        ch3.enabled = false;
                    }
                    break;

                case 0x1B:
                    ch3.lengthCounter = 256 - value;
                    break;

                case 0x1C:
                    ch3.volume = (value >> 5) & 0x03;
                    break;

                case 0x1D:
                    ch3.frequency = (ch3.frequency & 0x700) | value;
                    break;

                case 0x1E:
                    ch3.frequency = (ch3.frequency & 0xFF) | ((value & 0x07) << 8);
                    if (value & 0x80) {
                        ch3.enabled = true;
                        if (ch3.lengthCounter == 0) ch3.lengthCounter = 256;
                        ch3.frequencyTimer = (2048 - ch3.frequency) * 2;
                        ch3.position = 0;
                    }
                    break;

                case 0x20:
                    ch4.lengthCounter = 64 - (value & 0x3F);
                    break;

                case 0x21:
                    ch4.volume = (value >> 4) & 0x0F;
                    ch4.envelopeIncrease = value & 0x08;
                    ch4.envelopePeriod = value & 0x07;
                    ch4.envelopeTimer = ch4.envelopePeriod;
                    if ((value & 0xF8) == 0) {
                        ch4.enabled = false;
                    }
                    break;

                case 0x22:
                    ch4.shiftAmount = (value >> 4) & 0x0F;
                    ch4.widthMode = value & 0x08;
                    ch4.divisor = value & 0x07;
                    break;

                case 0x23:
                    if (value & 0x80) {
                        ch4.enabled = true;
                        if (ch4.lengthCounter == 0) ch4.lengthCounter = 64;
                        int divisor = ch4.divisor == 0 ? 8 : ch4.divisor * 16;
                        ch4.frequencyTimer = divisor << ch4.shiftAmount;
                        ch4.envelopeTimer = ch4.envelopePeriod;
                        ch4.volume = io[0x21] >> 4;
                        ch4.lfsr = 0x7FFF;
                    }
                    break;

                case 0x24:
                    apu.masterVolumeLeft = (value >> 4) & 0x07;
                    apu.masterVolumeRight = value & 0x07;
                    break;

                case 0x25:
                    apu.ch4Left = value & 0x80;
                    apu.ch3Left = value & 0x40;
                    apu.ch2Left = value & 0x20;
                    apu.ch1Left = value & 0x10;
                    apu.ch4Right = value & 0x08;
                    apu.ch3Right = value & 0x04;
                    apu.ch2Right = value & 0x02;
                    apu.ch1Right = value & 0x01;
                    break;

                case 0x26:
                    apu.masterEnable = value & 0x80;
                    if (!apu.masterEnable) {
                        ch1.enabled = false;
                        ch2.enabled = false;
                        ch3.enabled = false;
                        ch4.enabled = false;
                    }
                    break;
            }
        }

        uint8_t readRegister(GBState& state, uint8_t reg) {
            auto& apu = state.apu;
            auto& io = state.memory.io;

            switch (reg) {
                case 0x26: {
                    uint8_t status = apu.masterEnable ? 0x80 : 0x00;
                    if (apu.ch1.enabled) status |= 0x01;
                    if (apu.ch2.enabled) status |= 0x02;
                    if (apu.ch3.enabled) status |= 0x04;
                    if (apu.ch4.enabled) status |= 0x08;
                    return status | 0x70;
                }
                default:
                    return io[reg];
            }
        }

        // Platform-specific audio (3DS) - stubs for now
        void initAudio() {
            // TODO: Initialize 3DS audio (ndsp)
        }

        void exitAudio() {
            // TODO: Cleanup 3DS audio
        }

    }
}
