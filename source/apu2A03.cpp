#include "apu2A03.h"

uint8_t apu2A03::length_table[] = {  10, 254, 20,  2, 40,  4, 80,  6,
							        160,   8, 60, 10, 14, 12, 26, 14,
							         12,  16, 24, 18, 48, 20, 96, 22,
							        192,  24, 72, 26, 16, 28, 32, 30 };

apu2A03::apu2A03()
{
	noise_seq.sequence = 0xDBDB;
}


apu2A03::~apu2A03()
{
}

void apu2A03::cpuWrite(uint16_t addr, uint8_t data)
{
	switch (addr)
	{
	case 0x4000:
		switch ((data & 0xC0) >> 6)
		{
		case 0x00: pulse1_seq.new_sequence = 0b01000000; pulse1_osc.dutycycle = 0.125; break;
		case 0x01: pulse1_seq.new_sequence = 0b01100000; pulse1_osc.dutycycle = 0.250; break;
		case 0x02: pulse1_seq.new_sequence = 0b01111000; pulse1_osc.dutycycle = 0.500; break;
		case 0x03: pulse1_seq.new_sequence = 0b10011111; pulse1_osc.dutycycle = 0.750; break;
		}
		pulse1_seq.sequence = pulse1_seq.new_sequence;
		pulse1_halt = (data & 0x20);
		pulse1_env.volume = (data & 0x0F);
		pulse1_env.disable = (data & 0x10);
		break;

	case 0x4001:
		pulse1_sweep.enabled = data & 0x80;
		pulse1_sweep.period = (data & 0x70) >> 4;
		pulse1_sweep.down = data & 0x08;
		pulse1_sweep.shift = data & 0x07;
		pulse1_sweep.reload = true;
		break;

	case 0x4002:
		pulse1_seq.reload = (pulse1_seq.reload & 0xFF00) | data;
		break;

	case 0x4003:
		pulse1_seq.reload = (uint16_t)((data & 0x07)) << 8 | (pulse1_seq.reload & 0x00FF);
		pulse1_seq.timer = pulse1_seq.reload;
		pulse1_seq.sequence = pulse1_seq.new_sequence;
		pulse1_lc.counter = length_table[(data & 0xF8) >> 3];
		pulse1_env.start = true;
		break;

	case 0x4004:
		switch ((data & 0xC0) >> 6)
		{
		case 0x00: pulse2_seq.new_sequence = 0b01000000; pulse2_osc.dutycycle = 0.125; break;
		case 0x01: pulse2_seq.new_sequence = 0b01100000; pulse2_osc.dutycycle = 0.250; break;
		case 0x02: pulse2_seq.new_sequence = 0b01111000; pulse2_osc.dutycycle = 0.500; break;
		case 0x03: pulse2_seq.new_sequence = 0b10011111; pulse2_osc.dutycycle = 0.750; break;
		}
		pulse2_seq.sequence = pulse2_seq.new_sequence;
		pulse2_halt = (data & 0x20);
		pulse2_env.volume = (data & 0x0F);
		pulse2_env.disable = (data & 0x10);
		break;

	case 0x4005:
		pulse2_sweep.enabled = data & 0x80;
		pulse2_sweep.period = (data & 0x70) >> 4;
		pulse2_sweep.down = data & 0x08;
		pulse2_sweep.shift = data & 0x07;
		pulse2_sweep.reload = true;
		break;

	case 0x4006:
		pulse2_seq.reload = (pulse2_seq.reload & 0xFF00) | data;
		break;

	case 0x4007:
		pulse2_seq.reload = (uint16_t)((data & 0x07)) << 8 | (pulse2_seq.reload & 0x00FF);
		pulse2_seq.timer = pulse2_seq.reload;
		pulse2_seq.sequence = pulse2_seq.new_sequence;
		pulse2_lc.counter = length_table[(data & 0xF8) >> 3];
		pulse2_env.start = true;
		
		break;

	case 0x4008:
		triangle_linear_reload = data & 0x7F;
        triangle_halt = (data & 0x80) != 0;
		break;

	case 0x400A:
		triangle_seq.reload = (triangle_seq.reload & 0xFF00) | data;
		break;
	
	case 0x400B:
		triangle_seq.reload = (uint16_t)((data & 0x07) << 8) | (triangle_seq.reload & 0x00FF);
        triangle_seq.timer = triangle_seq.reload;
        triangle_lc.counter = length_table[data >> 3]; 
        triangle_linear_reload_flag = true;
		break;

	case 0x400C:
		noise_env.volume = (data & 0x0F);
		noise_env.disable = (data & 0x10) != 0;
		noise_halt = (data & 0x20) != 0;
		break;

	case 0x400E:
		// Noise periods lookup table
        static const uint16_t noise_periods[16] = { 4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068 };
        noise_seq.reload = noise_periods[data & 0x0F];
        
		// Capture Bit 7 to determine Mode 0 (White Noise) or Mode 1 (Metallic Noise)
        noise_mode = (data & 0x80) != 0;
		break;

	case 0x4010:
		dpcm_irq = (data & 0x80) != 0;
        dpcm_loop = (data & 0x40) != 0;
		// The bottom 4 bits (data & 0x0F) control the sample playback rate
		break;

	case 0x4011:
		// Direct load of the 7-bit output level
        dpcm_output = data & 0x7F;
		break;
	
	case 0x4012:
		// Sample address starts at $C000 + (data * 64)
        dpcm_addr_load = 0xC000 + ((uint16_t)data << 6);
		break;

	case 0x4013:
		// Sample length is (data * 16) + 1 bytes
        dpcm_length_load = ((uint16_t)data << 4) + 1;
		break;

	case 0x4015: // APU STATUS
		pulse1_enable = data & 0x01;
		pulse2_enable = data & 0x02;
		triangle_enable = data & 0x04;
		noise_enable = data & 0x08;
		dpcm_enable = data & 0x10;

		if (!pulse1_enable) pulse1_lc.counter = 0;
        if (!pulse2_enable) pulse2_lc.counter = 0;
        if (!triangle_enable) triangle_lc.counter = 0;
        if (!noise_enable) noise_lc.counter = 0;
		break;

	case 0x400F:
		pulse1_env.start = true;
		pulse2_env.start = true;
		noise_env.start = true;
		noise_lc.counter = length_table[(data & 0xF8) >> 3];
		break;
	}
}

uint8_t apu2A03::cpuRead(uint16_t addr)
{
	uint8_t data = 0x00;

	if (addr == 0x4015) {
		data |= (pulse1_lc.counter > 0) ? 0x01 : 0x00;
		data |= (pulse2_lc.counter > 0) ? 0x02 : 0x00;		
		data |= (noise_lc.counter > 0) ? 0x04 : 0x00;
	}

	return data;
}

void apu2A03::clock()
{
	// Depending on the frame count, we set a flag to tell 
	// us where we are in the sequence. Essentially, changes
	// to notes only occur at these intervals, meaning, in a
	// way, this is responsible for ensuring musical time is
	// maintained.
	bool bQuarterFrameClock = false;
	bool bHalfFrameClock = false;

	dGlobalTime += (0.3333333333 / 1789773);

	
	if (clock_counter % 6 == 0) {
		frame_clock_counter++;


		// 4-Step Sequence Mode
		if (frame_clock_counter == 3729)
		{
			bQuarterFrameClock = true;
		}

		if (frame_clock_counter == 7457)
		{
			bQuarterFrameClock = true;
			bHalfFrameClock = true;
		}

		if (frame_clock_counter == 11186)
		{
			bQuarterFrameClock = true;
		}

		if (frame_clock_counter == 14916)
		{
			bQuarterFrameClock = true;
			bHalfFrameClock = true;
			frame_clock_counter = 0;
		}

		// Update functional units

		// Quater frame "beats" adjust the volume envelope
		if (bQuarterFrameClock)
		{
			pulse1_env.clock(pulse1_halt);
			pulse2_env.clock(pulse2_halt);
			noise_env.clock(noise_halt);

			// Update Triangle Linear Counter
			if (triangle_linear_reload_flag)
				triangle_linear_counter = triangle_linear_reload;
			else if (triangle_linear_counter > 0)
				triangle_linear_counter--;

			if (!triangle_halt)
				triangle_linear_reload_flag = false;
		}


		// Half frame "beats" adjust the note length and
		// frequency sweepers
		if (bHalfFrameClock)
		{
			pulse1_lc.clock(pulse1_enable, pulse1_halt);
			pulse2_lc.clock(pulse2_enable, pulse2_halt);
			triangle_lc.clock(triangle_enable, triangle_halt);
			noise_lc.clock(noise_enable, noise_halt);
			pulse1_sweep.clock(pulse1_seq.reload, 0);
			pulse2_sweep.clock(pulse2_seq.reload, 1);
		}

        // Update Pulse1 Channel ================================
        pulse1_seq.clock(pulse1_enable, [](uint32_t &s)
        {
            s = ((s & 0x0001) << 7) | ((s & 0x00FE) >> 1);
        });

        // Use proper > 0 volume check, and mathematically correct 15.0 division
        if (pulse1_lc.counter > 0 && pulse1_seq.timer >= 8 && !pulse1_sweep.mute && pulse1_env.output > 0)
            pulse1_output = (double)pulse1_seq.output * ((double)pulse1_env.output / 15.0);
        else
            pulse1_output = 0;


        // Update Pulse2 Channel ================================
        pulse2_seq.clock(pulse2_enable, [](uint32_t &s)
        {
            s = ((s & 0x0001) << 7) | ((s & 0x00FE) >> 1);
        });

        if (pulse2_lc.counter > 0 && pulse2_seq.timer >= 8 && !pulse2_sweep.mute && pulse2_env.output > 0)
            pulse2_output = (double)pulse2_seq.output * ((double)pulse2_env.output / 15.0);
        else
            pulse2_output = 0;

		// Update Triangle Channel ==============================
		// The hardcoded jagged hardware array the NES uses for bass
        static const uint8_t triangle_sequence[32] = {
            15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
             0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
        };

        triangle_seq.clock(triangle_enable, [&](uint32_t &s)
        {
            // Only advance the sequence if counters are active
            if (triangle_lc.counter > 0 && triangle_linear_counter > 0 && triangle_seq.reload > 2)
            {
                s++;
                if (s >= 32) s = 0;
            }
        });

        triangle_output = (double)triangle_sequence[triangle_seq.sequence] / 15.0;

		// Update Noise Channel =================================
		noise_seq.clock(noise_enable, [&](uint32_t &s)
        {
            // Mode 0 uses bit 1, Mode 1 uses bit 6
            uint16_t shift = noise_mode ? 6 : 1;
            uint16_t feedback = (noise_lfsr & 0x0001) ^ ((noise_lfsr & (1 << shift)) >> shift);
            noise_lfsr = (noise_lfsr >> 1) | (feedback << 14);
        });

        if (noise_lc.counter > 0 && (noise_lfsr & 0x0001) == 0 && noise_env.output > 0)
        {
            noise_output = (double)noise_env.output / 15.0; 
        }
        else
        {
            noise_output = 0;
        }

		if (!pulse1_enable) pulse1_output = 0;
		if (!pulse2_enable) pulse2_output = 0;
		if (!noise_enable) noise_output = 0;
	}

	// Frequency sweepers change at high frequency
	pulse1_sweep.track(pulse1_seq.reload);
	pulse2_sweep.track(pulse2_seq.reload);

	pulse1_visual = (pulse1_enable && pulse1_env.output > 1 && !pulse1_sweep.mute) ? pulse1_seq.reload : 2047;
	pulse2_visual = (pulse2_enable && pulse2_env.output > 1 && !pulse2_sweep.mute) ? pulse2_seq.reload : 2047;
	noise_visual = (noise_enable && noise_env.output > 1) ? noise_seq.reload : 2047;

	clock_counter++;
}

double apu2A03::GetOutputSample()
{
    // Scale the PC 0.0-1.0 outputs back to authentic 0-15 Hardware Levels
    double p1 = pulse1_output * 15.0;
    double p2 = pulse2_output * 15.0;
    double tr = triangle_output * 15.0;
    double nn = noise_output * 15.0;
    double dp = (double)dpcm_output; // DPCM is already 0-127

    // The Authentic Motherboard Resistor Mixer Formula
    double pulse_out = 0.0;
    if (p1 + p2 > 0.0)
    {
        pulse_out = 95.88 / ((8128.0 / (p1 + p2)) + 100.0);
    }

    double tnd_out = 0.0;
    if (tr + nn + dp > 0.0)
    {
        tnd_out = 159.79 / (1.0 / ((tr / 8227.0) + (nn / 12241.0) + (dp / 22638.0)) + 100.0);
    }

    double raw_sample = pulse_out + tnd_out;

    // Authentic CRT TV Filter Chain
    // The NES output passes through specific electrical filters before hitting the TV speaker.

    // High-Pass Filter (90Hz) - Removes DC offset and centers the wave perfectly on 0.0
    static double hp_prev_in = 0.0;
    static double hp_prev_out = 0.0;
    double hp_out = 0.996 * hp_prev_out + 0.996 * (raw_sample - hp_prev_in);
    hp_prev_in = raw_sample;
    hp_prev_out = hp_out;

    // Low-Pass Filter (14kHz) - Muffles the harsh digital razor-edges (The "CRT" sound)
    static double lp_prev_out = 0.0;
    double lp_out = lp_prev_out + 0.5 * (hp_out - lp_prev_out);
    lp_prev_out = lp_out;

    return lp_out; 
}

void apu2A03::reset()
{
}