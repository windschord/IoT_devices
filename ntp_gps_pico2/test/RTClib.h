// RTClib.h mock for native testing
#include "arduino_mock.h"
#include <stdint.h>

class DateTime {
public:
    DateTime() {}
    DateTime(uint16_t y, uint8_t m, uint8_t d, uint8_t h = 0, uint8_t mm = 0, uint8_t s = 0) {}
    DateTime(const char* date, const char* time) {}
    
    uint16_t year() const { return 2024; }
    uint8_t month() const { return 1; }
    uint8_t day() const { return 1; }
    uint8_t hour() const { return 12; }
    uint8_t minute() const { return 0; }
    uint8_t second() const { return 0; }
    uint8_t dayOfTheWeek() const { return 1; }
    uint32_t unixtime() const { return 1704067200; }
};

class RTC_DS3231 {
public:
    bool begin() { return true; }
    void adjust(const DateTime&) {}
    DateTime now() { return DateTime(); }
    bool lostPower() { return false; }
    float getTemperature() { return 25.0; }
};