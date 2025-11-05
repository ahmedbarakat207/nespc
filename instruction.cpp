#include <iostream>
#include "bus.h"
#include "6502.h"

// Credits go to javidx9 for all of these instructios that i was too lazy to type them for scratch and thanks to him for helping me as much as possible in the emulator
// Don't forget to subscribe to his channel <3

// https://www.youtube.com/@javidx9

uint8_t cpu6502::ADC()
{
	// Grab the data that we are adding to the accumulator
	fetch();
	
	// Add is performed in 16-bit domain for emulation to capture any
	// carry bit, which will exist in bit 8 of the 16-bit word
	uint16_t temp = (uint16_t)a + (uint16_t)fetched + (uint16_t)get_flag(C);
	
	// The carry flag out exists in the high byte bit 0
	set_flag(C, temp > 255);
	
	// The Zero flag is set if the result is 0
	set_flag(Z, (temp & 0x00FF) == 0);
	
	// The signed Overflow flag is set based on all that up there! :D
	set_flag(V, (~((uint16_t)a ^ (uint16_t)fetched) & ((uint16_t)a ^ (uint16_t)temp)) & 0x0080);
	
	// The negative flag is set to the most significant bit of the result
	set_flag(N, temp & 0x80);
	
	// Load the result into the accumulator (it's 8-bit dont forget!)
	a = temp & 0x00FF;
	
	// This instruction has the potential to require an additional clock cycle
	return 1;
}



uint8_t cpu6502::SBC()
{
	fetch();
	
	// Operating in 16-bit domain to capture carry out
	
	// We can invert the bottom 8 bits with bitwise xor
	uint16_t value = ((uint16_t)fetched) ^ 0x00FF;
	
	// Notice this is exactly the same as addition from here!
	uint16_t temp = (uint16_t)a + value + (uint16_t)get_flag(C);
	set_flag(C, temp & 0xFF00);
	set_flag(Z, ((temp & 0x00FF) == 0));
	set_flag(V, (temp ^ (uint16_t)a) & (temp ^ value) & 0x0080);
	set_flag(N, temp & 0x0080);
	a = temp & 0x00FF;
	return 1;
}

uint8_t cpu6502::AND()
{
	fetch();
	a = a & fetched;
	set_flag(Z, a == 0x00);
	set_flag(N, a & 0x80);
	return 1;
}


// Instruction: Arithmetic Shift Left
// Function:    A = C <- (A << 1) <- 0
// Flags Out:   N, Z, C
uint8_t cpu6502::ASL()
{
	fetch();
	uint16_t temp = (uint16_t)fetched << 1;
	set_flag(C, (temp & 0xFF00) > 0);
	set_flag(Z, (temp & 0x00FF) == 0x00);
	set_flag(N, temp & 0x80);
	if (lookup[opcode].addrmode == &cpu6502::IMP)
		a = temp & 0x00FF;
	else
		write(addr_abs, temp & 0x00FF);
	return 0;
}


// Instruction: Branch if Carry Clear
// Function:    if(C == 0) pc = address 
uint8_t cpu6502::BCC()
{
	if (get_flag(C) == 0)
	{
		cycles++;
		addr_abs = pc + addr_rel;
		
		if((addr_abs & 0xFF00) != (pc & 0xFF00))
			cycles++;
		
		pc = addr_abs;
	}
	return 0;
}


// Instruction: Branch if Carry Set
// Function:    if(C == 1) pc = address
uint8_t cpu6502::BCS()
{
	if (get_flag(C) == 1)
	{
		cycles++;
		addr_abs = pc + addr_rel;

		if ((addr_abs & 0xFF00) != (pc & 0xFF00))
			cycles++;

		pc = addr_abs;
	}
	return 0;
}


// Instruction: Branch if Equal
// Function:    if(Z == 1) pc = address
uint8_t cpu6502::BEQ()
{
	if (get_flag(Z) == 1)
	{
		cycles++;
		addr_abs = pc + addr_rel;

		if ((addr_abs & 0xFF00) != (pc & 0xFF00))
			cycles++;

		pc = addr_abs;
	}
	return 0;
}

uint8_t cpu6502::BIT()
{
	fetch();
	temp = a & fetched;
	set_flag(Z, (temp & 0x00FF) == 0x00);
	set_flag(N, fetched & (1 << 7));
	set_flag(V, fetched & (1 << 6));
	return 0;
}


// Instruction: Branch if Negative
// Function:    if(N == 1) pc = address
uint8_t cpu6502::BMI()
{
	if (get_flag(N) == 1)
	{
		cycles++;
		addr_abs = pc + addr_rel;

		if ((addr_abs & 0xFF00) != (pc & 0xFF00))
			cycles++;

		pc = addr_abs;
	}
	return 0;
}


// Instruction: Branch if Not Equal
// Function:    if(Z == 0) pc = address
uint8_t cpu6502::BNE()
{
	if (get_flag(Z) == 0)
	{
		cycles++;
		addr_abs = pc + addr_rel;

		if ((addr_abs & 0xFF00) != (pc & 0xFF00))
			cycles++;

		pc = addr_abs;
	}
	return 0;
}


// Instruction: Branch if Positive
// Function:    if(N == 0) pc = address
uint8_t cpu6502::BPL()
{
	if (get_flag(N) == 0)
	{
		cycles++;
		addr_abs = pc + addr_rel;

		if ((addr_abs & 0xFF00) != (pc & 0xFF00))
			cycles++;

		pc = addr_abs;
	}
	return 0;
}

// Instruction: Break
// Function:    Program Sourced Interrupt
uint8_t cpu6502::BRK()
{
	pc++;
	
	set_flag(I, 1);
	write(0x0100 + stkp, (pc >> 8) & 0x00FF);
	stkp--;
	write(0x0100 + stkp, pc & 0x00FF);
	stkp--;

	set_flag(B, 1);
	write(0x0100 + stkp, status);
	stkp--;
	set_flag(B, 0);

	pc = (uint16_t)read(0xFFFE) | ((uint16_t)read(0xFFFF) << 8);
	return 0;
}


// Instruction: Branch if Overflow Clear
// Function:    if(V == 0) pc = address
uint8_t cpu6502::BVC()
{
	if (get_flag(V) == 0)
	{
		cycles++;
		addr_abs = pc + addr_rel;

		if ((addr_abs & 0xFF00) != (pc & 0xFF00))
			cycles++;

		pc = addr_abs;
	}
	return 0;
}


// Instruction: Branch if Overflow Set
// Function:    if(V == 1) pc = address
uint8_t cpu6502::BVS()
{
	if (get_flag(V) == 1)
	{
		cycles++;
		addr_abs = pc + addr_rel;

		if ((addr_abs & 0xFF00) != (pc & 0xFF00))
			cycles++;

		pc = addr_abs;
	}
	return 0;
}


// Instruction: Clear Carry Flag
// Function:    C = 0
uint8_t cpu6502::CLC()
{
	set_flag(C, false);
	return 0;
}


// Instruction: Clear Decimal Flag
// Function:    D = 0
uint8_t cpu6502::CLD()
{
	set_flag(D, false);
	return 0;
}


// Instruction: Disable Interrupts / Clear Interrupt Flag
// Function:    I = 0
uint8_t cpu6502::CLI()
{
	set_flag(I, false);
	return 0;
}


// Instruction: Clear Overflow Flag
// Function:    V = 0
uint8_t cpu6502::CLV()
{
	set_flag(V, false);
	return 0;
}

// Instruction: Compare Accumulator
// Function:    C <- A >= M      Z <- (A - M) == 0
// Flags Out:   N, C, Z
uint8_t cpu6502::CMP()
{
	fetch();
	temp = (uint16_t)a - (uint16_t)fetched;
	set_flag(C, a >= fetched);
	set_flag(Z, (temp & 0x00FF) == 0x0000);
	set_flag(N, temp & 0x0080);
	return 1;
}

// Instruction: Decrement Value at Memory Location
// Function:    M = M - 1
// Flags Out:   N, Z
uint8_t cpu6502::DEC()
{
	fetch();
	temp = fetched - 1;
	write(addr_abs, temp & 0x00FF);
	set_flag(Z, (temp & 0x00FF) == 0x0000);
	set_flag(N, temp & 0x0080);
	return 0;
}


// Instruction: Bitwise Logic XOR
// Function:    A = A xor M
// Flags Out:   N, Z
uint8_t cpu6502::EOR()
{
	fetch();
	a = a ^ fetched;	
	set_flag(Z, a == 0x00);
	set_flag(N, a & 0x80);
	return 1;
}


// Instruction: Increment Value at Memory Location
// Function:    M = M + 1
// Flags Out:   N, Z
uint8_t cpu6502::INC()
{
	fetch();
	temp = fetched + 1;
	write(addr_abs, temp & 0x00FF);
	set_flag(Z, (temp & 0x00FF) == 0x0000);
	set_flag(N, temp & 0x0080);
	return 0;
}


// Instruction: Jump To Location
// Function:    pc = address
uint8_t cpu6502::JMP()
{
	pc = addr_abs;
	return 0;
}


// Instruction: Jump To Sub-Routine
// Function:    Push current pc to stack, pc = address
uint8_t cpu6502::JSR()
{
	pc--;

	write(0x0100 + stkp, (pc >> 8) & 0x00FF);
	stkp--;
	write(0x0100 + stkp, pc & 0x00FF);
	stkp--;

	pc = addr_abs;
	return 0;
}


// Instruction: Load The Accumulator
// Function:    A = M
// Flags Out:   N, Z
uint8_t cpu6502::LDA()
{
	fetch();
	a = fetched;
	set_flag(Z, a == 0x00);
	set_flag(N, a & 0x80);
	return 1;
}
uint8_t cpu6502::LSR()
{
	fetch();
	set_flag(C, fetched & 0x0001);
	temp = fetched >> 1;	
	set_flag(Z, (temp & 0x00FF) == 0x0000);
	set_flag(N, temp & 0x0080);
	if (lookup[opcode].addrmode == &cpu6502::IMP)
		a = temp & 0x00FF;
	else
		write(addr_abs, temp & 0x00FF);
	return 0;
}

uint8_t cpu6502::NOP()
{
	// Sadly not all NOPs are equal, Ive added a few here
	// based on https://wiki.nesdev.com/w/index.php/CPU_unofficial_opcodes
	// and will add more based on game compatibility, and ultimately
	// I'd like to cover all illegal opcodes too
	switch (opcode) {
	case 0x1C:
	case 0x3C:
	case 0x5C:
	case 0x7C:
	case 0xDC:
	case 0xFC:
		return 1;
		break;
	}
	return 0;
}


// Instruction: Bitwise Logic OR
// Function:    A = A | M
// Flags Out:   N, Z
uint8_t cpu6502::ORA()
{
	fetch();
	a = a | fetched;
	set_flag(Z, a == 0x00);
	set_flag(N, a & 0x80);
	return 1;
}


uint8_t cpu6502::ROL()
{
	fetch();
	temp = (uint16_t)(fetched << 1) | get_flag(C);
	set_flag(C, temp & 0xFF00);
	set_flag(Z, (temp & 0x00FF) == 0x0000);
	set_flag(N, temp & 0x0080);
	if (lookup[opcode].addrmode == &cpu6502::IMP)
		a = temp & 0x00FF;
	else
		write(addr_abs, temp & 0x00FF);
	return 0;
}

uint8_t cpu6502::ROR()
{
	fetch();
	temp = (uint16_t)(get_flag(C) << 7) | (fetched >> 1);
	set_flag(C, fetched & 0x01);
	set_flag(Z, (temp & 0x00FF) == 0x00);
	set_flag(N, temp & 0x0080);
	if (lookup[opcode].addrmode == &cpu6502::IMP)
		a = temp & 0x00FF;
	else
		write(addr_abs, temp & 0x00FF);
	return 0;
}

uint8_t cpu6502::RTI()
{
	stkp++;
	status = read(0x0100 + stkp);
	status &= ~B;
	status &= ~U;

	stkp++;
	pc = (uint16_t)read(0x0100 + stkp);
	stkp++;
	pc |= (uint16_t)read(0x0100 + stkp) << 8;
	return 0;
}

uint8_t cpu6502::RTS()
{
	stkp++;
	pc = (uint16_t)read(0x0100 + stkp);
	stkp++;
	pc |= (uint16_t)read(0x0100 + stkp) << 8;
	
	pc++;
	return 0;
}




// Instruction: Set Carry Flag
// Function:    C = 1
uint8_t cpu6502::SEC()
{
	set_flag(C, true);
	return 0;
}


// Instruction: Set Decimal Flag
// Function:    D = 1
uint8_t cpu6502::SED()
{
	set_flag(D, true);
	return 0;
}


// Instruction: Set Interrupt Flag / Enable Interrupts
// Function:    I = 1
uint8_t cpu6502::SEI()
{
	set_flag(I, true);
	return 0;
}


// Instruction: Store Accumulator at Address
// Function:    M = A
uint8_t cpu6502::STA()
{
	write(addr_abs, a);
	return 0;
}


// Instruction: Transfer Accumulator to X Register
// Function:    X = A
// Flags Out:   N, Z
uint8_t cpu6502::TAX()
{
	x = a;
	set_flag(Z, x == 0x00);
	set_flag(N, x & 0x80);
	return 0;
}


// Instruction: Transfer Accumulator to Y Register
// Function:    Y = A
// Flags Out:   N, Z
uint8_t cpu6502::TAY()
{
	y = a;
	set_flag(Z, y == 0x00);
	set_flag(N, y & 0x80);
	return 0;
}


// Instruction: Transfer Stack Pointer to X Register
// Function:    X = stack pointer
// Flags Out:   N, Z
uint8_t cpu6502::TSX()
{
	x = stkp;
	set_flag(Z, x == 0x00);
	set_flag(N, x & 0x80);
	return 0;
}




// This function captures illegal opcodes
uint8_t cpu6502::XXX()
{
	return 0;
}
// Add these to your instruction implementations
uint8_t cpu6502::LDX() {
    fetch();
    x = fetched;
    set_flag(Z, x == 0x00);
    set_flag(N, x & 0x80);
    return 1;
}

uint8_t cpu6502::LDY() {
    fetch();
    y = fetched;
    set_flag(Z, y == 0x00);
    set_flag(N, y & 0x80);
    return 1;
}

uint8_t cpu6502::STX() {
    write(addr_abs, x);
    return 0;
}

uint8_t cpu6502::STY() {
    write(addr_abs, y);
    return 0;
}

uint8_t cpu6502::TXA() {
    a = x;
    set_flag(Z, a == 0x00);
    set_flag(N, a & 0x80);
    return 0;
}

uint8_t cpu6502::TYA() {
    a = y;
    set_flag(Z, a == 0x00);
    set_flag(N, a & 0x80);
    return 0;
}

uint8_t cpu6502::TXS() {
    stkp = x;
    return 0;
}
uint8_t cpu6502::PHA() {
    write(0x0100 + stkp, a);
    stkp--;
    return 0;
}

uint8_t cpu6502::PLA() {
    stkp++;
    a = read(0x0100 + stkp);
    set_flag(Z, a == 0x00);
    set_flag(N, a & 0x80);
    return 0;
}

uint8_t cpu6502::PHP() {
    write(0x0100 + stkp, status | B | U);
    stkp--;
    return 0;
}

uint8_t cpu6502::PLP() {
    stkp++;
    status = read(0x0100 + stkp);
    status |= U;
    return 0;
}

uint8_t cpu6502::INX() {
    x++;
    set_flag(Z, x == 0x00);
    set_flag(N, x & 0x80);
    return 0;
}

uint8_t cpu6502::INY() {
    y++;
    set_flag(Z, y == 0x00);
    set_flag(N, y & 0x80);
    return 0;
}

uint8_t cpu6502::DEX() {
    x--;
    set_flag(Z, x == 0x00);
    set_flag(N, x & 0x80);
    return 0;
}

uint8_t cpu6502::DEY() {
    y--;
    set_flag(Z, y == 0x00);
    set_flag(N, y & 0x80);
    return 0;
}

uint8_t cpu6502::CPX() {
    fetch();
    uint16_t temp = (uint16_t)x - (uint16_t)fetched;
    set_flag(C, x >= fetched);
    set_flag(Z, (temp & 0x00FF) == 0x0000);
    set_flag(N, temp & 0x0080);
    return 0;
}

uint8_t cpu6502::CPY() {
    fetch();
    uint16_t temp = (uint16_t)y - (uint16_t)fetched;
    set_flag(C, y >= fetched);
    set_flag(Z, (temp & 0x00FF) == 0x0000);
    set_flag(N, temp & 0x0080);
    return 0;
}
