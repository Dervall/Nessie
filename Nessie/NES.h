#pragma once

/*	This is a a write only register, reading it will
	return open bus. Here is the list of what each
	bit in the write do.  

	76543210
	||||||||
	||||||++- Base nametable address
	||||||    (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
	|||||+--- VRAM address increment per CPU read/write of PPUDATA
	|||||     (0: increment by 1, going across; 1: increment by 32, going down)
	||||+---- Sprite pattern table address for 8x8 sprites (0: $0000; 1: $1000)
	|||+----- Background pattern table address (0: $0000; 1: $1000)
	||+------ Sprite size (0: 8x8; 1: 8x16)
	|+------- PPU master/slave select (has no effect on the NES)
	+-------- Generate an NMI at the start of the
			  vertical blanking interval (0: off; 1: on) */
#define PPUCTRL (0x2000)

/*	This is a write only regiser, reading it will return open bus. This 
	register controls some operations of the PPU when it writes to the screen.

	76543210
	||||||||
	|||||||+- Grayscale (0: normal color; 1: AND all palette entries
	|||||||   with 0x30, effectively producing a monochrome display;
	|||||||   note that colour emphasis STILL works when this is on!)
	||||||+-- Enable background in leftmost 8 pixels of screen (0: no; 1:yes)
	|||||+--- Enable sprite in leftmost 8 pixels of screen (0: no; 1: yes)
	||||+---- Enable background rendering
	|||+----- Enable sprite rendering
	||+------ Intensify reds (and darken other colors)
	|+------- Intensify greens (and darken other colors)
	+-------- Intensify blues (and darken other colors) */
#define PPUMASK (0x2001)

/*	This register returns the status of the PPU. This is read-only.
	Writing to it does nothing.

	76543210 
	||||||||
	|||+++++- Unimplemented (read back as open bus data)
	||+------ Sprite overflow. The PPU can handle only eight sprites on one
	||        scanline and sets this bit if it starts dropping sprites.
	||        Normally, this triggers when there are 9 sprites on a scanline,
	||        but the actual behavior is significantly more complicated.
	|+------- Sprite 0 Hit.  Set when a nonzero pixel of sprite 0 'hits'
	|         a nonzero background pixel.  Used for raster timing.
	+-------- Vertical blank has started (0: not in VBLANK; 1: in VBLANK) 
	
	Reading here in vblank at a certain time can turn off NMI for the frame,
	that will be explained later.
	*/
#define PPUSTATUS (0x2002)

/* 	The address of where OAMDATA will use, it is also the starting address
	that SPRDMA will use to to write data to the sprite address. it is write 
	only, reading will return open bus. Writing here during the PPU rendering 
	might mess up the sprite rendering, though the exact behavior isn't 
	figured out yet. */
#define OAMADDR (0x2003)


/*	This the the register used to communicate with the sprite data, it 
	returns whatever is in the OAMADDR is set to when the PPU is not rendering
	graphics, if it is, then it returns what the PPU is reading from the 
	sprite data to render onto the screen. The OAMADDR is auto incremented 
	on writes to this. Attribute regs bit 5 to 8 are unimplemented,
	read back 0 here, so data &= 0xE3. Reading here increments the OAMADDR, 
	when it is over 0xFF mark, it wraps back to 0. Writing here does *not*
	increment the OAMADDR however. */
#define OAMDATA (0x2004)
	

/*	This affects where the PPU will read from the name table. 
	It modifies the horizontal and vertical offset of the name table read, a 
	detailed explanation of it will be given in a later section. */
#define PPUSCROLL (0x2005)
	
/*	This is the register where you write the address that you want to 
	read from or write to using PPUDATA (0x2007). Games must not ever 
	write to this address during rendering, or it will mess up the display, 
	this will be explained in a later section. Reading here will return
	open bus. During rendering the address written here will be the 
	"program counter" for the PPU, so this is why it messes up, because
	the PPU also increments that register as its rendering leading to 
	addresses you don't want. Only use this during vblank to update
	graphic data. */
#define PPUADDR (0x2006)
	

/*	This address is used to write or read data from the specified address 
	in PPUADDR (0x2006). Do not read from or write to this address during 
	rendering, it will mess up the display. If you do read from it it returns 
	the internal operation of the PPU, and if you write to it, it writes to 
	address the PPU currently is using to get data to draw to the screen.
	The reason it messes it up is because it increments the "program counter" 
	of the PPU which causes distortions to the rendering. I believe the 
	rationale for incrementing the program counter automatically on a read or 
	write is to help the programmer save precious VBLANK time from 
	manually incrementing it themselves. Since the only time you can write 
	to the graphics or read from it is VBLANK or when the PPU is turned "off".
	Reads are delayed by one cycle, discard the first byte read, in example, 
	here is how one reads 0x2007:
	tmp = cpu.reg2007; cpu.reg2007 = data; return tmp;
	However, if the address is in the palette range, that is (0x3F00 to 0x3FFF)
	accesses the address, without the delayed read. On writes to the 0x2007 
	register, if the game uses CHR-ROM, writing to memory address [0, 0x1FFF] 
	should do nothing, while it works normally for CHR-RAM.
	PPUADDR is incremented with the value set in PPUCTRL after every read or 
	write to 0x2007. There is one last quirk, and that is if the register 
	0x2006 address is in the palette range, that is if the range is 
	((ppu.reg[6] & 0x3F00) == 0x3F00) what is returned is the palette data, 
	but what is copied to the delayed buffer is addr[ppu.reg[6] - 0x1000]; */
#define PPUDATA (0x2007)
	

/*	You write the starting address of where you want to write the data to 
	the SPR data. The address written is shifted by 8 (xx00) since it copies 
	256 bytes starting with 0 to fill up the sprite memory space.
	This operations takes 513 cycles, 1 cycle for read, 
	1 cycle for write, (256 * 2) = 512, and 1 last read cycle. */
#define SPRDMA (0x4014)
	

#define NUM_CYCLES_PER_SCANLINE 113
#define CPU_FREQUENCY 1789772
#define NUM_SCANLINES_SCREEN    240
#define NUM_SCANLINES_VBLANK    22