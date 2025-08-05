// EEPROM.h mock for native testing
#include "arduino_mock.h"

class MockEEPROM {
private:
    static const size_t MAX_SIZE = 4096;
    uint8_t data[MAX_SIZE];
    size_t current_size;
    
public:
    MockEEPROM() : current_size(MAX_SIZE) {
        memset(data, 0, MAX_SIZE);
    }
    
    void begin(size_t size) {
        if (size > MAX_SIZE) {
            current_size = MAX_SIZE;
        } else {
            current_size = size;
        }
    }
    
    uint8_t read(int address) {
        if (address >= 0 && address < (int)current_size) {
            return data[address];
        }
        return 0;
    }
    
    void write(int address, uint8_t value) {
        if (address >= 0 && address < (int)current_size) {
            data[address] = value;
        }
    }
    
    void commit() {}
    void end() {}
    
    template<typename T>
    T get(int address, T& value) {
        if (address >= 0 && address + sizeof(T) <= current_size) {
            memcpy(&value, &data[address], sizeof(T));
        }
        return value;
    }
    
    template<typename T>
    void put(int address, const T& value) {
        if (address >= 0 && address + sizeof(T) <= current_size) {
            memcpy(&data[address], &value, sizeof(T));
        }
    }
};

extern MockEEPROM EEPROM;