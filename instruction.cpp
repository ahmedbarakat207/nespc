#include "bus.h"
#include "6502.h"

uint8_t cpu6502::ADC() {
    fetch();
    uint16_t t = (uint16_t)a + (uint16_t)fetched + (uint16_t)get_flag(C);
    set_flag(C, t > 255);
    set_flag(Z, (t & 0x00FF) == 0);
    set_flag(V, (~((uint16_t)a ^ (uint16_t)fetched) & ((uint16_t)a ^ t)) & 0x0080);
    set_flag(N, t & 0x80);
    a = t & 0x00FF;
    return 1;
}

uint8_t cpu6502::SBC() {
    fetch();
    uint16_t value = (uint16_t)fetched ^ 0x00FF;
    uint16_t t = (uint16_t)a + value + (uint16_t)get_flag(C);
    set_flag(C, t & 0xFF00);
    set_flag(Z, (t & 0x00FF) == 0);
    set_flag(V, (t ^ (uint16_t)a) & (t ^ value) & 0x0080);
    set_flag(N, t & 0x0080);
    a = t & 0x00FF;
    return 1;
}

uint8_t cpu6502::AND() {
    fetch();
    a &= fetched;
    set_flag(Z, a == 0);
    set_flag(N, a & 0x80);
    return 1;
}

uint8_t cpu6502::ASL() {
    fetch();
    uint16_t t = (uint16_t)fetched << 1;
    set_flag(C, t & 0xFF00);
    set_flag(Z, (t & 0x00FF) == 0);
    set_flag(N, t & 0x80);
    if (lookup[opcode].addrmode == &cpu6502::IMP) a = t & 0x00FF;
    else write(addr_abs, t & 0x00FF);
    return 0;
}

uint8_t cpu6502::BCC() {
    if (get_flag(C) == 0) {
        cycles++;
        addr_abs = pc + addr_rel;
        if ((addr_abs & 0xFF00) != (pc & 0xFF00)) cycles++;
        pc = addr_abs;
    }
    return 0;
}

uint8_t cpu6502::BCS() {
    if (get_flag(C) == 1) {
        cycles++;
        addr_abs = pc + addr_rel;
        if ((addr_abs & 0xFF00) != (pc & 0xFF00)) cycles++;
        pc = addr_abs;
    }
    return 0;
}

uint8_t cpu6502::BEQ() {
    if (get_flag(Z) == 1) {
        cycles++;
        addr_abs = pc + addr_rel;
        if ((addr_abs & 0xFF00) != (pc & 0xFF00)) cycles++;
        pc = addr_abs;
    }
    return 0;
}

uint8_t cpu6502::BIT() {
    fetch();
    temp = a & fetched;
    set_flag(Z, (temp & 0x00FF) == 0);
    set_flag(N, fetched & (1 << 7));
    set_flag(V, fetched & (1 << 6));
    return 0;
}

uint8_t cpu6502::BMI() {
    if (get_flag(N) == 1) {
        cycles++;
        addr_abs = pc + addr_rel;
        if ((addr_abs & 0xFF00) != (pc & 0xFF00)) cycles++;
        pc = addr_abs;
    }
    return 0;
}

uint8_t cpu6502::BNE() {
    if (get_flag(Z) == 0) {
        cycles++;
        addr_abs = pc + addr_rel;
        if ((addr_abs & 0xFF00) != (pc & 0xFF00)) cycles++;
        pc = addr_abs;
    }
    return 0;
}

uint8_t cpu6502::BPL() {
    if (get_flag(N) == 0) {
        cycles++;
        addr_abs = pc + addr_rel;
        if ((addr_abs & 0xFF00) != (pc & 0xFF00)) cycles++;
        pc = addr_abs;
    }
    return 0;
}

uint8_t cpu6502::BRK() {
    pc++;
    write(0x0100 + stkp--, (pc >> 8) & 0x00FF);
    write(0x0100 + stkp--, pc & 0x00FF);
    set_flag(B, 1);
    set_flag(U, 1);
    write(0x0100 + stkp--, status);
    set_flag(B, 0);
    set_flag(I, 1);
    pc = (uint16_t)read(0xFFFE) | ((uint16_t)read(0xFFFF) << 8);
    return 0;
}

uint8_t cpu6502::BVC() {
    if (get_flag(V) == 0) {
        cycles++;
        addr_abs = pc + addr_rel;
        if ((addr_abs & 0xFF00) != (pc & 0xFF00)) cycles++;
        pc = addr_abs;
    }
    return 0;
}

uint8_t cpu6502::BVS() {
    if (get_flag(V) == 1) {
        cycles++;
        addr_abs = pc + addr_rel;
        if ((addr_abs & 0xFF00) != (pc & 0xFF00)) cycles++;
        pc = addr_abs;
    }
    return 0;
}

uint8_t cpu6502::CLC() { set_flag(C, false); return 0; }
uint8_t cpu6502::CLD() { set_flag(D, false); return 0; }
uint8_t cpu6502::CLI() { set_flag(I, false); return 0; }
uint8_t cpu6502::CLV() { set_flag(V, false); return 0; }

uint8_t cpu6502::CMP() {
    fetch();
    temp = (uint16_t)a - (uint16_t)fetched;
    set_flag(C, a >= fetched);
    set_flag(Z, (temp & 0x00FF) == 0);
    set_flag(N, temp & 0x0080);
    return 1;
}

uint8_t cpu6502::CPX() {
    fetch();
    temp = (uint16_t)x - (uint16_t)fetched;
    set_flag(C, x >= fetched);
    set_flag(Z, (temp & 0x00FF) == 0);
    set_flag(N, temp & 0x0080);
    return 0;
}

uint8_t cpu6502::CPY() {
    fetch();
    temp = (uint16_t)y - (uint16_t)fetched;
    set_flag(C, y >= fetched);
    set_flag(Z, (temp & 0x00FF) == 0);
    set_flag(N, temp & 0x0080);
    return 0;
}

uint8_t cpu6502::DEC() {
    fetch();
    temp = fetched - 1;
    write(addr_abs, temp & 0x00FF);
    set_flag(Z, (temp & 0x00FF) == 0);
    set_flag(N, temp & 0x0080);
    return 0;
}

uint8_t cpu6502::DEX() { x--; set_flag(Z, x == 0); set_flag(N, x & 0x80); return 0; }
uint8_t cpu6502::DEY() { y--; set_flag(Z, y == 0); set_flag(N, y & 0x80); return 0; }

uint8_t cpu6502::EOR() {
    fetch();
    a ^= fetched;
    set_flag(Z, a == 0);
    set_flag(N, a & 0x80);
    return 1;
}

uint8_t cpu6502::INC() {
    fetch();
    temp = fetched + 1;
    write(addr_abs, temp & 0x00FF);
    set_flag(Z, (temp & 0x00FF) == 0);
    set_flag(N, temp & 0x0080);
    return 0;
}

uint8_t cpu6502::INX() { x++; set_flag(Z, x == 0); set_flag(N, x & 0x80); return 0; }
uint8_t cpu6502::INY() { y++; set_flag(Z, y == 0); set_flag(N, y & 0x80); return 0; }

uint8_t cpu6502::JMP() { pc = addr_abs; return 0; }

uint8_t cpu6502::JSR() {
    pc--;
    write(0x0100 + stkp--, (pc >> 8) & 0x00FF);
    write(0x0100 + stkp--, pc & 0x00FF);
    pc = addr_abs;
    return 0;
}

uint8_t cpu6502::LDA() {
    fetch();
    a = fetched;
    set_flag(Z, a == 0);
    set_flag(N, a & 0x80);
    return 1;
}

uint8_t cpu6502::LDX() {
    fetch();
    x = fetched;
    set_flag(Z, x == 0);
    set_flag(N, x & 0x80);
    return 1;
}

uint8_t cpu6502::LDY() {
    fetch();
    y = fetched;
    set_flag(Z, y == 0);
    set_flag(N, y & 0x80);
    return 1;
}

uint8_t cpu6502::LSR() {
    fetch();
    set_flag(C, fetched & 0x0001);
    temp = fetched >> 1;
    set_flag(Z, (temp & 0x00FF) == 0);
    set_flag(N, temp & 0x0080);
    if (lookup[opcode].addrmode == &cpu6502::IMP) a = temp & 0x00FF;
    else write(addr_abs, temp & 0x00FF);
    return 0;
}

uint8_t cpu6502::NOP() {
    switch (opcode) {
        case 0x1C: case 0x3C: case 0x5C: case 0x7C: case 0xDC: case 0xFC: return 1;
    }
    return 0;
}

uint8_t cpu6502::ORA() {
    fetch();
    a |= fetched;
    set_flag(Z, a == 0);
    set_flag(N, a & 0x80);
    return 1;
}

uint8_t cpu6502::PHA() { write(0x0100 + stkp--, a); return 0; }

uint8_t cpu6502::PHP() {
    write(0x0100 + stkp--, status | B | U);
    return 0;
}

uint8_t cpu6502::PLA() {
    stkp++;
    a = read(0x0100 + stkp);
    set_flag(Z, a == 0);
    set_flag(N, a & 0x80);
    return 0;
}

uint8_t cpu6502::PLP() {
    stkp++;
    status = read(0x0100 + stkp);
    status &= ~B;
    status |= U;
    return 0;
}

uint8_t cpu6502::ROL() {
    fetch();
    temp = (uint16_t)(fetched << 1) | get_flag(C);
    set_flag(C, temp & 0xFF00);
    set_flag(Z, (temp & 0x00FF) == 0);
    set_flag(N, temp & 0x0080);
    if (lookup[opcode].addrmode == &cpu6502::IMP) a = temp & 0x00FF;
    else write(addr_abs, temp & 0x00FF);
    return 0;
}

uint8_t cpu6502::ROR() {
    fetch();
    temp = (uint16_t)(get_flag(C) << 7) | (fetched >> 1);
    set_flag(C, fetched & 0x01);
    set_flag(Z, (temp & 0x00FF) == 0);
    set_flag(N, temp & 0x0080);
    if (lookup[opcode].addrmode == &cpu6502::IMP) a = temp & 0x00FF;
    else write(addr_abs, temp & 0x00FF);
    return 0;
}

uint8_t cpu6502::RTI() {
    stkp++;
    status = read(0x0100 + stkp);
    status &= ~B;
    status &= ~U;
    stkp++;
    pc  = (uint16_t)read(0x0100 + stkp);
    stkp++;
    pc |= (uint16_t)read(0x0100 + stkp) << 8;
    return 0;
}

uint8_t cpu6502::RTS() {
    stkp++;
    pc  = (uint16_t)read(0x0100 + stkp);
    stkp++;
    pc |= (uint16_t)read(0x0100 + stkp) << 8;
    pc++;
    return 0;
}

uint8_t cpu6502::SEC() { set_flag(C, true); return 0; }
uint8_t cpu6502::SED() { set_flag(D, true); return 0; }
uint8_t cpu6502::SEI() { set_flag(I, true); return 0; }

uint8_t cpu6502::STA() { write(addr_abs, a); return 0; }
uint8_t cpu6502::STX() { write(addr_abs, x); return 0; }
uint8_t cpu6502::STY() { write(addr_abs, y); return 0; }

uint8_t cpu6502::TAX() { x = a; set_flag(Z, x == 0); set_flag(N, x & 0x80); return 0; }
uint8_t cpu6502::TAY() { y = a; set_flag(Z, y == 0); set_flag(N, y & 0x80); return 0; }
uint8_t cpu6502::TSX() { x = stkp; set_flag(Z, x == 0); set_flag(N, x & 0x80); return 0; }
uint8_t cpu6502::TXA() { a = x; set_flag(Z, a == 0); set_flag(N, a & 0x80); return 0; }
uint8_t cpu6502::TXS() { stkp = x; return 0; }
uint8_t cpu6502::TYA() { a = y; set_flag(Z, a == 0); set_flag(N, a & 0x80); return 0; }

uint8_t cpu6502::XXX() { return 0; }
