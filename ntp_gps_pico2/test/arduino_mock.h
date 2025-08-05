#ifndef ARDUINO_MOCK_H
#define ARDUINO_MOCK_H

// Arduino mock for native testing environment
// This file provides minimal Arduino API simulation for unit tests

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstring>
#include <cstdlib>

// Basic time types - simple approach
typedef long arduino_time_t;

// Basic Arduino types
typedef uint8_t byte;
typedef bool boolean;

// Arduino constants
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

// Mock Arduino functions
inline unsigned long millis() { return 0; }
inline unsigned long micros() { return 0; }
inline void delay(unsigned long) {}
inline void delayMicroseconds(unsigned int) {}

// Mock Serial class
class MockSerial {
public:
    template<typename T>
    void print(T) {}
    
    template<typename T>
    void println(T) {}
    
    template<typename... Args>
    void printf(const char*, Args...) {}
    
    bool available() { return false; }
    int read() { return -1; }
    void begin(unsigned long) {}
    operator bool() { return true; }
};

extern MockSerial Serial;

// Mock String class - simplified without std::string dependency
class String {
private:
    static const size_t MAX_STRING_LENGTH = 256;
    char data[MAX_STRING_LENGTH];
    size_t len;
    
public:
    String() : len(0) { data[0] = '\0'; }
    
    String(const char* str) : len(0) {
        if (str) {
            strncpy(data, str, MAX_STRING_LENGTH - 1);
            data[MAX_STRING_LENGTH - 1] = '\0';
            len = strlen(data);
        } else {
            data[0] = '\0';
        }
    }
    
    String(const String& other) : len(other.len) {
        strcpy(data, other.data);
    }
    
    String& operator=(const String& other) {
        if (this != &other) {
            strcpy(data, other.data);
            len = other.len;
        }
        return *this;
    }
    
    String& operator=(const char* str) {
        if (str) {
            strncpy(data, str, MAX_STRING_LENGTH - 1);
            data[MAX_STRING_LENGTH - 1] = '\0';
            len = strlen(data);
        } else {
            data[0] = '\0';
            len = 0;
        }
        return *this;
    }
    
    bool operator==(const String& other) const {
        return strcmp(data, other.data) == 0;
    }
    
    const char* c_str() const { return data; }
    size_t length() const { return len; }
    
    int indexOf(const char* str) const {
        if (!str) return -1;
        const char* pos = strstr(data, str);
        return pos ? (int)(pos - data) : -1;
    }
};

// Mock Wire class
class MockWire {
public:
    void begin() {}
    void beginTransmission(uint8_t) {}
    uint8_t endTransmission() { return 0; }
    size_t write(uint8_t) { return 1; }
    uint8_t requestFrom(uint8_t, uint8_t) { return 0; }
    int available() { return 0; }
    int read() { return 0; }
    void setClock(uint32_t) {}
    void setSDA(uint8_t) {}
    void setSCL(uint8_t) {}
};

extern MockWire Wire;
extern MockWire Wire1;

// Forward declaration - definition is in separate header files
class MockEEPROM;

// Mock SPI class
class MockSPI {
public:
    void begin() {}
    void end() {}
    uint8_t transfer(uint8_t data) { return 0; }
    void beginTransaction(uint32_t) {}
    void endTransaction() {}
    void setClockDivider(uint8_t) {}
    void setDataMode(uint8_t) {}
    void setBitOrder(uint8_t) {}
};

extern MockSPI SPI;

// SPI constants
#define SPI_MODE0 0
#define SPI_MODE1 1
#define SPI_MODE2 2
#define SPI_MODE3 3
#define MSBFIRST 1
#define LSBFIRST 0

// Mock GPIO functions
inline void pinMode(uint8_t, uint8_t) {}
inline void digitalWrite(uint8_t, uint8_t) {}
inline int digitalRead(uint8_t) { return LOW; }
inline int analogRead(uint8_t) { return 0; }
inline void analogWrite(uint8_t, int) {}

// Mock interrupt functions
inline void attachInterrupt(uint8_t, void(*)(), int) {}
inline void detachInterrupt(uint8_t) {}

// Interrupt modes
#define RISING 1
#define FALLING 2
#define CHANGE 3
#define LOW 0
#define HIGH 1

// PROGMEM mock
#define PROGMEM
#define F(x) x

#endif // ARDUINO_MOCK_H