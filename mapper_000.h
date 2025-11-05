#pragma once

#include "mapper.h"

class mapper_000 : public mapper
{
public:
    mapper_000(uint8_t prgBanks, uint8_t chrBanks);  
    ~mapper_000();

public:
    bool cpuWrite(uint16_t addr, uint32_t &mapped_addr);
    bool cpuRead(uint16_t addr, uint32_t &mapped_addr);
    bool ppuWrite(uint16_t addr, uint32_t &mapped_addr);
    bool ppuRead(uint16_t addr, uint32_t &mapped_addr);
};