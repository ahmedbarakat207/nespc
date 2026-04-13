#include "mapper_001.h"

mapper_001::mapper_001(uint8_t prgBanks, uint8_t chrBanks)
    : mapper(prgBanks, chrBanks) {
    shiftRegister = 0x10;
    controlRegister = 0x0C;
    chrBank0 = 0;
    chrBank1 = 0;
    prgBank = 0;
}

mapper_001::~mapper_001() {}

void mapper_001::setMirroringCallback(std::function<void(uint8_t)> cb) {
    mirrorCallback = cb;
}

bool mapper_001::cpuRead(uint16_t addr, uint32_t &mapped_addr) {
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        mapped_addr = 0x80000000 | (addr & 0x1FFF);
        return true;
    }
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        uint8_t prgMode = (controlRegister >> 2) & 0x03;
        // Mask prgBank to 4 bits (bits 0-3); bit 4 is used by some boards
        // but upper bits must not bleed into the bank number
        uint8_t bank = prgBank & 0x0F;

        if (prgMode <= 1) {
            // 32 KB switching mode — use bits 1-3 (ignore bit 0)
            uint8_t bank32 = (bank & 0x0E) >> 1;
            mapped_addr = (bank32 * 32768) + (addr & 0x7FFF);
        } else if (prgMode == 2) {
            // Fix first 16 KB at $8000, switch 16 KB at $C000
            if (addr <= 0xBFFF) {
                mapped_addr = addr & 0x3FFF;
            } else {
                mapped_addr = (bank * 16384) + (addr & 0x3FFF);
            }
        } else {
            // prgMode == 3: switch 16 KB at $8000, fix last 16 KB at $C000
            if (addr <= 0xBFFF) {
                mapped_addr = (bank * 16384) + (addr & 0x3FFF);
            } else {
                mapped_addr = ((prgBanks - 1) * 16384) + (addr & 0x3FFF);
            }
        }
        return true;
    }
    return false;
}

bool mapper_001::cpuWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) {
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        mapped_addr = 0x80000000 | (addr & 0x1FFF);
        return true;
    }

    mapped_addr = 0xFFFFFFFF;
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        if (data & 0x80) {
            // Reset shift register and force PRG mode 3
            shiftRegister = 0x10;
            controlRegister |= 0x0C;
        } else {
            bool isComplete = shiftRegister & 0x01;
            shiftRegister >>= 1;
            shiftRegister |= ((data & 0x01) << 4);

            if (isComplete) {
                uint8_t targetRegister = (addr >> 13) & 0x03;
                uint8_t val = shiftRegister & 0x1F; // only 5 bits are valid
                switch (targetRegister) {
                    case 0: 
                        controlRegister = val; 
                        if (mirrorCallback) mirrorCallback(val & 0x03);
                        break;
                    case 1: chrBank0       = val; break;
                    case 2: chrBank1       = val; break;
                    case 3: prgBank        = val; break;
                }
                shiftRegister = 0x10;
            }
        }
        return true;
    }
    return false;
}

bool mapper_001::ppuRead(uint16_t addr, uint32_t &mapped_addr) {
    if (addr < 0x2000) {
        if (chrBanks == 0) {
            // CHR RAM — pass address straight through
            mapped_addr = addr;
            return true;
        }

        uint8_t chrMode = (controlRegister >> 4) & 0x01;
        if (chrMode == 0) {
            // 8 KB mode — ignore the low bit of chrBank0, select an 8 KB bank
            uint8_t bank8 = (chrBank0 & 0x1E) >> 1;
            mapped_addr = (bank8 * 8192) + (addr & 0x1FFF);
        } else {
            // 4 KB mode
            if (addr <= 0x0FFF) {
                mapped_addr = (chrBank0 * 4096) + (addr & 0x0FFF);
            } else {
                mapped_addr = (chrBank1 * 4096) + (addr & 0x0FFF);
            }
        }
        return true;
    }
    return false;
}

bool mapper_001::ppuWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) {
    (void)data;
    if (addr < 0x2000) {
        if (chrBanks == 0) {
            // CHR RAM write
            mapped_addr = addr;
            return true;
        }
        return false;
    }
    return false;
}
