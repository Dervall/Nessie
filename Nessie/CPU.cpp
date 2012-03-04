#include "CPU.h"
#include <memory.h>
#include <stdio.h>
#include "CPUMem.h"
#include "NES.h"
#include "Emulator.h"
#include "PPU.h"

/* Bit0 - C - Carry flag: this holds the carry out of the most significant
   bit in any arithmetic operation. In subtraction operations however, this
   flag is cleared - set to 0 - if a borrow is required, set to 1 - if no
   borrow is required. The carry flag is also used in shift and rotate
   logical operations. */
#define FLAG_C (0x1)

   /* Bit1 - Z - Zero flag: this is set to 1 when any arithmetic or logical
   operation produces a zero result, and is set to 0 if the result is
   non-zero. */
#define FLAG_Z (0x2)

   /*	Bit 2 - I: this is an interrupt enable/disable flag. If it is set,
		interrupts are disabled. If it is cleared, interrupts are enabled. */
#define FLAG_I (0x4)

   /*  Bit 3 - D: this is the decimal mode status flag. When set, and an Add with 
	   Carry or Subtract with Carry instruction is executed, the source values are
	   treated as valid BCD (Binary Coded Decimal, eg. 0x00-0x99 = 0-99) numbers.
	   The result generated is also a BCD number. */
#define FLAG_D (0x8)

   /*	Bit 4 - B: this is set when a software interrupt (BRK instruction) is
		executed. */
#define FLAG_B (0x10)

   /*   Bit 6 - V - Overflow flag: when an arithmetic operation produces a result
		too large to be represented in a byte, V is set */
#define FLAG_V (0x40)

   /*   Bit 7 - S - Sign flag: this is set if the result of an operation is
		negative, cleared if positive. */
#define FLAG_N (0x80)

#define SETFLAG(A, B) A |= B
#define CLEARFLAG(A, B) A &= ~B
#define TESTFLAG(A,B) (A & B)

// Naive implementation to get things rolling
#define SETFLAGIFTRUE(A, B, C) if (C) { SETFLAG(A, B); } else { CLEARFLAG(A, B); }

#define SET_N_Z(A, B) SETFLAGIFTRUE(A, FLAG_Z, B == 0); SETFLAGIFTRUE(A, FLAG_N, B & 0x80);
#define SET_N_Z_C(A, B) SETFLAGIFTRUE(A, FLAG_Z, B == 0); SETFLAGIFTRUE(A, FLAG_N, B & 0x80); SETFLAGIFTRUE(A, FLAG_C, B >= 0x100);

//						 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F		
const BYTE instrCycleCount[] = { 
				/* 0 */  7, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 0, 4, 6, 0,
                /* 1 */  2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
                /* 2 */  6, 6, 0, 0, 3, 3, 5, 0, 4, 2, 2, 0, 4, 4, 6, 0,
                /* 3 */  2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
                /* 4 */ 13, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 3, 4, 6, 0,
                /* 5 */  2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
                /* 6 */  6, 6, 0, 0, 0, 3, 5, 0, 4, 2, 2, 0, 5, 4, 6, 0,
                /* 7 */  2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
                /* 8 */  0, 6, 0, 0, 3, 3, 3, 0, 2, 0, 2, 0, 4, 4, 4, 0,
                /* 9 */  2, 6, 0, 0, 4, 4, 4, 0, 2, 5, 2, 0, 0, 5, 0, 0,
                /* A */  2, 6, 2, 2, 3, 3, 3, 2, 2, 2, 2, 2, 4, 4, 4, 2,
                /* B */  2, 5, 2, 2, 4, 4, 4, 2, 2, 4, 2, 2, 4, 4, 4, 2,
                /* C */  2, 6, 2, 2, 3, 3, 5, 2, 2, 2, 2, 2, 4, 4, 6, 2,
                /* D */  2, 5, 2, 2, 2, 4, 6, 2, 2, 4, 2, 2, 2, 4, 7, 2,
                /* E */  2, 6, 2, 2, 3, 3, 5, 2, 2, 2, 2, 2, 4, 4, 6, 2,
                /* F */  2, 5, 2, 2, 2, 4, 6, 2, 2, 4, 2, 2, 2, 4, 7, 2
};

CPU::CPU(CPUMem* p, Emulator* pEmu) {
	pMemory = p;
	pEmulator = pEmu;
}

CPU::~CPU() {
}

void CPU::reset() {
	// Assumes that stuff is loaded
	P = pMemory->getInitialProgramCounter();
	A = 0;
	F = 1 << 5;
	Y = X = S = 0;

	cyclesLeftOnScanline = NUM_CYCLES_PER_SCANLINE;
	scanline = 0;
}

void CPU::run() {
	// Load instruction
	BYTE opCode = readMem(P++);
	int extraCycles = 0;

	// Execute instrution
	switch (opCode) {
		//
		// ADC
		//			
	case 0x69: A = addWithCarry(A, readMemImmediate()); break;
	case 0x65: A = addWithCarry(A, readMemZeroPage()); break;
	case 0x75: A = addWithCarry(A, readMemZeroPageOffset(X)); break;
	case 0x6D: A = addWithCarry(A, readMemAbsolute()); break;
	case 0x7D: A = addWithCarry(A, readMemAbsoluteOffset(X)); break;
	case 0x79: A = addWithCarry(A, readMemAbsoluteOffset(Y)); break;
	case 0x61: A = addWithCarry(A, readMemPreIndexedIndirect()); break;
	case 0x71: A = addWithCarry(A, readMemPostIndexedIndirect()); break;

		//
		// AND
		//
	case 0x29:
		A &= readMemImmediate();
		SET_N_Z(F, A);
		break;
	case 0x25:
		A &= readMemZeroPage();
		SET_N_Z(F, A);
		break;
	case 0x35:
		A &= readMemZeroPageOffset(X);
		SET_N_Z(F, A);
		break;
	case 0x2D:
		A &= readMemAbsolute();
		SET_N_Z(F, A);
		break;
	case 0x3D:
		A &= readMemAbsoluteOffset(X);
		SET_N_Z(F, A);
		break;
	case 0x39:
		A &= readMemAbsoluteOffset(Y);
		SET_N_Z(F, A);
		break;
	case 0x21:
		A &= readMemPreIndexedIndirect();
		SET_N_Z(F, A);
		break;
	case 0x31:
		A &= readMemPostIndexedIndirect();
		SET_N_Z(F, A);
		break;

		//
		// BCC
		//
	case 0x90: branchIfNotFlag(FLAG_C); break;

		//
		// BCS
		//
	case 0xB0:
		extraCycles = branchIfFlag(FLAG_C);
		break;

		//
		// BEQ
		//
	case 0xF0:
		extraCycles = branchIfFlag(FLAG_Z);
		break;

		//
		// BNE
		//
	case 0xD0:
		extraCycles = branchIfNotFlag(FLAG_Z);
		break;

		//
		// BPL Branch on result plus
		//
	case 0x10:
		extraCycles = branchIfNotFlag(FLAG_N);
		break;

		//
		// CLC
		//
	case 0x18:
		CLEARFLAG(F, FLAG_C);
		break;

		//
		// CLD		
		//
	case 0xD8:		
		CLEARFLAG(F, FLAG_D);
		break;

		//
		// CLI
		//
	case 0x58:
		CLEARFLAG(F, FLAG_I);
		break;

		//
		// CLV
		//
	case 0xB8:
		CLEARFLAG(F, FLAG_V);
		break;

		//
		// CPX
		//
	case 0xE0: // Immediate
		{
			WORD src = (WORD)X - (WORD)readMemImmediate();
			SET_N_Z_C(F, src);
		}
		break;
	case 0xE4: // Zero page
		{
			WORD src = (WORD)X - (WORD)readMemZeroPage();
			SET_N_Z_C(F, src);
		}
		break;
	case 0xEC: // Absolute
		{
			WORD src = (WORD)X - (WORD)readMemAbsolute();
			SET_N_Z_C(F, src);
		}
		break;

		//
		// CMP
		//	
	case 0xC9:
		{
			WORD src = (WORD)A - (WORD)readMemImmediate();
			SET_N_Z_C(F, src);
		}
		break;
	case 0xC5:
		{
			WORD src = (WORD)A - (WORD)readMemZeroPage();
			SET_N_Z_C(F, src);
		}
		break;
	case 0xD5:
		{
			WORD src = (WORD)A - (WORD)readMemZeroPageOffset(X);
			SET_N_Z_C(F, src);
		}
		break;
	case 0xCD:
		{
			WORD src = (WORD)A - (WORD)readMemAbsolute();
			SET_N_Z_C(F, src);
		}
		break;
	case 0xDD:
		{
			WORD src = (WORD)A - (WORD)readMemAbsoluteOffset(X);
			SET_N_Z_C(F, src);
		}
		break;
	case 0xD9:
		{
			WORD src = (WORD)A - (WORD)readMemAbsoluteOffset(Y);
			SET_N_Z_C(F, src);
		}
		break;
	case 0xC1:
		{
			WORD src = (WORD)A - (WORD)readMemPreIndexedIndirect();
			SET_N_Z_C(F, src);
		}
		break;
	case 0xD1:
		{
			WORD src = (WORD)A - (WORD)readMemPostIndexedIndirect();
			SET_N_Z_C(F, src);
		}
		break;

		//
		// DEX
		//
	case 0xCA:
		--X;
		SET_N_Z(F, X);
		break;

		//
		// DEY
		//
	case 0x88:
		--Y;
		SET_N_Z(F, Y);
		break;

		//
		// EOR
		//
	case 0x49:
		A ^= readMemImmediate();
		SET_N_Z(F, A);
		break;
	case 0x45:
		A ^= readMemZeroPage();
		SET_N_Z(F, A);
		break;
	case 0x55:
		A ^= readMemZeroPageOffset(X);
		SET_N_Z(F, A);
		break;
	case 0x4D:
		A ^= readMemAbsolute();
		SET_N_Z(F, A);
		break;
	case 0x5D:
		A ^= readMemAbsoluteOffset(X);
		SET_N_Z(F, A);
		break;
	case 0x59:
		A ^= readMemAbsoluteOffset(Y);
		SET_N_Z(F, A);
		break;
	case 0x41:
		A ^= readMemPreIndexedIndirect();
		SET_N_Z(F, A);
		break;
	case 0x51:
		A ^= readMemPostIndexedIndirect();
		SET_N_Z(F, A);
		break;

		//
		// INC
		//
	case 0xE6:
		incMem(getAddressZeroPage());
		break;
	case 0xF6:
		incMem(getAddressZeroPageOffset(X));
		break;
	case 0xEE:
		incMem(getAddressAbsolute());
		break;
	case 0xFE:
		incMem(getAddressAbsoluteOffset(X));
		break;

		//
		// INX
		//
	case 0xE8:
		++X;
		SET_N_Z(F, X);
		break;

		//
		// INY
		//
	case 0xC8:
		++Y;
		SET_N_Z(F, Y);
		break;

		//
		// JMP
		//
	case 0x4C:
		P = getAddressAbsolute();
		break;
	case 0x6C:
		{
			WORD address = getAddressAbsolute();
			BYTE low = readMem(address);
			BYTE high = readMem(address + 1);
			P = low;
			P |= ((WORD)high) << 8;
		}
		break;

		//
		// JSR
		//
	case 0x20:
		{
			WORD destination = getAddressAbsolute();			
			--P;
			push((P >> 8) & 0xFF);
			push(P & 0xFF);
			P = destination;
		}			
		break;

		//
		// LDA Load accumulator with memory 
		//
	case 0xA9:		// Immediate
		A = readMemImmediate();
		SET_N_Z(F, A);
		break;
	case 0xA5:
		A = readMemZeroPage();
		SET_N_Z(F, A);
		break;
	case 0xB5:
		A = readMemZeroPageOffset(X);
		SET_N_Z(F, A);
		break;
	case 0xAD:
		A = readMemAbsolute();
		SET_N_Z(F, A);		
		break;
	case 0xBD:
		A = readMemAbsoluteOffset(X);
		SET_N_Z(F, A);
		break;
	case 0xB9:
		A = readMemAbsoluteOffset(Y);
		SET_N_Z(F, A);
		break;
	case 0xA1:
		A = readMemPreIndexedIndirect();
		SET_N_Z(F, A);
		break;
	case 0xB1:
		A = readMemPostIndexedIndirect();
		SET_N_Z(F, A);
		break;

		//
		// LDX Load index X with memory
		//
	case 0xA2: // Immediate
		X = readMemImmediate();		
		SET_N_Z(F, X);		
		break;
	case 0xA6:	// Zero page
		X = readMemZeroPage();
		SET_N_Z(F, X);
		break;
	case 0xB6:	// Zero page,Y
		X = readMemZeroPageOffset(Y);
		SET_N_Z(F, X);
		break;
	case 0xAE:	// Absolute		
		X = readMemAbsolute();
		SET_N_Z(F, X);
		break;
	case 0xBE:	// Absolute,Y
		X = readMemAbsoluteOffset(Y);
		SET_N_Z(F, X);
		break;

		//
		// LDY
		//
	case 0xA0: // Immediate
		Y = readMemImmediate();		
		SET_N_Z(F, Y);		
		break;
	case 0xA4:	// Zero page
		Y = readMemZeroPage();
		SET_N_Z(F, Y);
		break;
	case 0xB4:	// Zero page,X
		Y = readMemZeroPageOffset(X);
		SET_N_Z(F, Y);
		break;
	case 0xAC:	// Absolute		
		Y = readMemAbsolute();
		SET_N_Z(F, Y);
		break;
	case 0xBC:	// Absolute,X
		Y = readMemAbsoluteOffset(X);
		SET_N_Z(F, Y);
		break;

		//
		// LSR
		//
	case 0x4A:			
		SETFLAGIFTRUE(F, FLAG_C, A & 1);
		A >>= 1;
		CLEARFLAG(F, FLAG_N);
		SETFLAGIFTRUE(F, FLAG_Z, A == 0);
		break;
	case 0x46:
		rightShift(getAddressZeroPage());
		break;
	case 0x56:
		rightShift(getAddressZeroPageOffset(X));
		break;
	case 0x4E:
		rightShift(getAddressAbsolute());
		break;
	case 0x5E:
		rightShift(getAddressAbsoluteOffset(X));
		break;

		//
		// PHA
		//
	case 0x48:
		push(A);
		break;

		//
		// PLA
		//
	case 0x68:
		A = pop();
		SET_N_Z(F, A);
		break;

		//
		// ROL
		//
	case 0x2A:
		rol(A);
		break;
	case 0x26:
		rolMem(getAddressZeroPage());
		break;
	case 0x36:
		rolMem(getAddressZeroPageOffset(X));
		break;
	case 0x2E:
		rolMem(getAddressAbsolute());
		break;
	case 0x3E:
		rolMem(getAddressAbsoluteOffset(X));
		break;

		//
		// RTI
		//
	case 0x40:
		{
			F = pop();
			WORD newP = pop();
			newP |= ((WORD)pop()) << 8;
			P = newP;
		}
		break;

		//
		// RTS
		//
	case 0x60:
		{
			WORD retAddr = pop();
			retAddr |= ((WORD)pop()) << 8;
			retAddr++;
			P = retAddr;
		}
		break;

		//
		// SEI
		//
	case 0x78:
		SETFLAG(F, FLAG_I);
		break;	

		//
		// STA
		//
	case 0x85: // Zero Page
		store(getAddressZeroPage(), A);
		break;
	case 0x95: // Zero Page, X
		store(getAddressZeroPageOffset(X), A);
		break;
	case 0x8D: // Absolute
		store(getAddressAbsolute(), A);
		break;
	case 0x9D: // Absolute, X
		store(getAddressAbsoluteOffset(X), A);
		break;
	case 0x99: // Absolute, Y
		store(getAddressAbsoluteOffset(Y), A);
		break;
	case 0x81: // Indirect, X
		store(getAddressPreIndexedIndirect(), A);
		break;
	case 0x91: // Indirect, Y
		store(getAddressPostIndexedIndirect(), A);
		break;

		//
		// STX
		//
	case 0x86: // Zero page
		store(getAddressZeroPage(), X);
		break;
	case 0x96: // Zero page Y offset
		store(getAddressZeroPageOffset(Y), X);
		break;
	case 0x8E: // Absolute
		store(getAddressAbsolute(), X);
		break;

		//
		// STX
		//
	case 0x84: // Zero page
		store(getAddressZeroPage(), Y);
		break;
	case 0x94: // Zero page Y offset
		store(getAddressZeroPageOffset(X), Y);
		break;
	case 0x8C: // Absolute
		store(getAddressAbsolute(), Y);
		break;

		//
		// TAX
		//
	case 0xAA:
		X = A;
		SET_N_Z(F, X);
		break;

		// 
		// TAY
		//
	case 0xA8:
		Y = A;
		SET_N_Z(F, Y);
		break;

		//
		// TXA
		//
	case 0x8A:
		A = X;
		SET_N_Z(F, A);
		break;

		//
		// TXS Transfer index X to stack pointer 
		//
	case 0x9A:
		S = X;
		break;

	default:
		char message[512];
		sprintf_s(message, 512, "Unregnized instruction 0x%x : $%x", opCode, P-1);
		printf(message);
		NOT_IMPLEMENTED;
		break;
	}
	
	int cycles = instrCycleCount[opCode] + extraCycles;
	if (cycles == 0) {
		// fuck off
		NOT_IMPLEMENTED;
	}

	cyclesLeftOnScanline -= cycles;
	if (cyclesLeftOnScanline < 0) {
		cyclesLeftOnScanline += NUM_CYCLES_PER_SCANLINE;
		++scanline;
		if (scanline < NUM_SCANLINES_SCREEN) {
			pEmulator->getPPU()->renderScanline(scanline - 1, pEmulator->getScreenPixelBuffer() + ((scanline-1) * 256));
		} else if (scanline == NUM_SCANLINES_SCREEN) {
			// Need to cause VBLANK
			doVblankInterrupt();
			pEmulator->flipScreen();
			pEmulator->getPPU()->setVblankFlag();
		} else if (scanline == NUM_SCANLINES_SCREEN + NUM_SCANLINES_VBLANK ) {
			scanline = 0;
			pEmulator->getPPU()->clearVblankFlag();
		}
	}
}

void CPU::doVblankInterrupt() {
	// Push the PC onto the stack
	push((BYTE)(P >> 8));
	push((BYTE)(P & 0xFF));
	
	// Push the flags register onto the stack.
	push(F);

	// Set the interrupt flag.
	F |= 0x04;

	// Set the PC equal to the address specified in the 
	// vector table for the NMI interrupt.
	P = (WORD)readMem(0xFFFA) | ((WORD)readMem(0xFFFB)) << 8;

	cyclesLeftOnScanline -= 7;
}

void CPU::incMem(WORD m) {
	BYTE b = readMem(m);
	++b;
	store(m, b);
	SET_N_Z(F, b);
}

void CPU::rol(BYTE& b) {
	BYTE carry = b & 0x80;
	b <<= 1;
	b |= (F & FLAG_C);
	SETFLAGIFTRUE(F, FLAG_C, carry);
	SET_N_Z(F, b);
}

void CPU::rolMem(WORD m) {
	BYTE b = readMem(m);
	rol(b);
	store(m, b);
}

void CPU::rightShift(WORD m) {
	BYTE b = readMem(m);
	SETFLAGIFTRUE(F, FLAG_C, b & 1);
	b >>= 1;
	CLEARFLAG(F, FLAG_N);
	SETFLAGIFTRUE(F, FLAG_Z, b == 0);
	store(m, b);
}

void CPU::push(BYTE b) {
	store(0x100 | S, b);
	--S;	
}

BYTE CPU::pop(void) {
	++S;	
	return  readMem(0x100 | (WORD)S);	
}

int CPU::branchIfNotFlag(BYTE flag) {
	int cycles = 2;			
	if (!TESTFLAG(F, flag)) {
		BYTE displacement = readMemImmediate();
		WORD newP;							
		if (displacement & 0x80) {			
			newP = P - (0x100 - displacement);	
		} else {								
			newP = P + displacement;			
		}										
		cycles += ((P & 0xFF00) != ((newP) & 0xFF00)) ? 2 : 1;
		P = newP;
	} else {	
		++P;	
	}

	return cycles;
}

int CPU::branchIfFlag(BYTE flag) {
	int cycles = 2;			
	if (TESTFLAG(F, flag)) {
		BYTE displacement = readMemImmediate();
		WORD newP;							
		if (displacement & 0x80) {			
			newP = P - (0x100 - displacement);	
		} else {								
			newP = P + displacement;			
		}										
		cycles += ((P & 0xFF00) != ((newP) & 0xFF00)) ? 2 : 1;
		P = newP;
	} else {	
		++P;	
	}

	return cycles;
}

BYTE CPU::readMemImmediate() {
	return readMem(P++);
}

WORD CPU::getAddressZeroPageOffset(BYTE offset) {
	return (WORD)readMem(P++)+(WORD)offset;
}

BYTE CPU::readMemZeroPageOffset(BYTE offset) {
	return readMem(getAddressZeroPageOffset(offset));
}

WORD CPU::getAddressZeroPage() {
	return readMem(P++);
}

BYTE CPU::readMemZeroPage() {
	return readMem(getAddressZeroPage());
}

WORD CPU::getAddressAbsolute() {
	BYTE low = readMem(P++);
	BYTE high = readMem(P++);
	WORD w = (((WORD)high) << 8) | ((WORD)low);
	return w;
}

BYTE CPU::readMemAbsolute() {
	WORD w = getAddressAbsolute();
	return readMem(w);
}

WORD CPU::getAddressAbsoluteOffset(BYTE offset) {
	BYTE low = readMem(P++);
	BYTE high = readMem(P++);
	WORD w = (((WORD)high) << 8) | ((WORD)low);
	w += offset;
	return w;
}

BYTE CPU::readMemAbsoluteOffset(BYTE offset) {
	WORD w = getAddressAbsoluteOffset(offset);
	return readMem(w);
}

WORD CPU::getAddressPreIndexedIndirect() {
	WORD w = readMem(0xFF & readMem(P++)+X);
	return w;
}

BYTE CPU::readMemPreIndexedIndirect() {
	WORD w = getAddressPreIndexedIndirect();
	return readMem(w);
}

WORD CPU::getAddressPostIndexedIndirect() {
	BYTE low = readMem(P++);
	BYTE high = readMem(P);
	WORD w = (((WORD)high) << 8) | ((WORD)low);
	w += Y;
	return w;
}

BYTE CPU::readMemPostIndexedIndirect() {
	WORD w = getAddressPostIndexedIndirect();
	return readMem(w);
}

BYTE CPU::addWithCarry(BYTE a, BYTE b) {
	WORD wa = (WORD)a;
	WORD wb = (WORD)b;
	wa += wb;
	wa += (WORD)F & 1;
	
	SET_N_Z_C(F, wa);

	// I find this suspicious
	SETFLAGIFTRUE(F, FLAG_V, 
		!((a ^ b) & 0x80) && ((a ^ wa) & 0x80));

	return (BYTE)(wa & 0xFF);
}

