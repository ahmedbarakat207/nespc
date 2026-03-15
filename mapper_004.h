#pragma once
#include "mapper.h"
#include <vector>

class mapper_004 : public mapper {
public:
    mapper_004(uint8_t prgBanks, uint8_t chrBanks);
    ~mapper_004() override;

    bool cpuWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0) override;
    bool cpuRead(uint16_t addr, uint32_t &mapped_addr) override;
    bool ppuWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0) override;
    bool ppuRead(uint16_t addr, uint32_t &mapped_addr) override;
    void scanline() override;
    bool irqState() override;
    void irqClear() override;
    void setMirroringCallback(std::function<void(uint8_t)> cb) override;

    void updateBanks();

private:
    std::function<void(uint8_t)> mirrorCallback;
    uint8_t targetRegister = 0x00;
    bool prgBankMode = false;
    bool chrInversion = false;

    uint32_t registers[8]{};

    uint32_t chrBank0, chrBank1, chrBank2, chrBank3, chrBank4, chrBank5, chrBank6, chrBank7;
    uint32_t prgBank0, prgBank1, prgBank2, prgBank3;

    bool     irqActive = false;
    bool     irqEnable = false;
    uint8_t  irqReload = 0x00;
    uint8_t  irqCounter = 0x00;
};
