#include "mapper_000.h"

mapper_000::mapper_000(uint8_t prgBanks, uint8_t chrBanks) : mapper(prgBanks, chrBanks) {}

mapper_000::~mapper_000() {}

bool mapper_000::cpuRead(uint16_t addr, uint32_t &mapped_addr) {
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        if (prgBanks == 1) {
            mapped_addr = addr & 0x3FFF;
        } else {
            mapped_addr = addr & 0x7FFF;
        }
        return true;
    }
    return false;
}

bool mapper_000::cpuWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) {
    (void)data;
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        if (prgBanks == 1) {
            mapped_addr = addr & 0x3FFF;
        } else {
            mapped_addr = addr & 0x7FFF;
        }
        return true;
    }
    return false;
}

bool mapper_000::ppuRead(uint16_t addr, uint32_t &mapped_addr) {
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        mapped_addr = addr;
        return true;
    }
    return false;
}

bool mapper_000::ppuWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data) {
    (void)data;
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        if (chrBanks == 0) {
            mapped_addr = addr;
            return true;
        }
    }
    return false;
}