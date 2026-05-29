#pragma once
#include <cstdint>
#include <memory>

#include "Cartridge.h"

class ppu2C02 {
public:
    ppu2C02();
    ~ppu2C02();

private:
    uint8_t tblName[2][1024]; // VRAM
    uint8_t tblPalette[32];

public:
    // Communications with Main bus
    uint8_t cpuRead(uint16_t addr, bool rdOnly = false);
    void    cpuWrite(uint16_t addr, uint8_t data);

    // Communications with PPU bus
    uint8_t ppuRead(uint16_t addr, bool rdOnly = false);
    void    ppuWrite(uint16_t addr, uint8_t data);  

private:
    // Cartridge or "GamePak"
    std::shared_ptr<Cartridge> cart;

public:
    //Interface
    void ConnectCartridge(const std::shared_ptr<Cartridge> &cartridge);
	void clock();
};