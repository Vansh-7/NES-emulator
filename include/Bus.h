#pragma once
#include <cstdint>
#include <array>

#include "cpu6502.h"
#include "ppu2C02.h"
#include "Cartridge.h"

class Bus {
public:
	Bus();
	~Bus();

public:	// Devices on Main bus

	// 6502 CPU
	cpu6502 cpu;	
	// 2C02 Picture processing unit
	ppu2C02 ppu;
	// Cartridge or "GamePak"
    std::shared_ptr<Cartridge> cart;
	// 2KB of RAM
	uint8_t	cpuRam[2048];
	// Controllers
	uint8_t controller[2];

public:	// MAin Bus Read & Write
	void cpuWrite(uint16_t addr, uint8_t data);
	uint8_t cpuRead(uint16_t addr, bool bReadOnly = false);

public: // System Interface
	// connects cartridge to internal bus
	void insertCartridge(const std::shared_ptr<Cartridge> &cartridge);
	void reset();
	void clock();

private:
	// A count of how many clocks have passed
	uint32_t nSystemClockCounter = 0;
	// Internal cache of controller state
	uint8_t controller_state[2];
};
