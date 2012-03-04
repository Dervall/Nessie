#include "SDL.h" 
#include "Emulator.h"
#include "Types.h"

const int SCREEN_WIDTH = 256;
const int SCREEN_HEIGHT = 240;
const int SCREEN_BPP = 32;


SDL_Surface* screen;

int main( int argc, char* args[] ) 
{ 
	screen = NULL;

	//Start SDL 
	SDL_Init( SDL_INIT_EVERYTHING ); 
	
	// Setup the screen
	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE);
	
	Emulator emu;
	emu.loadFromFile("superkuken");

	while(true) {
		emu.run();


//			SDL_Flip(screen);

		SDL_Delay(0);
	}
	

	//Quit SDL 
	SDL_Quit(); 
	return 0; 
} 

