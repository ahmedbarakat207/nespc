#pragma once
#include <cstdint>
#include <array>
#include <memory>
#include "6502.h"
#include "ppu.h"
#include "apu.h"
#include "rom.h"

class bus {
public:
    bus();
    ~bus();

    void    cpuWrite(uint16_t addr, uint8_t data);
    uint8_t cpuRead (uint16_t addr);

    void insert_cartridge(const std::shared_ptr<rom> &cartridge);
    void reset();
    void clock();

public:
    cpu6502              cpu;
    ppu*                 picture  = nullptr;
    apu                  sound;
    std::array<uint8_t, 2048> cpuRam{};
    std::shared_ptr<rom> cartridge;

    uint8_t controller[2] = {0, 0};

    float   audioSample = 0.0f;
    bool    audioReady  = false;

private:
    uint32_t nSystemClockCounter = 0;
    uint8_t  controller_state[2] = {0, 0};
    bool     dma_transfer = false;
    uint8_t  dma_page     = 0x00;
    uint8_t  dma_addr     = 0x00;
    uint8_t  dma_data     = 0x00;
    bool     dma_dummy    = true;

    static constexpr double CPU_FREQ    = 1789773.0;
    static constexpr double SAMPLE_RATE = 44100.0;
    double   audioAccum  = 0.0;
};
