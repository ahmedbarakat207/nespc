#include "bus.h"
#include "6502.h"

uint8_t cpu6502::IMP() { fetched = a; return 0; }

uint8_t cpu6502::IMM() { addr_abs = pc++; return 0; }

uint8_t cpu6502::ZP0() {
    addr_abs = read(pc++) & 0x00FF;
    return 0;
}

uint8_t cpu6502::ZPX() {
    addr_abs = (read(pc++) + x) & 0x00FF;
    return 0;
}

uint8_t cpu6502::ZPY() {
    addr_abs = (read(pc++) + y) & 0x00FF;
    return 0;
}

uint8_t cpu6502::ABS() {
    uint16_t lo = read(pc++);
    uint16_t hi = read(pc++);
    addr_abs = (hi << 8) | lo;
    return 0;
}

uint8_t cpu6502::ABX() {
    uint16_t lo = read(pc++);
    uint16_t hi = read(pc++);
    addr_abs = ((hi << 8) | lo) + x;
    return (addr_abs & 0xFF00) != (hi << 8) ? 1 : 0;
}

uint8_t cpu6502::ABY() {
    uint16_t lo = read(pc++);
    uint16_t hi = read(pc++);
    addr_abs = ((hi << 8) | lo) + y;
    return (addr_abs & 0xFF00) != (hi << 8) ? 1 : 0;
}

uint8_t cpu6502::IND() {
    uint16_t ptr_lo = read(pc++);
    uint16_t ptr_hi = read(pc++);
    uint16_t ptr = (ptr_hi << 8) | ptr_lo;
    if (ptr_lo == 0x00FF)
        addr_abs = (read(ptr & 0xFF00) << 8) | read(ptr);
    else
        addr_abs = (read(ptr + 1) << 8) | read(ptr);
    return 0;
}

uint8_t cpu6502::IZX() {
    uint16_t t = read(pc++);
    uint16_t lo = read((t + x) & 0x00FF);
    uint16_t hi = read((t + x + 1) & 0x00FF);
    addr_abs = (hi << 8) | lo;
    return 0;
}

uint8_t cpu6502::IZY() {
    uint16_t t = read(pc++);
    uint16_t lo = read(t & 0x00FF);
    uint16_t hi = read((t + 1) & 0x00FF);
    addr_abs = ((hi << 8) | lo) + y;
    return (addr_abs & 0xFF00) != (hi << 8) ? 1 : 0;
}

uint8_t cpu6502::REL() {
    addr_rel = read(pc++);
    if (addr_rel & 0x80) addr_rel |= 0xFF00;
    return 0;
}
