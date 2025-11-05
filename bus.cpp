#include "bus.h"
#include "6502.h"
#include "ppu.h"
#include "rom.h"
#include <iostream>


void bus::cpuWrite(uint16_t addr, uint8_t data) {
    //printf("Bus CPU Write: %04X = %02X\n", addr, data);
    
    if (cartridge && cartridge->cpuWrite(addr, data)) {
        return;
    }
    
    if (addr <= 0x1FFF) {
        cpuRam[addr & 0x07FF] = data;
    } else if (addr >= 0x2000 && addr <= 0x3FFF) {
        ppu->cpuWrite(addr & 0x0007, data);
    } else if (addr == 0x4014) {
        handleOAMDMA(data);
    } else if (addr >= 0x4000 && addr <= 0x401F) {
        // APU and controller - ignore for now
        printf("Bus: I/O Write ignored: %04X = %02X\n", addr, data);
    } else {
        printf("Bus: Unhandled CPU Write: %04X = %02X\n", addr, data);
    }
}
uint8_t bus::cpuRead(uint16_t addr) {
    //printf("Bus CPU Read: %04X\n", addr);
    
    uint8_t data = 0x00;
    
    // First check cartridge
    if (cartridge && cartridge->cpuRead(addr, data)) {
        return data;
    }
    
    if (addr <= 0x1FFF) {
        return cpuRam[addr & 0x07FF];
    } else if (addr >= 0x2000 && addr <= 0x3FFF) {
        return ppu->cpuRead(addr & 0x0007);
    } else if (addr >= 0x4000 && addr <= 0x401F) {
        // APU and controller - return 0 for now
        return 0x00;
    } else if (addr >= 0x4020 && addr <= 0x5FFF) {
        // Extended ROM - not handled
        return 0x00;
    } else if (addr >= 0x6000 && addr <= 0x7FFF) {
        // Cartridge RAM - not handled  
        return 0x00;
    } else if (addr >= 0x8000 && addr <= 0xFFFF) {
        // This should be handled by cartridge
        printf("Bus: Cartridge ROM Read not handled: %04X\n", addr);
        return 0x00;
    }
    
    printf("Bus: Unhandled CPU Read: %04X\n", addr);
    return 0x00;
}
bus::bus() : cpuRam{} {
    printf("Bus: Constructor started\n");
    cpuRam.fill(0x00);
    cpu.connect_bus(this);
    ppu = new ::ppu();  // Heap allocation
    printf("Bus: Constructor completed - CPU connected\n");
}

bus::~bus() {
    delete ppu;
    printf("Bus: Destructor called\n");
}

void bus::insert_cartridge(const std::shared_ptr<rom>& cartridge) {
    printf("Bus: Inserting cartridge\n");
    this->cartridge = cartridge;
    ppu->connect_cartridge(cartridge);
    printf("Bus: Cartridge inserted\n");
}

void bus::reset() {
    printf("Bus: Reset started\n");
    cpu.reset();
    ppu->reset();
    nSystemClockCounter = 0;
    printf("Bus: Reset completed\n");
}

void bus::clock() {
    ppu->clock();
    
    if (nSystemClockCounter % 3 == 0) {
        cpu.clock();
    }
    
    if (ppu->nmi) {
        ppu->nmi = false;
        cpu.nmi();
    }
    
    nSystemClockCounter++;
}

// Update handleOAMDMA to use pointer
void bus::handleOAMDMA(uint8_t page) {
    printf("Bus: OAM DMA from page %02X\n", page);
    uint16_t base_addr = page << 8;
    
    for (int i = 0; i < 256; i++) {
        uint16_t src_addr = base_addr + i;
        uint8_t data = cpuRead(src_addr);
        
        // Write to OAM
        if (i < 256) {
            ppu->oam[i] = data;
        }
    }
}