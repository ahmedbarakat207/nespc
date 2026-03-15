#include "bus.h"
#include <cstdio>
#include <cstring>

bus::bus() {
    cpuRam.fill(0x00);
    picture = new ppu();
    cpu.connect_bus(this);
}

bus::~bus() {
    delete picture;
}

void bus::insert_cartridge(const std::shared_ptr<rom> &cart) {
    cartridge = cart;
    picture->connect_cartridge(cart);
}

void bus::reset() {
    cpu.reset();
    picture->reset();
    sound.reset();
    nSystemClockCounter = 0;
    dma_transfer = false;
    dma_dummy    = true;
    dma_page     = 0;
    dma_addr     = 0;
    audioAccum   = 0.0;
    audioReady   = false;
    controller_state[0] = 0;
    controller_state[1] = 0;
}

void bus::clock() {
    picture->clock();

    if (nSystemClockCounter % 3 == 0) {
        if (dma_transfer) {
            if (dma_dummy) {
                if (nSystemClockCounter % 2 == 1) dma_dummy = false;
            } else {
                if (nSystemClockCounter % 2 == 0) {
                    dma_data = cpuRead((uint16_t)(dma_page << 8) | dma_addr);
                } else {
                    picture->oam[dma_addr] = dma_data;
                    uint8_t si = dma_addr / 4;
                    switch (dma_addr % 4) {
                        case 0: picture->OAM[si].y         = dma_data; break;
                        case 1: picture->OAM[si].id        = dma_data; break;
                        case 2: picture->OAM[si].attribute = dma_data; break;
                        case 3: picture->OAM[si].x         = dma_data; break;
                    }
                    dma_addr++;
                    if (dma_addr == 0x00) {
                        dma_transfer = false;
                        dma_dummy    = true;
                    }
                }
            }
        } else {
            cpu.clock();
        }

        sound.clock();

        audioAccum += SAMPLE_RATE / CPU_FREQ;
        if (audioAccum >= 1.0) {
            audioAccum -= 1.0;
            audioSample = sound.getOutput();
            audioReady  = true;
        }
    }

    if (picture->nmi) {
        picture->nmi = false;
        cpu.nmi();
    }

    if (cartridge && cartridge->pMapper && cartridge->pMapper->irqState()) {
        cartridge->pMapper->irqClear();
        cpu.irq();
    }

    nSystemClockCounter++;
}

void bus::cpuWrite(uint16_t addr, uint8_t data) {
    if (cartridge && cartridge->cpuWrite(addr, data)) return;

    if (addr <= 0x1FFF) {
        cpuRam[addr & 0x07FF] = data;
    } else if (addr >= 0x2000 && addr <= 0x3FFF) {
        picture->cpuWrite(addr & 0x0007, data);
    } else if (addr == 0x4014) {
        dma_page     = data;
        dma_addr     = 0x00;
        dma_transfer = true;
    } else if (addr == 0x4016) {
        controller_state[0] = controller[0];
        controller_state[1] = controller[1];
    } else if ((addr >= 0x4000 && addr <= 0x4013) || addr == 0x4015 || addr == 0x4017) {
        sound.cpuWrite(addr, data);
    }
}

uint8_t bus::cpuRead(uint16_t addr) {
    uint8_t data = 0x00;

    if (cartridge && cartridge->cpuRead(addr, data)) return data;

    if (addr <= 0x1FFF) {
        return cpuRam[addr & 0x07FF];
    } else if (addr >= 0x2000 && addr <= 0x3FFF) {
        return picture->cpuRead(addr & 0x0007);
    } else if (addr == 0x4015) {
        return sound.cpuRead(addr);
    } else if (addr == 0x4016) {
        data = (controller_state[0] & 0x80) ? 1 : 0;
        controller_state[0] <<= 1;
        return data;
    } else if (addr == 0x4017) {
        data = (controller_state[1] & 0x80) ? 1 : 0;
        controller_state[1] <<= 1;
        return data;
    }

    return 0x00;
}
