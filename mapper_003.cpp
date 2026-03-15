#include "mapper_003.h"

mapper_003::mapper_003(uint8_t prgBanks, uint8_t chrBanks)
    : mapper(prgBanks, chrBanks), chrBank(0) {}

mapper_003::~mapper_003() {}

bool mapper_003::cpuRead(uint16_t addr, uint32_t &mapped_addr) {
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        mapped_addr = (prgBanks == 1) ? (addr & 0x3FFF) : (addr & 0x7FFF);
        return true;
    }
    return false;
}

bool mapper_003::cpuWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) {
    mapped_addr = 0xFFFFFFFF;
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        chrBank = (chrBanks > 0) ? (data & (chrBanks - 1)) : 0;
        return true;
    }
    return false;
}

bool mapper_003::ppuRead(uint16_t addr, uint32_t &mapped_addr) {
    if (addr <= 0x1FFF) {
        mapped_addr = (uint32_t)chrBank * 0x2000 + addr;
        return true;
    }
    return false;
}

bool mapper_003::ppuWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) {
    (void)data;
    if (addr <= 0x1FFF) {
        if (chrBanks == 0) {
            mapped_addr = addr;
            return true;
        }
    }
    return false;
}
