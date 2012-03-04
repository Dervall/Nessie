#pragma once
#include "Types.h"

class PPU;

class CPUMem {
public:
			CPUMem	( void );
			~CPUMem	( void );

	void	reset	( void );

	BYTE	read	( WORD wAddress );
	void	write	( WORD wAddress, BYTE value );

	void	setPrgRomBank1	( BYTE* p );
	void	setPrgRomBank2	( BYTE* p );
	void	setPPU			( PPU* p )	{ pPpu = p; }

	WORD	getInitialProgramCounter	( void );

private:
	BYTE	ppuRegRead	( WORD wAddress );
	void	ppuRegWrite	( WORD address, BYTE value );
	BYTE	openBus		( void );

	
	
	// Main RAM of the NES
	BYTE	memory			[ 0x2000 ];	
	
	// Two ROM banks for program memory.
	BYTE*	pPrgRomBank1;	
	BYTE*	pPrgRomBank2;	

	PPU*	pPpu;
};