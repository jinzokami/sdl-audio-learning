#include "pch.h"

#include <stdio.h>

#include "SDL/SDL.h"

struct Audio
{
	SDL_AudioSpec spec;
	SDL_AudioDeviceID dev_id;
	Uint32 buffer_length;
	Uint32 buffer_pos = 0;
	Uint8 *buffer;

	Audio()
	{	
		spec.freq = 96000;
		spec.format = AUDIO_F32SYS;
		spec.channels = 1;
		spec.samples = 4096;
		spec.userdata = this;
		spec.callback = [](void* param, Uint8* stream, int len)
		{
			((Audio*)param)->callback((Sint16*)stream, len / sizeof(float));
		};
		dev_id = SDL_OpenAudioDevice(nullptr, 0, &spec, &spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
		SDL_PauseAudioDevice(dev_id, 0);
	}

	Audio(const char * path)
	{
		if (SDL_LoadWAV(path, &spec, &buffer, &buffer_length) == NULL)
		{
			fprintf(stderr, "Could not open wav: %s\n", SDL_GetError());
		}

		printf("Opening File: %s\nFrequency: %d\nFormat: %d\nChannels: %d\nSamples: %d", path, spec.freq, spec.format, spec.channels, spec.samples);
		spec.userdata = this;
		spec.callback = [](void* param, Uint8* stream, int len)
		{
			((Audio*)param)->callback((Sint16*)stream, len / sizeof(Sint16));
		};
		dev_id = SDL_OpenAudioDevice(nullptr, 0, &spec, &spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
		SDL_PauseAudioDevice(dev_id, 0);
	}

	void callback(Sint16* target, int num_samples)
	{
		for (int position = 0; position < num_samples; ++position)
		{
			Sint16 sample = (buffer[buffer_pos] << 8) | buffer[buffer_pos + 1];
			target[position] = sample;
			buffer_pos += 2;
		}
	}
};

int main(int argc, char ** argv)
{
	SDL_InitSubSystem(SDL_INIT_AUDIO);
	Audio beeper("res/alarm.wav");

	for (;;)
	{
		SDL_Delay(500);
	}

	SDL_Quit();
	return 0;
}
