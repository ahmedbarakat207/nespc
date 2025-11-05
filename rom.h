#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include "mapper.h"  

class rom
{
public:
    rom(const std::string &filename);
    ~rom();
    
public:
    enum MIRROR {
        HORIZONTAL,
        VERTICAL,
        ONESCREEN_LO,
        ONESCREEN_HI,
    } mirror = HORIZONTAL;
    std::vector<uint8_t> prgMemory;
    std::vector<uint8_t> chrMemory;

    uint8_t mapperID = 0;
    uint8_t prgBanks = 0;  
    uint8_t chrBanks = 0;  

    std::shared_ptr<mapper> pMapper;  

    uint8_t subMapperID = 0;
    bool verticalMirroring = false;
    bool horizontalMirroring = false;
    bool fourScreenMirroring = false;
    bool batteryBackedRAM = false;
    bool trainer = false;
    bool ignoreMirroring = false;    

public:
    bool cpuWrite(uint16_t addr, uint8_t data);
    bool cpuRead(uint16_t addr, uint8_t &data);
    
    bool ppuWrite(uint16_t addr, uint8_t data);
    bool ppuRead(uint16_t addr, uint8_t &data);
};