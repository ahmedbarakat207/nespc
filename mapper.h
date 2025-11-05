#pragma once

#include <cstdint>

class mapper
{
public:
    mapper(uint8_t prgBanks, uint8_t chrBanks);
    virtual ~mapper() = default; 

public:
    virtual bool cpuWrite(uint16_t addr, uint32_t &mapped_addr) = 0;
    virtual bool cpuRead(uint16_t addr, uint32_t &mapped_addr) = 0;
    virtual bool ppuWrite(uint16_t addr, uint32_t &mapped_addr) = 0;
    virtual bool ppuRead(uint16_t addr, uint32_t &mapped_addr) = 0;

protected:
    uint8_t prgBanks = 0;
    uint8_t chrBanks = 0;
};