#pragma once
#include <cstdint>
#include <cmath>

class apu {
public:
    apu();
    void reset();
    void clock();
    void cpuWrite(uint16_t addr, uint8_t data);
    uint8_t cpuRead(uint16_t addr);
    float getOutput();

private:
    struct LengthCounter {
        static constexpr uint8_t table[32] = {
            10,254,20,2,40,4,80,6,160,8,60,10,14,12,26,14,
            12,16,24,18,48,20,96,22,192,24,72,26,16,28,32,30
        };
        uint8_t counter = 0;
        bool    halt    = false;
        void load(uint8_t idx) { counter = table[idx & 0x1F]; }
        void clock()           { if (!halt && counter > 0) counter--; }
    };

    struct Envelope {
        bool    start      = false;
        bool    loop       = false;
        bool    constant   = false;
        uint8_t volume     = 0;
        uint8_t divider    = 0;
        uint8_t decay      = 0;
        void    clock() {
            if (start) {
                start   = false;
                decay   = 15;
                divider = volume;
            } else {
                if (divider == 0) {
                    divider = volume;
                    if (decay > 0)      decay--;
                    else if (loop)      decay = 15;
                } else {
                    divider--;
                }
            }
        }
        uint8_t output() const { return constant ? volume : decay; }
    };

    struct Sweep {
        bool    enabled  = false;
        bool    negate   = false;
        bool    reload   = false;
        uint8_t period   = 0;
        uint8_t shift    = 0;
        uint8_t divider  = 0;
        bool    mute     = false;
        uint16_t target  = 0;
        void compute(uint16_t timer, bool ch1) {
            uint16_t delta = timer >> shift;
            target = timer + (negate ? (ch1 ? -(int)delta - 1 : -(int)delta) : (int)delta);
            mute   = (timer < 8) || (target > 0x7FF);
        }
        bool clock(uint16_t &timer, bool ch1) {
            compute(timer, ch1);
            bool changed = false;
            if (divider == 0 && enabled && shift > 0 && !mute) {
                timer   = target;
                changed = true;
            }
            if (divider == 0 || reload) {
                divider = period;
                reload  = false;
            } else {
                divider--;
            }
            return changed;
        }
    };

    struct Pulse {
        bool     enabled    = false;
        uint8_t  duty       = 0;
        uint16_t timer      = 0;
        uint16_t timerLoad  = 0;
        uint8_t  dutyPos    = 0;
        LengthCounter length;
        Envelope      envelope;
        Sweep         sweep;
        bool          ch1   = false;

        static constexpr uint8_t DUTY[4][8] = {
            {0,1,0,0,0,0,0,0},
            {0,1,1,0,0,0,0,0},
            {0,1,1,1,1,0,0,0},
            {1,0,0,1,1,1,1,1}
        };

        void clockTimer() {
            if (timer == 0) {
                timer   = timerLoad;
                dutyPos = (dutyPos + 1) & 7;
            } else {
                timer--;
            }
        }

        float output() const {
            if (!enabled || length.counter == 0 || sweep.mute ||
                timerLoad < 8 || DUTY[duty][dutyPos] == 0)
                return 0.0f;
            return envelope.output() / 15.0f;
        }
    };

    struct Triangle {
        bool     enabled   = false;
        uint16_t timer     = 0;
        uint16_t timerLoad = 0;
        uint8_t  seqPos    = 0;
        bool     control   = false;
        uint8_t  linearLoad= 0;
        uint8_t  linear    = 0;
        bool     linearReload = false;
        LengthCounter length;

        static constexpr uint8_t SEQ[32] = {
            15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,
            0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
        };

        void clockTimer() {
            if (timer == 0) {
                timer = timerLoad;
                if (length.counter > 0 && linear > 0)
                    seqPos = (seqPos + 1) & 31;
            } else {
                timer--;
            }
        }

        void clockLinear() {
            if (linearReload)    linear = linearLoad;
            else if (linear > 0) linear--;
            if (!control)        linearReload = false;
        }

        float output() const {
            if (!enabled || length.counter == 0 || linear == 0) return 0.0f;
            if (timerLoad < 2) return 0.0f;
            return SEQ[seqPos] / 15.0f;
        }
    };

    struct Noise {
        bool     enabled  = false;
        bool     mode     = false;
        uint16_t lfsr     = 1;
        uint16_t timer    = 0;
        uint16_t timerLoad= 0;
        LengthCounter length;
        Envelope      envelope;

        static constexpr uint16_t PERIOD[16] = {
            4,8,16,32,64,96,128,160,202,254,380,508,762,1016,2034,4068
        };

        void clockTimer() {
            if (timer == 0) {
                timer = timerLoad;
                uint16_t bit = (lfsr & 1) ^ ((mode ? (lfsr >> 6) : (lfsr >> 1)) & 1);
                lfsr = (lfsr >> 1) | (bit << 14);
            } else {
                timer--;
            }
        }

        float output() const {
            if (!enabled || length.counter == 0 || (lfsr & 1)) return 0.0f;
            return envelope.output() / 15.0f;
        }
    };

    struct DMC {
        bool     enabled      = false;
        bool     irq          = false;
        bool     loop         = false;
        uint16_t freq         = 0;
        uint16_t timer        = 0;
        uint16_t addr         = 0xC000;
        uint16_t addrLoad     = 0xC000;
        uint16_t bytesLeft    = 0;
        uint16_t bytesLoad    = 0;
        uint8_t  sample       = 0;
        uint8_t  output       = 0;
        uint8_t  bitsLeft     = 8;
        uint8_t  shiftReg     = 0;
        bool     silence      = true;
        bool     sampleBuffer = false;
        uint8_t  sampleByte   = 0;

        static constexpr uint16_t RATE[16] = {
            428,380,340,320,286,254,226,214,190,160,142,128,106,84,72,54
        };

        float getOutput() const { return output / 127.0f; }
    };

    Pulse    pulse1, pulse2;
    Triangle triangle;
    Noise    noise;
    DMC      dmc;

    uint32_t clockCount  = 0;
    bool     frameMode   = false;
    bool     frameIRQ    = false;
    uint16_t frameStep   = 0;
    uint16_t frameCounter= 0;

    void clockFrameCounter();
    void clockQuarter();
    void clockHalf();
};
