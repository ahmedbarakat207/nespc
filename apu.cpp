#include "apu.h"
#include <cstring>

constexpr uint8_t  apu::LengthCounter::table[32];
constexpr uint8_t  apu::Pulse::DUTY[4][8];
constexpr uint8_t  apu::Triangle::SEQ[32];
constexpr uint16_t apu::Noise::PERIOD[16];
constexpr uint16_t apu::DMC::RATE[16];

apu::apu() {
    pulse1.ch1 = true;
    reset();
}

void apu::reset() {
    pulse1   = Pulse{};  pulse1.ch1 = true;
    pulse2   = Pulse{};
    triangle = Triangle{};
    noise    = Noise{};   noise.lfsr = 1;
    dmc      = DMC{};
    clockCount   = 0;
    frameMode    = false;
    frameIRQ     = false;
    frameStep    = 0;
    frameCounter = 0;
}

void apu::clockQuarter() {
    pulse1.envelope.clock();
    pulse2.envelope.clock();
    triangle.clockLinear();
    noise.envelope.clock();
}

void apu::clockHalf() {
    clockQuarter();
    pulse1.length.clock();
    pulse2.length.clock();
    triangle.length.clock();
    noise.length.clock();
    pulse1.sweep.clock(pulse1.timerLoad, true);
    pulse2.sweep.clock(pulse2.timerLoad, false);
}

void apu::clockFrameCounter() {
    if (!frameMode) {
        switch (frameStep) {
            case 3728:  clockQuarter(); break;
            case 7456:  clockHalf();    break;
            case 11185: clockQuarter(); break;
            case 14914:
                clockHalf();
                if (!frameIRQ) frameIRQ = true;
                break;
            case 14915: frameStep = 0; return;
        }
    } else {
        switch (frameStep) {
            case 3728:  clockQuarter(); break;
            case 7456:  clockHalf();    break;
            case 11185: clockQuarter(); break;
            case 18640: clockHalf();    break;
            case 18641: frameStep = 0; return;
        }
    }
    frameStep++;
}

void apu::clock() {
    bool oddClock = (clockCount & 1);

    if (oddClock) {
        pulse1.clockTimer();
        pulse2.clockTimer();
        noise.clockTimer();
    }
    triangle.clockTimer();
    clockFrameCounter();
    clockCount++;
}

void apu::cpuWrite(uint16_t addr, uint8_t data) {
    switch (addr) {
    case 0x4000:
        pulse1.duty              = (data >> 6) & 3;
        pulse1.length.halt       = (data >> 5) & 1;
        pulse1.envelope.loop     = (data >> 5) & 1;
        pulse1.envelope.constant = (data >> 4) & 1;
        pulse1.envelope.volume   = data & 0x0F;
        break;
    case 0x4001:
        pulse1.sweep.enabled = (data >> 7) & 1;
        pulse1.sweep.period  = (data >> 4) & 7;
        pulse1.sweep.negate  = (data >> 3) & 1;
        pulse1.sweep.shift   = data & 7;
        pulse1.sweep.reload  = true;
        break;
    case 0x4002:
        pulse1.timerLoad = (pulse1.timerLoad & 0xFF00) | data;
        break;
    case 0x4003:
        pulse1.timerLoad       = (pulse1.timerLoad & 0x00FF) | ((data & 7) << 8);
        pulse1.timer           = pulse1.timerLoad;
        pulse1.dutyPos         = 0;
        pulse1.envelope.start  = true;
        if (pulse1.enabled) pulse1.length.load(data >> 3);
        break;
    case 0x4004:
        pulse2.duty              = (data >> 6) & 3;
        pulse2.length.halt       = (data >> 5) & 1;
        pulse2.envelope.loop     = (data >> 5) & 1;
        pulse2.envelope.constant = (data >> 4) & 1;
        pulse2.envelope.volume   = data & 0x0F;
        break;
    case 0x4005:
        pulse2.sweep.enabled = (data >> 7) & 1;
        pulse2.sweep.period  = (data >> 4) & 7;
        pulse2.sweep.negate  = (data >> 3) & 1;
        pulse2.sweep.shift   = data & 7;
        pulse2.sweep.reload  = true;
        break;
    case 0x4006:
        pulse2.timerLoad = (pulse2.timerLoad & 0xFF00) | data;
        break;
    case 0x4007:
        pulse2.timerLoad       = (pulse2.timerLoad & 0x00FF) | ((data & 7) << 8);
        pulse2.timer           = pulse2.timerLoad;
        pulse2.dutyPos         = 0;
        pulse2.envelope.start  = true;
        if (pulse2.enabled) pulse2.length.load(data >> 3);
        break;
    case 0x4008:
        triangle.control     = (data >> 7) & 1;
        triangle.length.halt = (data >> 7) & 1;
        triangle.linearLoad  = data & 0x7F;
        break;
    case 0x400A:
        triangle.timerLoad = (triangle.timerLoad & 0xFF00) | data;
        break;
    case 0x400B:
        triangle.timerLoad      = (triangle.timerLoad & 0x00FF) | ((data & 7) << 8);
        triangle.timer          = triangle.timerLoad;
        triangle.linearReload   = true;
        if (triangle.enabled) triangle.length.load(data >> 3);
        break;
    case 0x400C:
        noise.length.halt       = (data >> 5) & 1;
        noise.envelope.loop     = (data >> 5) & 1;
        noise.envelope.constant = (data >> 4) & 1;
        noise.envelope.volume   = data & 0x0F;
        break;
    case 0x400E:
        noise.mode      = (data >> 7) & 1;
        noise.timerLoad = Noise::PERIOD[data & 0x0F];
        break;
    case 0x400F:
        noise.envelope.start = true;
        if (noise.enabled) noise.length.load(data >> 3);
        break;
    case 0x4010:
        dmc.irq      = (data >> 7) & 1;
        dmc.loop     = (data >> 6) & 1;
        dmc.freq     = DMC::RATE[data & 0x0F];
        break;
    case 0x4011:
        dmc.output = data & 0x7F;
        break;
    case 0x4012:
        dmc.addrLoad = 0xC000 | ((uint16_t)data << 6);
        break;
    case 0x4013:
        dmc.bytesLoad = ((uint16_t)data << 4) + 1;
        break;
    case 0x4015:
        pulse1.enabled   = data & 0x01; if (!pulse1.enabled)   pulse1.length.counter   = 0;
        pulse2.enabled   = data & 0x02; if (!pulse2.enabled)   pulse2.length.counter   = 0;
        triangle.enabled = data & 0x04; if (!triangle.enabled) triangle.length.counter = 0;
        noise.enabled    = data & 0x08; if (!noise.enabled)    noise.length.counter    = 0;
        dmc.enabled      = data & 0x10;
        if (!dmc.enabled) dmc.bytesLeft = 0;
        else if (dmc.bytesLeft == 0) { dmc.addr = dmc.addrLoad; dmc.bytesLeft = dmc.bytesLoad; }
        break;
    case 0x4017:
        frameMode = (data >> 7) & 1;
        frameIRQ  = !((data >> 6) & 1);
        frameStep = 0;
        if (frameMode) clockHalf();
        break;
    }
}

uint8_t apu::cpuRead(uint16_t addr) {
    if (addr == 0x4015) {
        uint8_t s = 0;
        if (pulse1.length.counter   > 0) s |= 0x01;
        if (pulse2.length.counter   > 0) s |= 0x02;
        if (triangle.length.counter > 0) s |= 0x04;
        if (noise.length.counter    > 0) s |= 0x08;
        if (dmc.bytesLeft           > 0) s |= 0x10;
        if (frameIRQ)                    s |= 0x40;
        frameIRQ = false;
        return s;
    }
    return 0;
}

float apu::getOutput() {
    float p1 = pulse1.output();
    float p2 = pulse2.output();
    float tr = triangle.output();
    float no = noise.output();
    float dm = dmc.getOutput();

    float pulse_out = 0.0f;
    if (p1 + p2 > 0.0f)
        pulse_out = 95.88f / (8128.0f / (p1 + p2) + 100.0f);

    float tnd_out = 0.0f;
    float tnd = tr / 8227.0f + no / 12241.0f + dm / 22638.0f;
    if (tnd > 0.0f)
        tnd_out = 159.79f / (1.0f / tnd + 100.0f);

    return pulse_out + tnd_out;
}
