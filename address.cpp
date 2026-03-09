#include <iostream>
#include "bus.h"
#include "6502.h"

uint8_t cpu6502::IMP(){
    fetched = a;
    return 0;
}

uint8_t cpu6502::IMM(){
    addr_abs = pc++;
    return 0;
}

uint8_t cpu6502::ZP0(){
    addr_abs = read(pc);
    pc++;
    addr_abs &= 0x00FF;
    return 0;
}

uint8_t cpu6502::ZPX(){
    addr_abs = (read(pc) + x);
    pc++;
    addr_abs &= 0x00FF;
    return 0;
}

uint8_t cpu6502::ZPY(){
    addr_abs = (read(pc) + y);
    pc++;
    addr_abs &= 0x00FF;
    return 0;
}

uint8_t cpu6502::ABS(){
    uint16_t lo = read(pc);
    pc++;
    uint16_t hi = read(pc);
    pc++;
    addr_abs = (hi << 8) | lo;
    return 0;
}
uint8_t cpu6502::ABX(){
    uint16_t lo = read(pc);
    pc++;
    uint16_t hi = read(pc);
    pc++;
    addr_abs = (hi << 8) | lo;
    addr_abs += x;
    if ((addr_abs & 0XFF00) != (hi << 8)){
        return 1;
    } else {
        return 0;
    }
}
uint8_t cpu6502::ABY(){
    uint16_t lo = read(pc);
    pc++;
    uint16_t hi = read(pc);
    pc++;
    addr_abs = (hi << 8) | lo;
    addr_abs += y;
    if ((addr_abs & 0XFF00) != (hi << 8)){
        return 1;
    } else {
        return 0;
    }
}

uint8_t cpu6502::IND(){
    uint16_t ptr_lo = read(pc);
    pc++;
    uint16_t ptr_hi = read(pc);
    pc++;
    uint16_t ptr = (ptr_hi << 8) | ptr_lo;
    if (ptr_lo == 0x00FF){
        addr_abs = (read(ptr & 0xFF00) << 8) | read(ptr + 0);
    } else {
        addr_abs = (read(ptr + 1) << 8) | read(ptr + 0);
    }
    return 0;
}

uint8_t cpu6502::IZX(){
    uint16_t t = read(pc);
    pc++;

    uint16_t lo = read((uint16_t)(t + (uint16_t)x) & 0x00FF);
    uint16_t hi = read((uint16_t)(t + (uint16_t)x + 1) & 0x00FF);

    addr_abs = (hi << 8) | lo;

    return 0;
}

uint8_t cpu6502::IZY(){
    uint16_t t = read(pc);
    pc++;

    uint16_t lo = read(t & 0x00FF);
    uint16_t hi = read((t + 1) & 0x00FF);

    addr_abs = (hi << 8) | lo;
    addr_abs += y;

    if ((addr_abs & 0xFF00) != (hi << 8)){
        return 1;
    } else {
        return 0;
    }
}

uint8_t cpu6502::REL(){
    addr_rel = read(pc);
    pc++;
    
    if (addr_rel & 0x80){
        addr_rel |= 0xFF00;
    }
    return 0;
}