#pragma once
#include "mapper.h"
#include <functional>

class mapper_004 : public mapper {
public:
    mapper_004(uint8_t prgBanks, uint8_t chrBanks);
    ~mapper_004() override;

    bool cpuRead (uint16_t addr, uint32_t& mapped_addr) override;
    bool cpuWrite(uint16_t addr, uint32_t& mapped_addr, uint8_t data = 0) override;
    bool ppuRead (uint16_t addr, uint32_t& mapped_addr) override;
    bool ppuWrite(uint16_t addr, uint32_t& mapped_addr, uint8_t data = 0) override;

    void scanline() override;
    bool irqState() override;
    void irqClear() override;
    void setMirroringCallback(std::function<void(uint8_t)> cb) override;

private:
    void updateBanks();

    std::function<void(uint8_t)> mirrorCallback;

    uint8_t registers[8]{};
    uint8_t targetRegister = 0;
    bool    prgBankMode    = false;
    bool    chrInversion   = false;

    // Four 8-KB PRG windows and eight 1-KB CHR windows
    uint32_t prgBank[4]{};
    uint32_t chrBank[8]{};

    uint8_t  irqReload        = 0;
    uint8_t  irqCounter       = 0;
    bool     irqReloadPending = false;
    bool     irqEnable        = false;
    bool     irqActive        = false;
};
