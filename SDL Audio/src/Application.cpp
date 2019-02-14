#include "pch.h"
#include "GBAudio.cpp"


int main(int argc, char ** argv)
{
	SDL_InitSubSystem(SDL_INIT_AUDIO);

	GBAudio audio;
	audio.pulse1.frequency = 440;
	audio.pulse1.volume = 127;
	audio.pulse1.duty_cycle = 0;
	
	for (;;)
	{
		SDL_Delay(500);
		audio.pulse1.duty_cycle = (audio.pulse1.duty_cycle+1) % 4;
	}

	SDL_Quit();
	return 0;
}

