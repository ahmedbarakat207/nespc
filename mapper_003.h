#pragma once
#include "mapper.h"

class mapper_003 : public mapper {
public:
    mapper_003(uint8_t prgBanks, uint8_t chrBanks);
    ~mapper_003();

    bool cpuRead (uint16_t addr, uint32_t &mapped_addr) override;
    bool cpuWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0) override;
    bool ppuRead (uint16_t addr, uint32_t &mapped_addr) override;
    bool ppuWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0) override;

private:
    uint8_t chrBank = 0;
};
