#include "pch.h"

struct SimplePlayer
{
	SDL_AudioSpec spec {};
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

/*
pulse 1

P = sweeping period 3-bit unsigned
S = sweep rate 4-bit signed
D = duty cycle 2-bit, 4 choices (12.5%, 25%, 50%, 75%)
L = length 6-bit unsigned
V = volume 4-bit unsigned
v = volume sweep 4-bit signed
F = frequency 11-bit unsigned
T = trigger switch
L = length feature switch

$FF10 _PPPSSSS
$FF11 DDLLLLLL
$FF12 VVVVvvvv
$FF13 FFFFFFFF
$FF14 TL___FFF
*/
struct GBAudioPulseA
{
	Uint8 frequency_sweep_period; //0b111 u
	Uint8 frequency_sweep_rate; //0b1111 s
	Uint8 duty_cycle; //0b11 u
	Uint8 length; //0b111111 u
	Uint8 volume; //0b1111 u
	Uint8 volume_sweep; //0b1111 s
	Uint16 frequency; //0b11111111111 u
	bool trigger;
	bool length_switch;
};

/*
pulse 2

D = duty cycle 2-bit, 4 choices (12.5%, 25%, 50%, 75%)
L = length 6-bit unsigned
V = volume 4-bit unsigned
v = volume sweep 4-bit signed
F = frequency 11-bit unsigned
T = trigger switch
L = length feature switch

$FF16 DDLLLLLL
$FF17 VVVVvvvv
$FF18 FFFFFFFF
$FF19 TL___FFF
*/
struct GBAudioPulseB
{
	Uint8 duty_cycle; //0b11 u
	Uint8 length; //0b111111 u
	Uint8 volume; //0b1111 u
	Uint8 volume_sweep; //0b1111 s
	Uint16 frequency; //0b11111111111 u
	bool trigger;
	bool length_switch;
};

/*
wave

L = length 8-bit unsigned
V = volume 2-bit unsigned
F = frequency 11-bit unsigned
T = trigger switch
L = length feature switch

$FF1B LLLLLLLL
$FF1C _VV_____
$FF1D FFFFFFFF
$FF1E TL___FFF
*/
struct GBAudioWave
{
	Uint8 length; //0b11111111 u
	Uint8 volume; //0b11 u
	Uint16 frequency; //0b11111111111 u
	bool trigger;
	bool length_switch;
};

/*
noise, gets values from an LFSR of either 7 or 15 bits, polls F(S, H) times per second
F(S, H) = 524288/(H * 2^S+1)
F(S, 0) = 524288/2^S

L = length 6-bit unsigned
V = volume 4-bit unsigned
v = volume sweep 4-bit signed
S = part one of the frequency function 4-bit unsigned
W = LFSR length switch (7-bit, 15-bit)
H = part two of the frequency function 3-bit unsigned
T = trigger switch
L = length feature switch

$FF20 __LLLLLL
$FF21 VVVVvvvv
$FF22 SSSSWHHH
$FF23 TL______

LFSR function(7-bit):
7-bit int A - should be the last output of this function, or some seed value
7-bit int B
7-bit int C

B = A AND 1
A = A >> 1
C = A AND 1
C = B XOR C
C = C << 6
B = A AND 0b0111111
A = B OR C

from here, return A

LFSR function(15-bit):
8-bit int A - the low bits of the LFSR seed
7-bit int B - the high bits of the LFSR seed
7-bit int C
7-bit int D

C = A AND 1
A = A >> 1
D = A AND 1
D = C XOR D
D = D << 6
C = A AND 0b10111111
A = C OR D

C = B AND 1
C = C << 7
A = A OR C
B = B >> 1
C = B AND 0b0111111
B = C OR D

from here, return A and B
*/
struct GBAudioNoise
{
	Uint8 length; //0b111111
	Uint8 volume; //0b1111
	Uint8 volume_sweep; //0b1111
	Uint8 S; //0b1111
	bool width;
	Uint8 H; //0b111
	bool trigger;
	bool length_switch;
};

Uint8 LFSR(Uint8 seed)
{
	Uint8 A = seed;
	Uint8 B = A & 0b1;
	A = A >> 1;
	Uint8 C = A & 0b1;
	C = B ^ C;
	C = C << 7;
	A = A | C;
	return A;
}

struct GBAudio
{
	SDL_AudioSpec spec{};
	SDL_AudioDeviceID dev_id;

	GBAudioPulseA pulse1;
	GBAudioPulseB pulse2;
	GBAudioWave wave;
	GBAudioNoise noise;

	GBAudio()
	{
		spec.freq = 48000;
		spec.format = AUDIO_U8;
		spec.channels = 1;
		spec.samples = 1024;
		spec.userdata = this;
		spec.callback = [](void* param, Uint8* stream, int len)
		{
			((GBAudio*)param)->callback(stream, len);
		};
		dev_id = SDL_OpenAudioDevice(nullptr, 0, &spec, &spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
		SDL_PauseAudioDevice(dev_id, 0);
	}

	void callback(Uint8 * target, int num_samples)
	{

	}
};

int main(int argc, char ** argv)
{
	SDL_InitSubSystem(SDL_INIT_AUDIO);
	
	for (;;)
	{
		SDL_Delay(500);
	}

	SDL_Quit();
	return 0;
}

/*
Notes:

GB Audio:
4 channels
2 pulse square freq(0...2047) vol(0...15) duty(1, .75, .5, .25)(0...3) fade(-8...7) (channel 1 has freq sweep) sweep_period(0...7) sweep_rate(-8...7) len(0...63) auto_terminate(on, off) trigger(on, off)
1 wave (plays sound defined by PCM data in a LUT at $FF30 - $FF3F) vol(0...3) freq(0...2047) len(0...255) auto_terminate(on, off) trigger(on, off)
1 noise polling_freq(0...524288) F(S, H) vol(0...15) fade(-8...7) LFSR_width(0..1) len(0...63) auto_terminate(on, off) trigger(on, off)

sample PCM data:
EC BA 98 89 BD F2 45 66 66 77 77 77 66 65 54 31 (32 4-bit values in bytes (16 8-bit values))

specific memory layout:

pulse 1

P = sweeping period 3-bit unsigned
S = sweep rate 4-bit signed
D = duty cycle 2-bit, 4 choices (12.5%, 25%, 50%, 75%)
L = length 6-bit unsigned
V = volume 4-bit unsigned
v = volume sweep 4-bit signed
F = frequency 11-bit unsigned
T = trigger switch
L = length feature switch

$FF10 _PPPSSSS
$FF11 DDLLLLLL
$FF12 VVVVvvvv
$FF13 FFFFFFFF
$FF14 TL___FFF

pulse 2

D = duty cycle 2-bit, 4 choices (12.5%, 25%, 50%, 75%)
L = length 6-bit unsigned
V = volume 4-bit unsigned
v = volume sweep 4-bit signed
F = frequency 11-bit unsigned
T = trigger switch
L = length feature switch

$FF16 DDLLLLLL
$FF17 VVVVvvvv
$FF18 FFFFFFFF
$FF19 TL___FFF

wave

L = length 8-bit unsigned
V = volume 2-bit unsigned
F = frequency 11-bit unsigned
T = trigger switch
L = length feature switch

$FF1B LLLLLLLL
$FF1C _VV_____
$FF1D FFFFFFFF
$FF1E TL___FFF

noise, gets values from an LFSR of either 7 or 15 bits, polls F(S, H) times per second
F(S, H) = 524288/(H * 2^S+1)
F(S, 0) = 524288/2^S

L = length 6-bit unsigned
V = volume 4-bit unsigned
v = volume sweep 4-bit signed
S = part one of the frequency function 4-bit unsigned
W = LFSR length switch (7-bit, 15-bit)
H = part two of the frequency function 3-bit unsigned 
T = trigger switch
L = length feature switch

$FF20 __LLLLLL
$FF21 VVVVvvvv
$FF22 SSSSWHHH
$FF23 TL______

LFSR function(7-bit):
7-bit int A - should be the last output of this function, or some seed value
7-bit int B
7-bit int C

B = A AND 1
A = A >> 1
C = A AND 1
C = B XOR C
C = C << 6
B = A AND 0b0111111
A = B OR C

from here, return A

LFSR function(15-bit):
8-bit int A - the low bits of the LFSR seed
7-bit int B - the high bits of the LFSR seed
7-bit int C
7-bit int D

C = A AND 1
A = A >> 1
D = A AND 1
D = C XOR D
D = D << 6
C = A AND 0b10111111
A = C OR D

C = B AND 1
C = C << 7
A = A OR C
B = B >> 1
C = B AND 0b0111111
B = C OR D

from here, return A and B
*/