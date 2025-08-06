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
#include <cstdarg>

// Basic time types - simple approach
typedef long arduino_time_t;
typedef long time_t;

// Simple tm structure for time management
struct tm {
    int tm_sec;    // seconds after the minute - [0, 60] including leap second
    int tm_min;    // minutes after the hour - [0, 59]
    int tm_hour;   // hours since midnight - [0, 23]
    int tm_mday;   // day of the month - [1, 31]
    int tm_mon;    // months since January - [0, 11]
    int tm_year;   // years since 1900
    int tm_wday;   // days since Sunday - [0, 6]
    int tm_yday;   // days since January 1 - [0, 365]
    int tm_isdst;  // daylight savings time flag
};

// Simple time functions
inline time_t mktime(struct tm* timeptr) {
    // Simple mock implementation - returns fixed time for testing
    return 1577836800; // 2020-01-01 00:00:00 UTC
}

// Basic Arduino types
typedef uint8_t byte;
typedef bool boolean;

// Arduino constants
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

// Additional Arduino constants
#define RISING 1
#define FALLING 2
#define CHANGE 3

// Mock Arduino functions with incrementing time
static unsigned long mock_millis_counter = 1000;
static unsigned long mock_micros_counter = 1000000;

inline unsigned long millis() { 
    return mock_millis_counter++;
}

inline unsigned long micros() { 
    return mock_micros_counter++;
}

inline void delay(unsigned long ms) {
    mock_millis_counter += ms;
    mock_micros_counter += ms * 1000;
}

inline void delayMicroseconds(unsigned int us) {
    mock_micros_counter += us;
    mock_millis_counter += us / 1000;
}

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