#include "CPUMem.h"
#include "NES.h"
#include <memory.h>
#include "PPU.h"

CPUMem::CPUMem() {
	
}

CPUMem::~CPUMem() {
}

void CPUMem::reset() {
	memset(memory, 0, 0x2000); 
}

BYTE CPUMem::openBus() {
	// TODO: Implement me properly
NOT_IMPLEMENTED;
	return 0;
}

BYTE CPUMem::ppuRegRead(WORD wAddress) {
	switch (wAddress) {
	case PPUCTRL:
		return openBus();	// The PPU control register is write only
	case PPUMASK:
		return openBus();
	case PPUSTATUS:
		return pPpu->readStatus();
	case OAMADDR:
		return openBus();
	case OAMDATA:
		return pPpu->readOAMData();
	case PPUSCROLL:
		NOT_IMPLEMENTED;	// I think this is open bus?!
		break;
	case PPUDATA:
		return pPpu->readPPUData();
	}

	NOT_IMPLEMENTED;
	return 0;
}

BYTE CPUMem::read(WORD wAddress)
{
	if (wAddress < 0x2000) {
		return memory[wAddress];
	} else if (wAddress >= 0x8000 && wAddress < 0xC000) {
		return pPrgRomBank1[wAddress-0x8000];
	} else if (wAddress >= 0xC000 && wAddress <= 0xFFFF) {
		return pPrgRomBank2[wAddress-0xC000];
	} else {
		if (wAddress >= 0x2000 && wAddress <= 0x3FFF) {
			// Remove the mirroring and use the base addresses
			return ppuRegRead(0x2000 | (wAddress&7));
		} else if (wAddress == SPRDMA) {
			// TODO: Unknown results reading from SPRDMA..
			return openBus();
		} else if (wAddress == 0x4015)
		{
			NOT_IMPLEMENTED;
			return 0;
		}
		else if (wAddress == 0x4016)
		{
			return 0;
		}
	}

	// Bogus read
	NOT_IMPLEMENTED;
	return 0;
} 

void CPUMem::ppuRegWrite(WORD address, BYTE value) {
	switch (address) {
	case PPUCTRL:
		pPpu->writeCtrlReg(value);
		break;
	case PPUMASK:
		pPpu->writeMask(value);
		break;
	case PPUSTATUS:
		break;	// Read only does nothing
	case OAMADDR:
		pPpu->writeOAMAddr(value);
		break;
	case OAMDATA:
		pPpu->writeOAMData(value);
		break;
	case PPUSCROLL:
		pPpu->writeScroll(value);
		break;
	case PPUADDR:
		pPpu->writePPUAddr(value);
		break;
	case PPUDATA:
		pPpu->writePPUData(value);
		break;
	}	
}

void CPUMem::write(WORD address, BYTE value) {
	// Check for all the register writes.
	if (address < 0x2000) {
		// Normal RAM write
		memory[address] = value;
	} else if (address >= 0x2000 && address <= 0x3FFF) {
		// Remove the mirroring and use the base addresses
		ppuRegWrite(0x2000 | (address&7), value);
	}
	// All the APU registers.
	else if (address == 0x4003 || address == 0x4015)
	{
		// Do nothing
		//NOT_IMPLEMENTED;
		//memory[address] = value;
//			APU_Write(wAddress, byData);
	}
	else if (address == 0x4014)
	{
		WORD dmaStart = value;
		dmaStart <<= 8;

		WORD oamAddr = pPpu->readOAMAddr();
		for (WORD i = 0; i != 256; ++i) {
			BYTE data = read(dmaStart | i);	
			pPpu->writeOAMMem((BYTE)(oamAddr + i & 0xFF), data);	// This takes a cycle
		}
	
		// Here something takes an extra cycle
		read(0);
	}
	else if (address == 0x4016)
	{			
		//write to joystick here
	//	memory[0x4016] = value;
	}	
}

void CPUMem::setPrgRomBank1(BYTE* p) {
	pPrgRomBank1 = p;
}

void CPUMem::setPrgRomBank2(BYTE* p) {
	pPrgRomBank2 = p;
}


WORD CPUMem::getInitialProgramCounter( void ) {
	// This wont work in the long run with mappers and shit
	return (WORD)pPrgRomBank2[0xFFFC-0xC000] | ((WORD)pPrgRomBank2[0xFFFD-0xC000]) << 8;
}
