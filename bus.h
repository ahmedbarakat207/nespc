#pragma once
#include <cstdint>
#include "6502.h"
#include "ppu.h"
#include "rom.h"

class bus{ 
    public:
        bus();
        ~bus();

    public:
        void cpuWrite(uint16_t addr, uint8_t data);
        uint8_t cpuRead(uint16_t addr);

    public:
        cpu6502 cpu;
        ppu* ppu;  // Change to pointer
        std::array<uint8_t, 2048> cpuRam;
        std::shared_ptr<rom> cartridge;

    public:
        void insert_cartridge(const std::shared_ptr<rom> &cartridge);
        void reset();
        void clock();

    private:
        uint32_t nSystemClockCounter = 0;

        void handleOAMDMA(uint8_t page);
};