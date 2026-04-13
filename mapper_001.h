#pragma once
#include "mapper.h"

class mapper_001 : public mapper {
public:
    mapper_001(uint8_t prgBanks, uint8_t chrBanks);
    ~mapper_001() override;

    bool cpuRead(uint16_t addr, uint32_t &mapped_addr) override;
    bool cpuWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) override;
    bool ppuRead(uint16_t addr, uint32_t &mapped_addr) override;
    bool ppuWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) override;
    void setMirroringCallback(std::function<void(uint8_t)> cb) override;

private:
    std::function<void(uint8_t)> mirrorCallback;
    uint8_t chrBank0 = 0;
    uint8_t chrBank1 = 0;
    uint8_t prgBank = 0;
    uint8_t shiftRegister = 0x10;
    uint8_t controlRegister = 0x0C;
};
