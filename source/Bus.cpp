#include "Bus.h"

Bus::Bus() 
{
	// Connect CPU to communication bus
	cpu.ConnectBus(this);
}


Bus::~Bus() 
{
}

void Bus::cpuWrite(uint16_t addr, uint8_t data) 
{
	if (cart->cpuWrite(addr, data)) {
		// Cartridge Address Range
	}
	else if (addr >= 0x0000 && addr <= 0x1FFF) {
		// System RAM Address Range, mirrored every 2048 (addr % 2048)
		cpuRam[addr & 0x07FF] = data;
	}
	else if (addr >= 0x2000 && addr <= 0x3FFF) {
		// PPU Address range, mirrored every 8 (addr % 8)
		// Since PPU only has 8 primary regs
		ppu.cpuWrite(addr & 0x0007, data);
	}
		
}

uint8_t Bus::cpuRead(uint16_t addr, bool bReadOnly) 
{
	uint8_t data = 0x00;
	if (cart->cpuRead(addr, data)) {
		// Cartridge Address Range
	}
	else if (addr >= 0x0000 && addr <= 0x1FFF) {
		// System RAM Address Range, mirrored every 2048
		data = cpuRam[addr & 0x07FF];
	}
	else if (addr >= 0x2000 && addr <= 0x3FFF) {
		// PPU Address range, mirrored every 8
		data = ppu.cpuRead(addr & 0x0007, bReadOnly);
	}

	return data;
}

void Bus::insertCartridge(const std::shared_ptr<Cartridge> &cartridge)
{
	// Connects cartridge to both Main Bus and CPU Bus
	this->cart = cartridge;
	ppu.ConnectCartridge(cartridge);
}

void Bus::reset()
{
	cpu.reset();
	nSystemClockCounter = 0;
}

void Bus::clock()
{	
	// Fastest clock freq is equivaled to PPU clock
	// So PPU is clocked each time this fnx is called
	ppu.clock();

	// CPU runs 3x slower than PPU
	// clock() is called every 3 times this fxn is called
	// Global counter keeps the track of this
	if (nSystemClockCounter % 3 == 0) {
		cpu.clock();
	}

	nSystemClockCounter++;
}