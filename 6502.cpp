#include "6502.h"
#include "bus.h"
#include <iostream>

cpu6502::~cpu6502() {}

uint8_t cpu6502::get_flag(FLAGS6502 flag) {
    return ((status & flag) > 0) ? 1 : 0;
}

void cpu6502::set_flag(FLAGS6502 flag, bool value) {
    if (value)
        status |= flag;
    else
        status &= ~flag;
}

cpu6502::cpu6502() {
    initializeLookupTable();
}

void cpu6502::initializeLookupTable() {
    using a = cpu6502;
    
    // Initialize all opcodes first
    for (int i = 0; i < 256; i++) {
        lookup[i] = { "???", &a::XXX, &a::IMP, 2 };
    }
    
    // Complete the instruction set - ADD THESE
    lookup[0x69] = { "ADC", &a::ADC, &a::IMM, 2 };
    lookup[0x65] = { "ADC", &a::ADC, &a::ZP0, 3 };
    lookup[0x75] = { "ADC", &a::ADC, &a::ZPX, 4 };
    lookup[0x6D] = { "ADC", &a::ADC, &a::ABS, 4 };
    lookup[0x7D] = { "ADC", &a::ADC, &a::ABX, 4 };
    lookup[0x79] = { "ADC", &a::ADC, &a::ABY, 4 };
    lookup[0x61] = { "ADC", &a::ADC, &a::IZX, 6 };
    lookup[0x71] = { "ADC", &a::ADC, &a::IZY, 5 };

    lookup[0x29] = { "AND", &a::AND, &a::IMM, 2 };
    lookup[0x25] = { "AND", &a::AND, &a::ZP0, 3 };
    lookup[0x35] = { "AND", &a::AND, &a::ZPX, 4 };
    lookup[0x2D] = { "AND", &a::AND, &a::ABS, 4 };
    lookup[0x3D] = { "AND", &a::AND, &a::ABX, 4 };
    lookup[0x39] = { "AND", &a::AND, &a::ABY, 4 };
    lookup[0x21] = { "AND", &a::AND, &a::IZX, 6 };
    lookup[0x31] = { "AND", &a::AND, &a::IZY, 5 };

    lookup[0x0A] = { "ASL", &a::ASL, &a::IMP, 2 };
    lookup[0x06] = { "ASL", &a::ASL, &a::ZP0, 5 };
    lookup[0x16] = { "ASL", &a::ASL, &a::ZPX, 6 };
    lookup[0x0E] = { "ASL", &a::ASL, &a::ABS, 6 };
    lookup[0x1E] = { "ASL", &a::ASL, &a::ABX, 7 };

    lookup[0x90] = { "BCC", &a::BCC, &a::REL, 2 };
    lookup[0xB0] = { "BCS", &a::BCS, &a::REL, 2 };
    lookup[0xF0] = { "BEQ", &a::BEQ, &a::REL, 2 };
    lookup[0x30] = { "BMI", &a::BMI, &a::REL, 2 };
    lookup[0xD0] = { "BNE", &a::BNE, &a::REL, 2 };
    lookup[0x10] = { "BPL", &a::BPL, &a::REL, 2 };
    lookup[0x50] = { "BVC", &a::BVC, &a::REL, 2 };
    lookup[0x70] = { "BVS", &a::BVS, &a::REL, 2 };

    lookup[0x18] = { "CLC", &a::CLC, &a::IMP, 2 };
    lookup[0xD8] = { "CLD", &a::CLD, &a::IMP, 2 };
    lookup[0x58] = { "CLI", &a::CLI, &a::IMP, 2 };
    lookup[0xB8] = { "CLV", &a::CLV, &a::IMP, 2 };

    lookup[0xC9] = { "CMP", &a::CMP, &a::IMM, 2 };
    lookup[0xC5] = { "CMP", &a::CMP, &a::ZP0, 3 };
    lookup[0xD5] = { "CMP", &a::CMP, &a::ZPX, 4 };
    lookup[0xCD] = { "CMP", &a::CMP, &a::ABS, 4 };
    lookup[0xDD] = { "CMP", &a::CMP, &a::ABX, 4 };
    lookup[0xD9] = { "CMP", &a::CMP, &a::ABY, 4 };
    lookup[0xC1] = { "CMP", &a::CMP, &a::IZX, 6 };
    lookup[0xD1] = { "CMP", &a::CMP, &a::IZY, 5 };

    lookup[0xE0] = { "CPX", &a::CPX, &a::IMM, 2 };
    lookup[0xE4] = { "CPX", &a::CPX, &a::ZP0, 3 };
    lookup[0xEC] = { "CPX", &a::CPX, &a::ABS, 4 };

    lookup[0xC0] = { "CPY", &a::CPY, &a::IMM, 2 };
    lookup[0xC4] = { "CPY", &a::CPY, &a::ZP0, 3 };
    lookup[0xCC] = { "CPY", &a::CPY, &a::ABS, 4 };

    lookup[0xC6] = { "DEC", &a::DEC, &a::ZP0, 5 };
    lookup[0xD6] = { "DEC", &a::DEC, &a::ZPX, 6 };
    lookup[0xCE] = { "DEC", &a::DEC, &a::ABS, 6 };
    lookup[0xDE] = { "DEC", &a::DEC, &a::ABX, 7 };

    lookup[0xCA] = { "DEX", &a::DEX, &a::IMP, 2 };
    lookup[0x88] = { "DEY", &a::DEY, &a::IMP, 2 };

    lookup[0x49] = { "EOR", &a::EOR, &a::IMM, 2 };
    lookup[0x45] = { "EOR", &a::EOR, &a::ZP0, 3 };
    lookup[0x55] = { "EOR", &a::EOR, &a::ZPX, 4 };
    lookup[0x4D] = { "EOR", &a::EOR, &a::ABS, 4 };
    lookup[0x5D] = { "EOR", &a::EOR, &a::ABX, 4 };
    lookup[0x59] = { "EOR", &a::EOR, &a::ABY, 4 };
    lookup[0x41] = { "EOR", &a::EOR, &a::IZX, 6 };
    lookup[0x51] = { "EOR", &a::EOR, &a::IZY, 5 };

    lookup[0xE6] = { "INC", &a::INC, &a::ZP0, 5 };
    lookup[0xF6] = { "INC", &a::INC, &a::ZPX, 6 };
    lookup[0xEE] = { "INC", &a::INC, &a::ABS, 6 };
    lookup[0xFE] = { "INC", &a::INC, &a::ABX, 7 };

    lookup[0xE8] = { "INX", &a::INX, &a::IMP, 2 };
    lookup[0xC8] = { "INY", &a::INY, &a::IMP, 2 };

    lookup[0x4C] = { "JMP", &a::JMP, &a::ABS, 3 };
    lookup[0x6C] = { "JMP", &a::JMP, &a::IND, 5 };

    lookup[0x20] = { "JSR", &a::JSR, &a::ABS, 6 };

    lookup[0xA9] = { "LDA", &a::LDA, &a::IMM, 2 };
    lookup[0xA5] = { "LDA", &a::LDA, &a::ZP0, 3 };
    lookup[0xB5] = { "LDA", &a::LDA, &a::ZPX, 4 };
    lookup[0xAD] = { "LDA", &a::LDA, &a::ABS, 4 };
    lookup[0xBD] = { "LDA", &a::LDA, &a::ABX, 4 };
    lookup[0xB9] = { "LDA", &a::LDA, &a::ABY, 4 };
    lookup[0xA1] = { "LDA", &a::LDA, &a::IZX, 6 };
    lookup[0xB1] = { "LDA", &a::LDA, &a::IZY, 5 };

    lookup[0xA2] = { "LDX", &a::LDX, &a::IMM, 2 };
    lookup[0xA6] = { "LDX", &a::LDX, &a::ZP0, 3 };
    lookup[0xB6] = { "LDX", &a::LDX, &a::ZPY, 4 };
    lookup[0xAE] = { "LDX", &a::LDX, &a::ABS, 4 };
    lookup[0xBE] = { "LDX", &a::LDX, &a::ABY, 4 };

    lookup[0xA0] = { "LDY", &a::LDY, &a::IMM, 2 };
    lookup[0xA4] = { "LDY", &a::LDY, &a::ZP0, 3 };
    lookup[0xB4] = { "LDY", &a::LDY, &a::ZPX, 4 };
    lookup[0xAC] = { "LDY", &a::LDY, &a::ABS, 4 };
    lookup[0xBC] = { "LDY", &a::LDY, &a::ABX, 4 };

    lookup[0x4A] = { "LSR", &a::LSR, &a::IMP, 2 };
    lookup[0x46] = { "LSR", &a::LSR, &a::ZP0, 5 };
    lookup[0x56] = { "LSR", &a::LSR, &a::ZPX, 6 };
    lookup[0x4E] = { "LSR", &a::LSR, &a::ABS, 6 };
    lookup[0x5E] = { "LSR", &a::LSR, &a::ABX, 7 };

    lookup[0xEA] = { "NOP", &a::NOP, &a::IMP, 2 };

    lookup[0x09] = { "ORA", &a::ORA, &a::IMM, 2 };
    lookup[0x05] = { "ORA", &a::ORA, &a::ZP0, 3 };
    lookup[0x15] = { "ORA", &a::ORA, &a::ZPX, 4 };
    lookup[0x0D] = { "ORA", &a::ORA, &a::ABS, 4 };
    lookup[0x1D] = { "ORA", &a::ORA, &a::ABX, 4 };
    lookup[0x19] = { "ORA", &a::ORA, &a::ABY, 4 };
    lookup[0x01] = { "ORA", &a::ORA, &a::IZX, 6 };
    lookup[0x11] = { "ORA", &a::ORA, &a::IZY, 5 };

    lookup[0x48] = { "PHA", &a::PHA, &a::IMP, 3 };
    lookup[0x08] = { "PHP", &a::PHP, &a::IMP, 3 };
    lookup[0x68] = { "PLA", &a::PLA, &a::IMP, 4 };
    lookup[0x28] = { "PLP", &a::PLP, &a::IMP, 4 };

    lookup[0x2A] = { "ROL", &a::ROL, &a::IMP, 2 };
    lookup[0x26] = { "ROL", &a::ROL, &a::ZP0, 5 };
    lookup[0x36] = { "ROL", &a::ROL, &a::ZPX, 6 };
    lookup[0x2E] = { "ROL", &a::ROL, &a::ABS, 6 };
    lookup[0x3E] = { "ROL", &a::ROL, &a::ABX, 7 };

    lookup[0x6A] = { "ROR", &a::ROR, &a::IMP, 2 };
    lookup[0x66] = { "ROR", &a::ROR, &a::ZP0, 5 };
    lookup[0x76] = { "ROR", &a::ROR, &a::ZPX, 6 };
    lookup[0x6E] = { "ROR", &a::ROR, &a::ABS, 6 };
    lookup[0x7E] = { "ROR", &a::ROR, &a::ABX, 7 };

    lookup[0x40] = { "RTI", &a::RTI, &a::IMP, 6 };
    lookup[0x60] = { "RTS", &a::RTS, &a::IMP, 6 };

    lookup[0xE9] = { "SBC", &a::SBC, &a::IMM, 2 };
    lookup[0xE5] = { "SBC", &a::SBC, &a::ZP0, 3 };
    lookup[0xF5] = { "SBC", &a::SBC, &a::ZPX, 4 };
    lookup[0xED] = { "SBC", &a::SBC, &a::ABS, 4 };
    lookup[0xFD] = { "SBC", &a::SBC, &a::ABX, 4 };
    lookup[0xF9] = { "SBC", &a::SBC, &a::ABY, 4 };
    lookup[0xE1] = { "SBC", &a::SBC, &a::IZX, 6 };
    lookup[0xF1] = { "SBC", &a::SBC, &a::IZY, 5 };

    lookup[0x38] = { "SEC", &a::SEC, &a::IMP, 2 };
    lookup[0xF8] = { "SED", &a::SED, &a::IMP, 2 };
    lookup[0x78] = { "SEI", &a::SEI, &a::IMP, 2 };

    lookup[0x85] = { "STA", &a::STA, &a::ZP0, 3 };
    lookup[0x95] = { "STA", &a::STA, &a::ZPX, 4 };
    lookup[0x8D] = { "STA", &a::STA, &a::ABS, 4 };
    lookup[0x9D] = { "STA", &a::STA, &a::ABX, 5 };
    lookup[0x99] = { "STA", &a::STA, &a::ABY, 5 };
    lookup[0x81] = { "STA", &a::STA, &a::IZX, 6 };
    lookup[0x91] = { "STA", &a::STA, &a::IZY, 6 };

    lookup[0x86] = { "STX", &a::STX, &a::ZP0, 3 };
    lookup[0x96] = { "STX", &a::STX, &a::ZPY, 4 };
    lookup[0x8E] = { "STX", &a::STX, &a::ABS, 4 };

    lookup[0x84] = { "STY", &a::STY, &a::ZP0, 3 };
    lookup[0x94] = { "STY", &a::STY, &a::ZPX, 4 };
    lookup[0x8C] = { "STY", &a::STY, &a::ABS, 4 };

    lookup[0xAA] = { "TAX", &a::TAX, &a::IMP, 2 };
    lookup[0xA8] = { "TAY", &a::TAY, &a::IMP, 2 };
    lookup[0xBA] = { "TSX", &a::TSX, &a::IMP, 2 };
    lookup[0x8A] = { "TXA", &a::TXA, &a::IMP, 2 };
    lookup[0x9A] = { "TXS", &a::TXS, &a::IMP, 2 };
    lookup[0x98] = { "TYA", &a::TYA, &a::IMP, 2 };
}

uint8_t cpu6502::read(uint16_t addr){
    if (!bus) {
        return 0x00;
    }
    return bus->cpuRead(addr);
}

void cpu6502::write(uint16_t addr, uint8_t data){
    if (bus) {
        bus->cpuWrite(addr, data);
    }
}

void cpu6502::clock(){
    if (cycles == 0){
        opcode = read(pc);
        
        // Debug: Print the first 100 instructions
        static int instruction_count = 0;
        if (instruction_count < 100) {
            printf("CPU: PC=%04X, Opcode=%02X, A=%02X, X=%02X, Y=%02X, SP=%02X, Status=%02X\n", 
                   pc, opcode, a, x, y, stkp, status);
            instruction_count++;
        }
        
        pc++;

        cycles = lookup[opcode].cycles;

        uint8_t add_cycle1 = (this->*lookup[opcode].addrmode)();
        uint8_t add_cycle2 = (this->*lookup[opcode].operate)();

        cycles += (add_cycle1 & add_cycle2);
    }

    cycles--;
}

uint8_t cpu6502::fetch(){
    if (!(lookup[opcode].addrmode == &cpu6502::IMP)){
        fetched = read(addr_abs);
    }
    return fetched;
}

void cpu6502::reset(){
    printf("CPU Reset started\n");
    
    a = 0;
    x = 0;
    y = 0;
    stkp = 0xFD;
    status = 0x00 | U;

    addr_abs = 0xFFFC;
    printf("Reading reset vector from %04X\n", addr_abs);
    
    uint16_t lo = read(addr_abs + 0);
    uint16_t hi = read(addr_abs + 1);
    
    printf("Reset vector: %02X %02X\n", lo, hi);
    
    pc = (hi << 8) | lo;
    printf("PC set to: %04X\n", pc);

    addr_rel = 0x0000;
    addr_abs = 0x0000;
    fetched = 0x00;

    cycles = 8;
    printf("CPU Reset completed\n");
}

void cpu6502::irq(){
    if (get_flag(I) == 0){
        write(0x0100 + stkp, (pc >> 8) & 0x00FF);
        stkp--;
        write(0x0100 + stkp, pc & 0x00FF);
        stkp--;

        set_flag(B, 0);
        set_flag(U, 1);
        set_flag(I, 1);
        write(0x0100 + stkp, status);
        stkp--;

        addr_abs = 0xFFFE;
        uint16_t lo = read(addr_abs + 0);
        uint16_t hi = read(addr_abs + 1);
        pc = (hi << 8) | lo;

        cycles = 7;
    }
}

void cpu6502::nmi(){
    printf("NMI triggered\n");
    write(0x0100 + stkp, (pc >> 8) & 0x00FF);
    stkp--;
    write(0x0100 + stkp, pc & 0x00FF);
    stkp--;

    set_flag(B, 0);
    set_flag(U, 1);
    set_flag(I, 1);
    write(0x0100 + stkp, status);
    stkp--;

    addr_abs = 0xFFFA;
    uint16_t lo = read(addr_abs + 0);
    uint16_t hi = read(addr_abs + 1);
    pc = (hi << 8) | lo;

    cycles = 8;
}

bool cpu6502::complete()
{
    return cycles == 0;
}
void cpu6502::push(uint8_t data) {
    write(0x0100 + stkp, data);
    stkp--;
}

uint8_t cpu6502::pop() {
    stkp++;
    return read(0x0100 + stkp);
}