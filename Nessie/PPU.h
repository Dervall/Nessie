#pragma once

#include "Types.h"

class CPU;
class Emulator;

class PPU {
public:
			PPU			( CPU* p, Emulator* pEmu );
			~PPU		( void );

	void	reset		( void );

	BYTE	readStatus		( void );
	BYTE	readOAMAddr		( void );	// Not supported in hardware but present for DMA reasons
	BYTE	readOAMData		( void );
	BYTE	readPPUData		( void );

	void	writeCtrlReg	( BYTE value );
	void	writeMask		( BYTE value );
	void	writeOAMAddr	( BYTE value );
	void	writeOAMData	( BYTE value );
	void	writeScroll		( BYTE value );
	void	writePPUAddr	( BYTE value );
	void	writePPUData	( BYTE value );

	// Direct accessors into video memory, that is not used by 'real' NES instructions
	// This is open for DMA access
	void	writeOAMMem		( BYTE address, BYTE data );
	
	void	setupNameTables		( int mirror );
	void	setPatternTable1	( BYTE* p );
	void	setPatternTable2	( BYTE* p );

	void	renderScanline		(int scanline, unsigned int* pOut);

	void	setVblankFlag		(void);
	void	clearVblankFlag		(void);

private:
	BYTE	readPPUMem		( WORD address );
	void	writePPUMem		( WORD address, BYTE data );

	BYTE	readOAMMem		( BYTE address );

	BYTE*	getVramPtr		( WORD address );

	BYTE	regWriteToggle;

	// The bus-accessible registers
	BYTE	reg[8];

	BYTE	intX;	// Scroll X value i think

	// The result of the writes to PPUAddr
	WORD	ppuAddr;
	WORD	intReg;	// Intermediate register

	// OAM data
	BYTE	oamData			[ 0xFF ];		// 256 bytes of spritie goodness
	BYTE	spriteBuffer	[ 0x20 ];

	// PPU data
	BYTE*	pPatternTable1;
	BYTE*	pPatternTable2;
	BYTE*	apNameTable		[ 4 ];

	BYTE	palette			[ 0xFF ];

	// "real" name tables
	BYTE	aNameTableMem	[ 0x800 ];

	// Pointer to the CPU
	// Needed for NMI
	CPU*		pCpu;
	Emulator*	pEmulator;
};