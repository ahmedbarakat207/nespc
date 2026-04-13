#include "mapper_004.h"

mapper_004::mapper_004(uint8_t prgBanks, uint8_t chrBanks) : mapper(prgBanks, chrBanks) {
    for (int i = 0; i < 8; i++) registers[i] = 0;
    targetRegister = 0;
    prgBankMode  = false;
    chrInversion = false;

    irqCounter      = 0;
    irqReload       = 0;
    irqReloadPending = false;
    irqEnable       = false;
    irqActive       = false;

    updateBanks();
}

mapper_004::~mapper_004() {}

void mapper_004::updateBanks() {
    const int nPRG8 = prgBanks * 2;

    if (!prgBankMode) {
        prgBank[0] = (registers[6] & 0x3F) * 0x2000;
        prgBank[1] = (registers[7] & 0x3F) * 0x2000;
        prgBank[2] = (nPRG8 - 2)           * 0x2000;
        prgBank[3] = (nPRG8 - 1)           * 0x2000;
    } else {
        prgBank[0] = (nPRG8 - 2)           * 0x2000;
        prgBank[1] = (registers[7] & 0x3F) * 0x2000;
        prgBank[2] = (registers[6] & 0x3F) * 0x2000;
        prgBank[3] = (nPRG8 - 1)           * 0x2000;
    }

    if (!chrInversion) {
        chrBank[0] = (registers[0] & 0xFE) * 0x0400;
        chrBank[1] = chrBank[0]            + 0x0400;
        chrBank[2] = (registers[1] & 0xFE) * 0x0400;
        chrBank[3] = chrBank[2]            + 0x0400;
        chrBank[4] =  registers[2]         * 0x0400;
        chrBank[5] =  registers[3]         * 0x0400;
        chrBank[6] =  registers[4]         * 0x0400;
        chrBank[7] =  registers[5]         * 0x0400;
    } else {
        chrBank[0] =  registers[2]         * 0x0400;
        chrBank[1] =  registers[3]         * 0x0400;
        chrBank[2] =  registers[4]         * 0x0400;
        chrBank[3] =  registers[5]         * 0x0400;
        chrBank[4] = (registers[0] & 0xFE) * 0x0400;
        chrBank[5] = chrBank[4]            + 0x0400;
        chrBank[6] = (registers[1] & 0xFE) * 0x0400;
        chrBank[7] = chrBank[6]            + 0x0400;
    }
}

bool mapper_004::cpuRead(uint16_t addr, uint32_t& mapped_addr) {
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        mapped_addr = 0x80000000 | (addr & 0x1FFF);
        return true;
    }
    if (addr >= 0x8000) {
        uint8_t slot = (addr - 0x8000) >> 13;
        mapped_addr  = prgBank[slot] + (addr & 0x1FFF);
        return true;
    }
    return false;
}

bool mapper_004::cpuWrite(uint16_t addr, uint32_t& mapped_addr, uint8_t data) {
    mapped_addr = 0xFFFFFFFF;

    if (addr >= 0x6000 && addr <= 0x7FFF) {
        mapped_addr = 0x80000000 | (addr & 0x1FFF);
        return true;
    }

    if (addr < 0x8000) return false;

    bool odd = addr & 1;

    if (addr <= 0x9FFF) {
        if (!odd) {
            targetRegister = data & 0x07;
            prgBankMode    = (data & 0x40) != 0;
            chrInversion   = (data & 0x80) != 0;
            updateBanks();
        } else {
            registers[targetRegister] = data;
            updateBanks();
        }
    } else if (addr <= 0xBFFF) {
        if (!odd) {
            if (mirrorCallback) mirrorCallback(data & 0x01);
        }
    } else if (addr <= 0xDFFF) {
        if (!odd) {
            irqReload = data;
        } else {
            irqCounter       = 0;
            irqReloadPending = true;
        }
    } else {
        if (!odd) {
            irqEnable = false;
            irqActive = false;
        } else {
            irqEnable = true;
        }
    }

    return true;
}

bool mapper_004::ppuRead(uint16_t addr, uint32_t& mapped_addr) {
    if (addr > 0x1FFF) return false;
    uint8_t slot = addr >> 10;
    mapped_addr  = chrBank[slot] + (addr & 0x03FF);
    return true;
}

bool mapper_004::ppuWrite(uint16_t addr, uint32_t& mapped_addr, uint8_t data) {
    (void)data;
    if (addr > 0x1FFF) return false;
    if (chrBanks != 0) return false;

    uint8_t slot = addr >> 10;
    mapped_addr  = chrBank[slot] + (addr & 0x03FF);
    return true;
}

void mapper_004::scanline() {
    if (irqCounter == 0 || irqReloadPending) {
        irqCounter       = irqReload;
        irqReloadPending = false;
    } else {
        irqCounter--;
    }

    if (irqCounter == 0 && irqEnable) {
        irqActive = true;
    }
}

bool mapper_004::irqState() { return irqActive; }
void mapper_004::irqClear()  { irqActive = false; }

void mapper_004::setMirroringCallback(std::function<void(uint8_t)> cb) {
    mirrorCallback = cb;
}
