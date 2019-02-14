#pragma once
#include <SDL/SDL.h>

struct SimplePlayer
{
	SDL_AudioSpec spec{};
	SDL_AudioDeviceID dev_id;
	unsigned int counter = 0, pitch = 262;
	unsigned int note_counter = 0;
	const char* notes;
	Sint8 octave = 0;//relative to middle C 'C4'
	Uint8 volume = 255;

	SimplePlayer(const char * notes)
	{
		this->notes = notes;
		spec.freq = 48000;
		spec.format = AUDIO_U8;
		spec.channels = 1;
		spec.samples = 1024;
		spec.userdata = this;
		spec.callback = [](void* param, Uint8* stream, int len)
		{
			((SimplePlayer*)param)->callback(stream, len);
		};
		dev_id = SDL_OpenAudioDevice(nullptr, 0, &spec, &spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
		SDL_PauseAudioDevice(dev_id, 0);
		next_note();
	}



	void callback(Uint8 * target, int num_samples)
	{
		for (int i = 0; i < num_samples; i++)
		{
			target[i] = ((counter / spec.freq) % 2 == 0) ? (Uint8)volume : 0;
			counter += pitch * 2;
		}
	}

	void next_note()
	{
		select_note(notes[note_counter++]);
	}

	void increase_octave()
	{
		octave++;
	}

	void decrease_octave()
	{
		octave--;
	}

	void change_volume(Uint8 volume)
	{
		this->volume = volume;
	}

	//major scale
	void select_note(char note)
	{
		switch (note)
		{
		case 'C':
			pitch = (octave < 0) ? 262 >> abs(octave) : 262 << abs(octave);
			break;
		case 'D':
			pitch = (octave < 0) ? 294 >> abs(octave) : 294 << abs(octave);
			break;
		case 'E':
			pitch = (octave < 0) ? 330 >> abs(octave) : 330 << abs(octave);
			break;
		case 'F':
			pitch = (octave < 0) ? 349 >> abs(octave) : 349 << abs(octave);
			break;
		case 'G':
			pitch = (octave < 0) ? 392 >> abs(octave) : 392 << abs(octave);
			break;
		case 'A':
			pitch = (octave < 0) ? 440 >> abs(octave) : 440 << abs(octave);
			break;
		case 'B':
			pitch = (octave < 0) ? 494 >> abs(octave) : 494 << abs(octave);
			break;
		case 'c':
			pitch = (octave < 0) ? 523 >> abs(octave) : 523 << abs(octave);
			break;
		case 'R':
			pitch = 0;
			break;
		case '>':
			increase_octave();
			next_note();
			break;
		case '<':
			decrease_octave();
			next_note();
			break;
		default:
			note_counter = 0;
			next_note();
			break;
		}
	}
};