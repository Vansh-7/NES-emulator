#pragma once

#include <cstdint>
#include <functional>

class apu2A03 {
public:
	apu2A03();
	~apu2A03();

public:
	void cpuWrite(uint16_t addr, uint8_t data);
	uint8_t cpuRead(uint16_t addr);
	void clock();
	void reset();

	double GetOutputSample();

private:
	uint32_t frame_clock_counter = 0;
	uint32_t clock_counter = 0;
	bool bUseRawMode = false;

private:
	static uint8_t length_table[];

private:
	
	// Sequencer Module
	// ~~~~~~~~~~~~~~~~
	// The purpose of the sequencer is to output a '1' after a given 
	// interval. It does this by counting down from a start value,
	// when that value is < 0, it gets reset, and an internal "rotary"
	// buffer is shifted. The nature of ths shifted pattern is different
	// depending upon the channel, or module that requires sequencing.
	// For example, the square wave channels simply rotate the preset
	// sequence, but the noise channel needs to generate pseudo-random
	// outputs originating from the preset sequence.
	//
	// Consider a square wave channel. A preset sequence of 01010101
	// will output a 1 more freqently than 00010001, assuming we 
	// always output the LSB. The speed of this output is also
	// governed by the timer counting down. The frequency is higher
	// for small timer values, and lower for larger. Increasing
	// the frequency of the output potentially increases the
	// audible frequency. In fact, this is how the pulse channels
	// fundamentally work. A "duty cycle" shape is loaded into the
	// sequencer and the timer is used to vary the pitch, yielding 
	// notes.

	struct sequencer
	{
		uint32_t sequence = 0x00000000;
		uint32_t new_sequence = 0x00000000;
		uint16_t timer = 0x0000;
		uint16_t reload = 0x0000;
		uint8_t output = 0x00;

		// Pass in a lambda function to manipulate the sequence as required
		// by the owner of this sequencer module
		template<typename F>
		uint8_t clock(bool bEnable, F funcManip)
		{
			if (bEnable)
			{
				timer--;
				if (timer == 0xFFFF)
				{
					timer = reload;
					funcManip(sequence);
					output = sequence & 0x00000001;
				}
			}
			return output;
		}
	};

	struct lengthcounter
	{
		uint8_t counter = 0x00;
		uint8_t clock(bool bEnable, bool bHalt)
		{
			if (!bEnable)
				counter = 0;
			else
				if (counter > 0 && !bHalt)
					counter--;
			return counter;
		}
	};

	struct envelope
	{
		void clock(bool bLoop)
		{
			if (!start)
			{
				if (divider_count == 0)
				{
					divider_count = volume;

					if (decay_count == 0)
					{
						if (bLoop)
						{
							decay_count = 15;
						}

					}
					else
						decay_count--;
				}
				else
					divider_count--;
			}
			else
			{
				start = false;
				decay_count = 15;
				divider_count = volume;
			}

			if (disable)
			{
				output = volume;
			}
			else
			{
				output = decay_count;
			}
		}

		bool start = false;
		bool disable = false;
		uint16_t divider_count = 0;
		uint16_t volume = 0;
		uint16_t output = 0;
		uint16_t decay_count = 0;
	};


	struct oscpulse
	{
		double frequency = 0;
		double dutycycle = 0;
		double amplitude = 1;
		double pi = 3.14159;
		double harmonics = 20;

		double sample(double t)
		{
			double a = 0;
			double b = 0;
			double p = dutycycle * 2.0 * pi;

			auto approxsin = [](double t)
			{
				double j = t * 0.15915;
				j = j - (int)j;
				return 20.785 * j * (j - 0.5) * (j - 1.0);
			};

			for (double n = 1; n < harmonics; n++)
			{
				double c = n * frequency * 2.0 * pi * t;
				a += -approxsin(c) / n;
				b += -approxsin(c - p * n) / n;
			}

			return (2.0 * amplitude / pi) * (a - b);
		}
	};

	struct sweeper
	{
		bool enabled = false;
		bool down = false;
		bool reload = false;
		uint8_t shift = 0x00;
		uint8_t timer = 0x00;
		uint8_t period = 0x00;
		uint16_t change = 0;
		bool mute = false;

		void track(uint16_t &target)
		{
			change = target >> shift;
			mute = (target < 8) || (target > 0x7FF);

		}

		bool clock(uint16_t &target, bool channel)
		{
			bool changed = false;
			if (timer == 0 && enabled && shift > 0 && !mute)
			{
				if (target >= 8 && change < 0x07FF)
				{
					if (down)
					{
						target -= change - channel;
					}
					else
					{
						target += change;
					}
					changed = true;
				}
			}

			
			if (timer == 0 || reload)
			{
				timer = period;
				reload = false;
			}
			else
				timer--;

			mute = (target < 8) || (target > 0x7FF);

			return changed;
		}
	};

	double dGlobalTime = 0.0;

	// Square Wave Pulse Channel 1 (melody)
	bool pulse1_enable = false;
	bool pulse1_halt = false;
	double pulse1_sample = 0.0;
	double pulse1_output = 0.0;
	sequencer pulse1_seq;
	oscpulse pulse1_osc;
	envelope pulse1_env;
	lengthcounter pulse1_lc;
	sweeper pulse1_sweep;

	// Square Wave Pulse Channel 2 (Harmony channel)
	bool pulse2_enable = false;
	bool pulse2_halt = false;
	double pulse2_sample = 0.0;
	double pulse2_output = 0.0;
	sequencer pulse2_seq;
	oscpulse pulse2_osc;
	envelope pulse2_env;
	lengthcounter pulse2_lc;
	sweeper pulse2_sweep;

	// Triangle Channel (Bass)
	sequencer triangle_seq;
	oscpulse triangle_osc;
	lengthcounter triangle_lc;
	bool triangle_enable = false;
	bool triangle_halt = false;
	double triangle_sample = 0.0;
	double triangle_output = 0.0;
	// Triangle has a unique "linear counter" instead of an envelope
	uint8_t triangle_linear_counter = 0;
	uint8_t triangle_linear_reload = 0;
	bool triangle_linear_reload_flag = false;

	// Noise Channel (Percussion, explosions and snare drums)
	sequencer noise_seq;
	envelope noise_env;
	lengthcounter noise_lc;
	bool noise_enable = false;
	bool noise_halt = false;
	double noise_sample = 0.0;
	double noise_output = 0.0;
	// Noise uses a Linear Feedback Shift Register (LFSR) for randomness
	uint16_t noise_lfsr = 1; // MUST start at 1, not 0!
	bool noise_mode = false;

	// DPCM CHANNEL (Voice/Samples)
	bool dpcm_enable = false;
	bool dpcm_loop = false;
	bool dpcm_irq = false;
	uint16_t dpcm_addr_load = 0x0000;
	uint16_t dpcm_length_load = 0x0000;
	uint8_t dpcm_output = 0; // 7-bit output volume

public:
	uint16_t pulse1_visual = 0;
	uint16_t pulse2_visual = 0;
	uint16_t noise_visual = 0;
	uint16_t triangle_visual = 0;
	
};