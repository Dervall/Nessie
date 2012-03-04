#include "Emulator.h"
#include <stdio.h>
#include <memory.h>
#include <windows.h>
#include "CPU.h"
#include "PPU.h"
#include "SDL.h"
#include "Emulator.h"

#define PRGROM_BANKSIZE = (1024 * 16)

Emulator::Emulator( void ) {
	pCpuMem = new CPUMem();
	pCartridge = NULL;
	pCpu = new CPU(pCpuMem, this);
	pPpu = new PPU(pCpu, this);
	pCpuMem->setPPU(pPpu);
}

Emulator::~Emulator( void ) {
	if (pCpu) {
		delete pCpu;
		pCpu = NULL;
	}

	delete pCpuMem;
	delete pPpu;	
}

void Emulator::run(void) {
	pCpu->run();	
}

// Fix later
extern SDL_Surface* screen;

unsigned int* Emulator::getScreenPixelBuffer() {
	return (unsigned int*)screen->pixels;
}

void Emulator::flipScreen() {
	SDL_Flip(screen);
}


void Emulator::loadFromFile(const char* pFileName) {
	// Screw the filename
	FILE* pFile;
	fopen_s(&pFile, "E:\\nestest.nes", "r");
	if (pFile) {
		fseek(pFile, 0, SEEK_END);
		long lFileSize = ftell(pFile);
		fseek(pFile, 0, SEEK_SET);
		pCartridge = new unsigned char[lFileSize];
		fread(pCartridge, 1, lFileSize, pFile);
		fclose(pFile);

		// File should be loaded. See what the fuck it contains;
		struct NESHEADER {
			char magic[4];
			unsigned char num16kbROMbanks;
			unsigned char num8kbVROMbanks;
			
			unsigned char mirroring : 1;
			unsigned char batteryBacked : 1;
			unsigned char trainer : 1;
			unsigned char fourScreenVRAMlayout : 1;
			unsigned char ROMMapperTypeLower : 4;
			
			unsigned char VS_system : 1;
			unsigned char reserved_1 : 3;
			unsigned char ROMMapperTypeHigher : 4;

			unsigned char num8kRAMbanks;

			unsigned char PAL : 1;
			unsigned char reserved_2 : 7;
			
			unsigned char reserved_3[6];
		};

		NESHEADER* pHeader = (NESHEADER*)pCartridge;
		
		// No mapper right now, I'll just put the program rom pointer to the first
		// program bank
		pCpuMem->setPrgRomBank1(pCartridge + sizeof(NESHEADER));
		pCpuMem->setPrgRomBank2(pCartridge + sizeof(NESHEADER));
		
		// Initialize the pattern tables to point to the first and
		// second banks of CHR-ROM.
		pPpu->setPatternTable1(pCartridge + sizeof(NESHEADER) + 0x4000 * pHeader->num16kbROMbanks);
		pPpu->setPatternTable2(pCartridge + sizeof(NESHEADER) + 0x4000 * pHeader->num16kbROMbanks + 0x1000);

		pPpu->setupNameTables(pHeader->mirroring);

		// Reset the NES
		reset();
	}
}

void Emulator::reset() {
	pCpu->reset();	
	pCpuMem->reset();
	pPpu->reset();
}

