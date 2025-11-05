#include <iostream>
#include <fstream>
#include "rom.h"
#include "mapper_000.h"

rom::rom(const std::string &filename) {
    printf("ROM: Loading file: %s\n", filename.c_str());
    
    struct inesHeader {
        char name[4];
        uint8_t prg_rom_chunks;
        uint8_t chr_rom_chunks;
        uint8_t mapper1;
        uint8_t mapper2;
        uint8_t prg_ram_size;
        uint8_t tv_system1;
        uint8_t tv_system2;
        char unused[5];
    } header;

    std::ifstream ifs;
    ifs.open(filename, std::ifstream::binary);
    if (!ifs.is_open()) {
        throw std::runtime_error("Cannot open ROM file: " + filename);
    }
    
    // Read header
    ifs.read(reinterpret_cast<char*>(&header), sizeof(header));
    
    if (header.name[0] != 'N' || header.name[1] != 'E' || header.name[2] != 'S' || header.name[3] != 0x1A) {
        throw std::runtime_error("Invalid iNES header");
    }

    // Check for trainer
    bool has_trainer = (header.mapper1 & 0x04) != 0;
    if (has_trainer) {
        ifs.seekg(512, std::ios_base::cur);
        printf("ROM: Skipping 512-byte trainer\n");
    }

    mapperID = ((header.mapper2 >> 4) << 4) | (header.mapper1 >> 4);
    mirror = (header.mapper1 & 0x01) ? VERTICAL : HORIZONTAL;
    prgBanks = header.prg_rom_chunks;
    chrBanks = header.chr_rom_chunks;

    printf("ROM Info: PRG Banks: %d, CHR Banks: %d, Mapper: %d, Mirroring: %s\n", 
           prgBanks, chrBanks, mapperID, mirror == VERTICAL ? "Vertical" : "Horizontal");

    // Load PRG ROM
    size_t prgSize = prgBanks * 16384;
    if (prgSize > 0) {
        prgMemory.resize(prgSize);
        ifs.read(reinterpret_cast<char*>(prgMemory.data()), prgSize);
        printf("ROM: Loaded %zu bytes of PRG ROM\n", prgSize);
    } else {
        printf("ROM: No PRG ROM found!\n");
        prgMemory.resize(32768); // Default to 32KB
    }

    // Load CHR ROM
    size_t chrSize = chrBanks * 8192;
    if (chrSize > 0) {
        chrMemory.resize(chrSize);
        ifs.read(reinterpret_cast<char*>(chrMemory.data()), chrSize);
        printf("ROM: Loaded %zu bytes of CHR ROM\n", chrSize);
    } else {
        printf("ROM: No CHR ROM found, using CHR RAM\n");
        chrMemory.resize(8192); // 8KB CHR RAM
        // Initialize with basic pattern
        for (size_t i = 0; i < chrMemory.size(); i++) {
            chrMemory[i] = (i % 2 == 0) ? 0xFF : 0x00;
        }
    }

    // Initialize mapper
    switch (mapperID) {
        case 0:
            pMapper = std::make_shared<mapper_000>(prgBanks, chrBanks);
            printf("ROM: Mapper 000 (NROM) initialized\n");
            break;
        default:
            throw std::runtime_error("Unsupported mapper: " + std::to_string(mapperID));
    }

    ifs.close();
    printf("ROM: Successfully loaded\n");
}

rom::~rom() {
    printf("ROM: Destructor called\n");
}

bool rom::cpuRead(uint16_t addr, uint8_t &data) {
    uint32_t mapped_addr = 0;
    if (pMapper && pMapper->cpuRead(addr, mapped_addr)) {
        if (mapped_addr < prgMemory.size()) {
            data = prgMemory[mapped_addr];
            return true;
        } else {
            //printf("ROM: CPU Read out of bounds: %04X -> %08X (PRG size: %zu)\n", 
            //       addr, mapped_addr, prgMemory.size());
            data = 0xFF; // Return default value
            return true;
        }
    }
    return false;
}

bool rom::cpuWrite(uint16_t addr, uint8_t data) {
    uint32_t mapped_addr = 0;
    if (pMapper && pMapper->cpuWrite(addr, mapped_addr)) {
        if (mapped_addr < prgMemory.size()) {
            prgMemory[mapped_addr] = data;
            return true;
        } else {
            //printf("ROM: CPU Write out of bounds: %04X -> %08X (PRG size: %zu)\n", 
            //       addr, mapped_addr, prgMemory.size());
            return true;
        }
    }
    return false;
}

bool rom::ppuRead(uint16_t addr, uint8_t &data) {
    uint32_t mapped_addr = 0;
    if (pMapper && pMapper->ppuRead(addr, mapped_addr)) {
        if (mapped_addr < chrMemory.size()) {
            data = chrMemory[mapped_addr];
            return true;
        } else {
            //printf("ROM: PPU Read out of bounds: %04X -> %08X (CHR size: %zu)\n", 
            //       addr, mapped_addr, chrMemory.size());
            data = 0x00; // Return default value
            return true;
        }
    }
    return false;
}

bool rom::ppuWrite(uint16_t addr, uint8_t data) {
    uint32_t mapped_addr = 0;
    if (pMapper && pMapper->ppuWrite(addr, mapped_addr)) {
        if (mapped_addr < chrMemory.size()) {
            chrMemory[mapped_addr] = data;
            return true;
        } else {
            //printf("ROM: PPU Write out of bounds: %04X -> %08X (CHR size: %zu)\n", 
            //       addr, mapped_addr, chrMemory.size());
            return true;
        }
    }
    return false;
}