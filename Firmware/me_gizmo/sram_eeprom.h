#ifndef _SRAM_EEPROM_H_
#define _SRAM_EEPROM_H_

// Drop-in replacement for the flash-backed <EEPROM.h> library, for use on
// parts (e.g. flash-less RP2350 builds) where EEPROM.h's flash-sector
// emulation isn't available. Provides the same begin/read/write/commit/length
// interface used throughout this project, but the storage lives in plain
// SRAM instead of flash.
//
// IMPORTANT: because this is SRAM, not flash, the contents are NOT
// persistent across power cycles or resets -- commit() is a no-op kept only
// for API compatibility with existing call sites (e.g. eeprom_commit_handler,
// eeprom_erase_handler in parser.cpp).

#include <stdint.h>
#include <stddef.h>
#include <string.h>

class SRAM_EEPROM_Class {
public:
    // Mirrors EEPROMClass::begin(size_t) -- allocates/initializes the backing
    // buffer. Matches flash-erased convention of 0xFF per byte.
    void begin(size_t size) {
        if (_data != nullptr) {
            delete[] _data;
        }
        _size = size;
        _data = new uint8_t[_size];
        memset(_data, 0xFF, _size);
    }

    // Mirrors EEPROMClass::read(int)
    uint8_t read(int address) const {
        if ((_data == nullptr) || (address < 0) || ((size_t)address >= _size)) {
            return 0xFF;
        }
        return _data[address];
    }

    // Mirrors EEPROMClass::write(int, uint8_t)
    void write(int address, uint8_t value) {
        if ((_data == nullptr) || (address < 0) || ((size_t)address >= _size)) {
            return;
        }
        _data[address] = value;
    }

    // Mirrors EEPROMClass::commit(). No-op: writes already land directly in
    // SRAM, there is no separate flash page to flush.
    bool commit(void) {
        return true;
    }

    // Mirrors EEPROMClass::length()
    size_t length(void) const {
        return _size;
    }

    ~SRAM_EEPROM_Class() {
        if (_data != nullptr) {
            delete[] _data;
        }
    }

private:
    uint8_t *_data = nullptr;
    size_t _size = 0;
};

extern SRAM_EEPROM_Class EEPROM;

#endif
