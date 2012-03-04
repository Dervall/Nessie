#pragma once

#include "Types.h"

class CPU;
class PPU;
class CPUMem;

class Emulator {
public:
			Emulator		(void);
			~Emulator		(void);
	
	void	loadFromFile	(const char* fileName);
	void	run				(void);
	void	reset			(void);
	
	PPU*			getPPU					(void) { return pPpu; }
	unsigned int*	getScreenPixelBuffer	(void);
	void			flipScreen				(void);

private:
	CPU*	pCpu;
	PPU*	pPpu;
	CPUMem*	pCpuMem;	

	BYTE*	pCartridge;
};