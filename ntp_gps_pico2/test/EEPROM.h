// EEPROM.h mock for native testing
#include "arduino_mock.h"
#include <vector>

class MockEEPROM {
private:
    std::vector<uint8_t> data;
    
public:
    MockEEPROM() : data(4096, 0) {}  // 4KB EEPROM simulation
    
    void begin(size_t size) {
        if (size > data.size()) {
            data.resize(size, 0);
        }
    }
    
    uint8_t read(int address) {
        if (address >= 0 && address < (int)data.size()) {
            return data[address];
        }
        return 0;
    }
    
    void write(int address, uint8_t value) {
        if (address >= 0 && address < (int)data.size()) {
            data[address] = value;
        }
    }
    
    void commit() {}
    void end() {}
    
    template<typename T>
    T get(int address, T& value) {
        if (address >= 0 && address + sizeof(T) <= data.size()) {
            memcpy(&value, &data[address], sizeof(T));
        }
        return value;
    }
    
    template<typename T>
    void put(int address, const T& value) {
        if (address >= 0 && address + sizeof(T) <= data.size()) {
            memcpy(&data[address], &value, sizeof(T));
        }
    }
};

extern MockEEPROM EEPROM;