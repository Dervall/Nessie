#pragma once

#include "Types.h"
#include "CPUMem.h"

class Emulator;

class CPU {
public:
			CPU		( CPUMem* p, Emulator* pEmu );
			~CPU	( void );

	void	reset	( void );
	void	run		( void );	

	void	nmi		( void );
	

private:	
	void	doVblankInterrupt	();

	WORD	getAddressZeroPage();
	WORD	getAddressZeroPageOffset(BYTE offset);
	WORD	getAddressAbsolute();
	WORD	getAddressAbsoluteOffset(BYTE offset);
	WORD	getAddressPreIndexedIndirect();
	WORD	getAddressPostIndexedIndirect();

	BYTE	readMemImmediate			(void);
	BYTE	readMemZeroPage				(void);
	BYTE	readMemZeroPageOffset		(BYTE offset);
	BYTE	readMemAbsolute				(void);
	BYTE	readMemAbsoluteOffset		(BYTE offset);
	BYTE	readMemPreIndexedIndirect	(void);
	BYTE	readMemPostIndexedIndirect	(void);

	int		branchIfNotFlag				(BYTE flag);
	int		branchIfFlag				(BYTE flag);
	void	incMem						(WORD m);
	void	rightShift					(WORD m);
	void	rol							(BYTE& b);
	void	rolMem						(WORD m);
	BYTE	addWithCarry				(BYTE a, BYTE b);

	void	push						(BYTE b);
	BYTE	pop							(void);

	// Pointer to the memory class which also handles the system bus
	CPUMem*		pMemory;
	Emulator*	pEmulator;

	// Convencience
	inline void	store		(WORD address, BYTE value)	{ return pMemory->write(address, value); }
	inline BYTE	readMem		(WORD address)				{ return pMemory->read(address); }

	// Registers!
	WORD	P;	// Program counter
	BYTE	A;	// Accumulator
	BYTE	X;	// index X
	BYTE	Y;	// index Y
	BYTE	F;	// Flags
	BYTE	S;	// Stack pointer

	int cyclesLeftOnScanline;
	int scanline;
};