#include "Bus.h"

Bus::Bus() 
{
	// Connect CPU to communication bus
	cpu.ConnectBus(this);
}


Bus::~Bus() 
{
}

void Bus::SetSampleFrequency(uint32_t sample_rate)
{
	dAudioTimePerSystemSample = 1.0 / (double)sample_rate;
	dAudioTimePerNESClock = 1.0 / 5369318.0; // PPU Clock Frequency
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
	else if ((addr >= 0x4000 && addr <= 0x4013) || addr == 0x4015 || addr == 0x4017) //  NES APU
	{
		apu.cpuWrite(addr, data);
	}
	else if (addr == 0x4014) {
		// A write to this address initiates a DMA transfer
		dma_page = data;
		dma_addr = 0x00;
		dma_transfer = true;
	}
	else if (addr >= 0x4016 && addr <= 0x4017){
		// "Lock In" controller state at this time
		controller_state[addr & 0x0001] = controller[addr & 0x0001];
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
	else if (addr == 0x4015) {
		// APU Read Status
		data = apu.cpuRead(addr);
	}
	else if (addr >= 0x4016 && addr <= 0x4017) {
		// Read out the MSB of the controller status word
		data = (controller_state[addr & 0x0001] & 0x80) > 0;
		// ALWAYS shift the register on every read, forcing the bits down
		controller_state[addr & 0x0001] <<= 1;
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
	cart->reset();
	cpu.reset();
	ppu.reset();
	nSystemClockCounter = 0;
	dma_page = 0x00;
	dma_addr = 0x00;
	dma_data = 0x00;
	dma_dummy = true;
	dma_transfer = false;
}

bool Bus::clock()
{	
	// Fastest clock freq is equivaled to PPU clock
	// So PPU is clocked each time this fnx is called
	ppu.clock();

	//...also clock APU
	apu.clock();

	// CPU runs 3x slower than PPU
	// clock() is called every 3 times this fxn is called
	// Global counter keeps the track of this
	if (nSystemClockCounter % 3 == 0) {
		// Is the system performing a DMA transfer form CPU memory to 
		// OAM memory on PPU?...
		if (dma_transfer) {
			// ...Yes! We need to wait until the next even CPU clock cycle
			// before it starts...
			if (dma_dummy) {
				// ...So hang around in here each clock until 1 or 2 cycles
				// have elapsed...
				if (nSystemClockCounter % 2 == 1)
				{
					// ...and finally allow DMA to start
					dma_dummy = false;
				}
			}
			else {
				// DMA can take place!
				if (nSystemClockCounter % 2 == 0)
				{
					// On even clock cycles, read from CPU bus
					dma_data = cpuRead(dma_page << 8 | dma_addr);
				}
				else
				{
					// On odd clock cycles, write to PPU OAM
					ppu.pOAM[dma_addr] = dma_data;
					// Increment the lo byte of the address
					dma_addr++;
					// If this wraps around, we know that 256
					// bytes have been written, so end the DMA
					// transfer, and proceed as normal
					if (dma_addr == 0x00)
					{
						dma_transfer = false;
						dma_dummy = true;
					}
				}
			}
		}
		else {
			// No DMA happening, the CPU is in control
			cpu.clock();
		}		
	}

	// Synchronising with Audio
	bool bAudioSampleReady = false;
	dAudioTime += dAudioTimePerNESClock;
	if (dAudioTime >= dAudioTimePerSystemSample)
	{
		dAudioTime -= dAudioTimePerSystemSample;
		dAudioSample = apu.GetOutputSample();
		bAudioSampleReady = true;
	}

	// The PPU is capable of emitting an interrupt to indicate the
	// vertical blanking period has been entered. If it has, we need
	// to send that irq to the CPU.
	if (ppu.nmi) {
		ppu.nmi = false;
		cpu.nmi();
	}

	// Check if cartridge is requesting IRQ
	if (cart->GetMapper()->irqState())
	{
		cart->GetMapper()->irqClear();
		cpu.irq();		
	}

	nSystemClockCounter++;

	return bAudioSampleReady;
}