#include "mapper_004.h"
#include <cstdio>

mapper_004::mapper_004(uint8_t prgBanks, uint8_t chrBanks) : mapper(prgBanks, chrBanks) {
    prgBank0 = 0;
    prgBank1 = 1 * 0x2000;
    prgBank2 = (prgBanks * 2 - 2) * 0x2000;
    prgBank3 = (prgBanks * 2 - 1) * 0x2000;

    chrBank0 = 0;
    chrBank1 = 0x0400;
    chrBank2 = 0x0800;
    chrBank3 = 0x0C00;
    chrBank4 = 0x1000;
    chrBank5 = 0x1400;
    chrBank6 = 0x1800;
    chrBank7 = 0x1C00;

    irqActive = false;
    irqEnable = false;
    irqReload = 0x00;
    irqCounter = 0x00;
    targetRegister = 0x00;
    prgBankMode = false;
    chrInversion = false;

    for (int i = 0; i < 8; i++) registers[i] = 0;
    updateBanks();
}

mapper_004::~mapper_004() {}

void mapper_004::updateBanks() {
    if (chrInversion) {
        chrBank0 = registers[2] * 0x0400;
        chrBank1 = registers[3] * 0x0400;
        chrBank2 = registers[4] * 0x0400;
        chrBank3 = registers[5] * 0x0400;
        chrBank4 = (registers[0] & 0xFE) * 0x0400;
        chrBank5 = chrBank4 + 0x0400;
        chrBank6 = (registers[1] & 0xFE) * 0x0400;
        chrBank7 = chrBank6 + 0x0400;
    } else {
        chrBank0 = (registers[0] & 0xFE) * 0x0400;
        chrBank1 = chrBank0 + 0x0400;
        chrBank2 = (registers[1] & 0xFE) * 0x0400;
        chrBank3 = chrBank2 + 0x0400;
        chrBank4 = registers[2] * 0x0400;
        chrBank5 = registers[3] * 0x0400;
        chrBank6 = registers[4] * 0x0400;
        chrBank7 = registers[5] * 0x0400;
    }

    if (prgBankMode) {
        prgBank2 = (registers[6] & 0x3F) * 0x2000;
        prgBank0 = (prgBanks * 2 - 2) * 0x2000;
    } else {
        prgBank0 = (registers[6] & 0x3F) * 0x2000;
        prgBank2 = (prgBanks * 2 - 2) * 0x2000;
    }
    prgBank1 = (registers[7] & 0x3F) * 0x2000;
    prgBank3 = (prgBanks * 2 - 1) * 0x2000;
}

bool mapper_004::cpuWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) {
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        // PRG RAM write
        mapped_addr = addr & 0x1FFF;
        mapped_addr |= 0x80000000;
        return true;
    }

    if (addr >= 0x8000) {
        if (addr <= 0x9FFF) {
            if (!(addr & 0x0001)) {
                // Bank select
                targetRegister = data & 0x07;
                prgBankMode = (data & 0x40) != 0;
                chrInversion = (data & 0x80) != 0;
                updateBanks();
            } else {
                // Bank data
                registers[targetRegister] = data;
                updateBanks();
            }
        } else if (addr <= 0xBFFF) {
            if (!(addr & 0x0001)) {
                // Mirroring
                if (mirrorCallback) {
                    mirrorCallback(data & 0x01);
                }
            } else {
                // PRG RAM Protect
            }
        } else if (addr <= 0xDFFF) {
            if (!(addr & 0x0001)) {
                // IRQ Latch
                irqReload = data;
            } else {
                // IRQ Reload
                irqCounter = 0;
            }
        } else if (addr <= 0xFFFF) {
            if (!(addr & 0x0001)) {
                // IRQ Disable
                irqEnable = false;
                irqActive = false;
            } else {
                // IRQ Enable
                irqEnable = true;
            }
        }

        mapped_addr = 0xFFFFFFFF; // handled internally, intercept
        return true;
    }

    return false;
}

bool mapper_004::cpuRead(uint16_t addr, uint32_t &mapped_addr) {
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        mapped_addr = addr & 0x1FFF;
        mapped_addr |= 0x80000000;
        return true;
    }

    if (addr >= 0x8000 && addr <= 0x9FFF) {
        mapped_addr = prgBank0 + (addr & 0x1FFF);
        return true;
    }
    if (addr >= 0xA000 && addr <= 0xBFFF) {
        mapped_addr = prgBank1 + (addr & 0x1FFF);
        return true;
    }
    if (addr >= 0xC000 && addr <= 0xDFFF) {
        mapped_addr = prgBank2 + (addr & 0x1FFF);
        return true;
    }
    if (addr >= 0xE000 && addr <= 0xFFFF) {
        mapped_addr = prgBank3 + (addr & 0x1FFF);
        return true;
    }

    return false;
}

bool mapper_004::ppuWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) {
    (void)data;
    if (addr <= 0x1FFF) {
        if (chrBanks == 0) {
            // CHR RAM
            mapped_addr = addr;
            return true;
        }
        return false;
    }
    return false;
}

bool mapper_004::ppuRead(uint16_t addr, uint32_t &mapped_addr) {
    if (addr <= 0x03FF) { mapped_addr = chrBank0 + (addr & 0x03FF); return true; }
    if (addr >= 0x0400 && addr <= 0x07FF) { mapped_addr = chrBank1 + (addr & 0x03FF); return true; }
    if (addr >= 0x0800 && addr <= 0x0BFF) { mapped_addr = chrBank2 + (addr & 0x03FF); return true; }
    if (addr >= 0x0C00 && addr <= 0x0FFF) { mapped_addr = chrBank3 + (addr & 0x03FF); return true; }
    if (addr >= 0x1000 && addr <= 0x13FF) { mapped_addr = chrBank4 + (addr & 0x03FF); return true; }
    if (addr >= 0x1400 && addr <= 0x17FF) { mapped_addr = chrBank5 + (addr & 0x03FF); return true; }
    if (addr >= 0x1800 && addr <= 0x1BFF) { mapped_addr = chrBank6 + (addr & 0x03FF); return true; }
    if (addr >= 0x1C00 && addr <= 0x1FFF) { mapped_addr = chrBank7 + (addr & 0x03FF); return true; }

    return false;
}

void mapper_004::scanline() {
    if (irqCounter == 0) {
        irqCounter = irqReload;
    } else {
        irqCounter--;
        if (irqCounter == 0 && irqEnable) {
            irqActive = true;
        }
    }
}

bool mapper_004::irqState() {
    return irqActive;
}

void mapper_004::irqClear() {
    irqActive = false;
}

void mapper_004::setMirroringCallback(std::function<void(uint8_t)> cb) {
    mirrorCallback = cb;
}
