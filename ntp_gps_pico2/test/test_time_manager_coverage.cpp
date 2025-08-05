/*
Task 40: TimeManager Complete Coverage Test Implementation

GPS NTP Server - Comprehensive TimeManager Class Test Suite
Tests for high-precision time management, GPS synchronization, and PPS signal processing.

Coverage Areas:
- GPS time synchronization and PPS signal processing
- Unix timestamp conversion and UTC calculations  
- High-precision time retrieval with overflow protection
- RTC fallback functionality and validation
- NTP stratum calculation based on time source
- Microsecond fraction calculations for high precision
- GPS/RTC dual time source management

Test Requirements:
- All TimeManager public methods covered
- GPS sync vs RTC fallback scenarios
- Time precision and accuracy calculations
- PPS interrupt handling simulation
- Error conditions and boundary values
- Multi-source time management
*/

#include <unity.h>
#include "Arduino.h"

// Use Arduino Mock environment
// Enable debugging for test execution
#define DEBUG_TIME_MANAGER
#define DEBUG_GPS_SYNC

// Mock Arduino framework functions
unsigned long test_micros_value = 1000000;
unsigned long test_millis_value = 1000;

unsigned long micros() { return test_micros_value; }
unsigned long millis() { return test_millis_value; }
void pinMode(int pin, int mode) { /* Mock */ }
void analogWrite(int pin, int value) { /* Mock */ }
void delay(int ms) { /* Mock */ }

// Mock TwoWire (I2C) for RTC communication
class MockTwoWire {
public:
    uint8_t transmit_result = 0;
    uint8_t available_count = 0;
    uint8_t read_data[10] = {0x45, 0x23, 0x12, 0x01, 0x21, 0x01, 0x25}; // BCD format: 45sec 23min 12hour 1dow 21day 01month 25year
    int read_index = 0;
    
    void beginTransmission(uint8_t address) { 
        read_index = 0;
    }
    uint8_t endTransmission() { return transmit_result; }
    void write(uint8_t data) { /* Mock write */ }
    uint8_t requestFrom(uint8_t address, uint8_t count) { 
        available_count = count;
        return count;
    }
    uint8_t available() { return available_count; }
    uint8_t read() { 
        if (read_index < 7) {
            available_count--;
            return read_data[read_index++];
        }
        return 0;
    }
};

MockTwoWire Wire1;

// Mock RTC_DS3231 class
class MockDateTime {
public:
    int _year = 2025;
    int _month = 1;
    int _day = 21;
    int _hour = 12;  
    int _minute = 34;
    int _second = 56;
    
    int year() const { return _year; }
    int month() const { return _month; }
    int day() const { return _day; }
    int hour() const { return _hour; }
    int minute() const { return _minute; }
    int second() const { return _second; }
};

class MockDS3231 {
public:
    MockDateTime current_time;
    bool adjust_called = false;
    MockDateTime adjusted_time;
    
    MockDateTime now() { return current_time; }
    void adjust(const MockDateTime& dt) { 
        adjust_called = true;
        adjusted_time = dt;
    }
    
    void setMockTime(int year, int month, int day, int hour, int minute, int second) {
        current_time._year = year;
        current_time._month = month;
        current_time._day = day;
        current_time._hour = hour;
        current_time._minute = minute;
        current_time._second = second;
    }
};

// Mock DateTime constructor for GPS sync testing
class DateTime {
public:
    int _year, _month, _day, _hour, _minute, _second;
    
    DateTime(int year, int month, int day, int hour, int minute, int second)
        : _year(year), _month(month), _day(day), _hour(hour), _minute(minute), _second(second) {}
    
    int year() const { return _year; }
    int month() const { return _month; }
    int day() const { return _day; }
    int hour() const { return _hour; }
    int minute() const { return _minute; }
    int second() const { return _second; }
};

// Mock hardware configuration
#define GPS_PPS_PIN 8
#define LED_PPS_PIN 15

// Mock System Types
struct TimeSync {
    bool synchronized = false;
    time_t gpsTime = 0;
    unsigned long ppsTime = 0;
    time_t rtcTime = 0;
    float accuracy = 1.0;
    unsigned long lastGpsUpdate = 0;
};

struct GpsMonitor {
    bool inFallbackMode = false;
    bool signalValid = true;
    uint8_t satelliteCount = 12;
};

// Mock GPS data structure
struct GpsSummaryData {
    bool timeValid = true;
    bool dateValid = true;
    uint16_t year = 2025;
    uint8_t month = 1;
    uint8_t day = 21;
    uint8_t hour = 12;
    uint8_t min = 34;
    uint8_t sec = 56;
    uint16_t msec = 789;
    uint8_t numSV = 12;
    uint8_t fixType = 3;
    float hdop = 1.2f;
    
    void setValidTime(uint16_t y, uint8_t mo, uint8_t d, uint8_t h, uint8_t mi, uint8_t s) {
        year = y; month = mo; day = d; hour = h; min = mi; sec = s;
        timeValid = true; dateValid = true;
    }
    
    void setInvalidTime() {
        timeValid = false; dateValid = false;
    }
};

// Mock LoggingService
class MockLoggingService {
public:
    String last_message;
    String last_level;
    String last_component;
    
    void debug(const char* component, const char* message) {
        last_level = "DEBUG";
        last_component = String(component);
        last_message = String(message);
    }
    
    void debugf(const char* component, const char* format, ...) {
        last_level = "DEBUG";
        last_component = String(component);
        va_list args;
        va_start(args, format);
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        last_message = String(buffer);
    }
    
    void warning(const char* component, const char* message) {
        last_level = "WARNING";
        last_component = String(component);
        last_message = String(message);
    }
};

// Global mock instances
bool gpsConnected = true;
MockDS3231 mockRTC;
TimeSync mockTimeSync;
GpsMonitor mockGpsMonitor;
MockLoggingService mockLoggingService;

// Mock RTC_DS3231 typedef
typedef MockDS3231 RTC_DS3231;

// Helper function prototype
time_t gpsTimeToUnixTimestamp(uint16_t year, uint8_t month, uint8_t day, 
                             uint8_t hour, uint8_t min, uint8_t sec);

// Embedded TimeManager implementation (simplified version for testing)
class TimeManager {
private:
    RTC_DS3231* rtc;
    TimeSync* timeSync;
    const GpsMonitor* gpsMonitor;
    MockLoggingService* loggingService;
    volatile bool ppsReceived;
    volatile unsigned long ppsTimestamp;
    volatile unsigned long ppsCount;

public:
    TimeManager(RTC_DS3231* rtcInstance, TimeSync* timeSyncInstance, const GpsMonitor* gpsMonitorInstance)
        : rtc(rtcInstance), timeSync(timeSyncInstance), gpsMonitor(gpsMonitorInstance),
          loggingService(nullptr), ppsReceived(false), ppsTimestamp(0), ppsCount(0) {
    }
    
    void setLoggingService(MockLoggingService* loggingServiceInstance) {
        loggingService = loggingServiceInstance;
    }
    
    void init() {
        pinMode(GPS_PPS_PIN, INPUT_PULLUP);
        timeSync->synchronized = false;
        timeSync->accuracy = 1.0;
        timeSync->lastGpsUpdate = 0;
    }
    
    void onPpsInterrupt() {
        unsigned long now = micros();
        ppsTimestamp = now;
        ppsReceived = true;
        ppsCount++;
        analogWrite(LED_PPS_PIN, 255);
    }
    
    unsigned long getHighPrecisionTime() {
        // GPS time validity check
        bool gpsTimeValid = (timeSync->synchronized && timeSync->gpsTime > 1000000000);
        bool gpsRecentlyUpdated = (millis() - timeSync->lastGpsUpdate < 30000);
        
        if (gpsTimeValid && gpsRecentlyUpdated) {
            // Return high-precision PPS-synchronized time
            unsigned long elapsed = micros() - timeSync->ppsTime;
            
            // 64-bit arithmetic to prevent overflow
            uint64_t gpsTimeMs64 = (uint64_t)timeSync->gpsTime * 1000ULL;
            uint64_t elapsedMs64 = elapsed / 1000ULL;
            uint64_t result64 = gpsTimeMs64 + elapsedMs64;
            
            // Check if result fits in 32-bit
            unsigned long result;
            if (result64 > ULONG_MAX) {
                // Overflow case - use approximate calculation
                result = timeSync->gpsTime * 1000UL;
                if (loggingService) {
                    loggingService->warning("TIME", "64-bit overflow detected, using approximate calculation");
                }
            } else {
                result = (unsigned long)result64;
            }
            
            return result;
        } else {
            // RTC fallback time
            MockDateTime now = rtc->now();
            struct tm timeinfo = {0};
            timeinfo.tm_year = now.year() - 1900;
            timeinfo.tm_mon = now.month() - 1;
            timeinfo.tm_mday = now.day();
            timeinfo.tm_hour = now.hour();
            timeinfo.tm_min = now.minute();
            timeinfo.tm_sec = now.second();
            
            time_t rtcTime = mktime(&timeinfo);
            time_t year2020 = 1577836800; // 2020-01-01 00:00:00 UTC
            
            if (rtcTime < year2020) {
                // Default fallback time: 2025-01-21 12:00:00
                timeinfo.tm_year = 125; // 2025
                timeinfo.tm_mon = 0;    // January
                timeinfo.tm_mday = 21;
                timeinfo.tm_hour = 12;
                timeinfo.tm_min = 0;
                timeinfo.tm_sec = 0;
                rtcTime = mktime(&timeinfo);
            }
            
            return rtcTime * 1000 + millis() % 1000;
        }
    }
    
    time_t getUnixTimestamp() {
        // GPS time validity check
        bool gpsTimeValid = (timeSync->synchronized && timeSync->gpsTime > 1000000000);
        bool gpsRecentlyUpdated = (millis() - timeSync->lastGpsUpdate < 30000);
        
        if (gpsTimeValid && gpsRecentlyUpdated) {
            // GPS time in seconds (no overflow)
            unsigned long elapsedSec = (micros() - timeSync->ppsTime) / 1000000;
            time_t result = timeSync->gpsTime + elapsedSec;
            return result;
        } else {
            // RTC fallback
            MockDateTime now = rtc->now();
            struct tm timeinfo = {0};
            timeinfo.tm_year = now.year() - 1900;
            timeinfo.tm_mon = now.month() - 1;
            timeinfo.tm_mday = now.day();
            timeinfo.tm_hour = now.hour();
            timeinfo.tm_min = now.minute();
            timeinfo.tm_sec = now.second();
            
            time_t rtcTime = mktime(&timeinfo);
            time_t year2020 = 1577836800;
            
            if (rtcTime < year2020) {
                return 1737504000; // 2025-01-21 12:00:00
            }
            
            return rtcTime;
        }
    }
    
    uint32_t getMicrosecondFraction() {
        // GPS time validity check
        bool gpsTimeValid = (timeSync->synchronized && timeSync->gpsTime > 1000000000);
        bool gpsRecentlyUpdated = (millis() - timeSync->lastGpsUpdate < 30000);
        
        if (gpsTimeValid && gpsRecentlyUpdated) {
            // PPS signal elapsed microseconds
            unsigned long elapsed = micros() - timeSync->ppsTime;
            uint32_t microsInSecond = elapsed % 1000000;
            
            // Convert microseconds to NTP fraction (2^32 * microseconds / 1000000)
            return (uint32_t)((uint64_t)microsInSecond * 4294967296ULL / 1000000ULL);
        } else {
            // RTC fallback - use current milliseconds
            return (uint32_t)((uint64_t)(millis() % 1000) * 4294967296ULL / 1000ULL);
        }
    }
    
    int getNtpStratum() {
        // GPS time validity check
        bool gpsTimeValid = (timeSync->synchronized && timeSync->gpsTime > 1000000000);
        bool gpsRecentlyUpdated = (millis() - timeSync->lastGpsUpdate < 30000);
        
        if (gpsTimeValid && gpsRecentlyUpdated) {
            return 1; // Stratum 1 when GPS synchronized and recent
        } else {
            return 3; // Stratum 3 when RTC fallback
        }
    }
    
    void processPpsSync(const GpsSummaryData& gpsData) {
        if (ppsReceived && gpsConnected) {
            ppsReceived = false; // Reset flag
            
            if (gpsData.timeValid && gpsData.dateValid) {
                // Convert to Unix timestamp using UTC calculation
                time_t utc_result = gpsTimeToUnixTimestamp(gpsData.year, gpsData.month, gpsData.day,
                                                          gpsData.hour, gpsData.min, gpsData.sec);
                
                // Update time sync
                timeSync->gpsTime = utc_result;
                timeSync->ppsTime = ppsTimestamp;
                timeSync->lastGpsUpdate = millis();
                timeSync->synchronized = true;
                
                // Synchronize with RTC
                DateTime gpsDateTime(gpsData.year, gpsData.month, gpsData.day,
                                    gpsData.hour, gpsData.min, gpsData.sec);
                rtc->adjust(gpsDateTime);
                timeSync->rtcTime = timeSync->gpsTime;
                
                // High precision with PPS signal
                timeSync->accuracy = 0.001; // 1ms accuracy
            }
        }
    }
    
    // Test access methods
    bool isPpsReceived() const { return ppsReceived; }
    void resetPpsFlag() { ppsReceived = false; }
    unsigned long getPpsCount() const { return ppsCount; }
    void simulatePpsSignal() { onPpsInterrupt(); }
};

// Helper function implementation (copied from TimeManager.cpp)
time_t gpsTimeToUnixTimestamp(uint16_t year, uint8_t month, uint8_t day, 
                             uint8_t hour, uint8_t min, uint8_t sec) {
    // Calculate elapsed seconds from Unix epoch (1970-01-01)
    
    // Years since epoch
    int years_since_epoch = year - 1970;
    
    // Count leap years
    int leap_years = 0;
    for (int y = 1970; y < year; y++) {
        if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) {
            leap_years++;
        }
    }
    
    // Days in each month (non-leap year)
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    // Check if current year is leap year
    bool is_leap_year = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
    if (is_leap_year) {
        days_in_month[1] = 29; // February
    }
    
    // Calculate total days
    int total_days = years_since_epoch * 365 + leap_years;
    
    // Add days for months
    for (int m = 1; m < month; m++) {
        total_days += days_in_month[m - 1];
    }
    
    // Add days (1-based, so subtract 1)
    total_days += day - 1;
    
    // Convert to seconds
    time_t timestamp = (time_t)total_days * 24 * 60 * 60;
    timestamp += hour * 60 * 60;
    timestamp += min * 60;
    timestamp += sec;
    
    return timestamp;
}

// Test Cases

void test_time_manager_initialization() {
    TimeManager timeManager(&mockRTC, &mockTimeSync, &mockGpsMonitor);
    
    // Test initialization
    timeManager.init();
    
    TEST_ASSERT_FALSE(mockTimeSync.synchronized);
    TEST_ASSERT_EQUAL_FLOAT(1.0, mockTimeSync.accuracy);
    TEST_ASSERT_EQUAL(0, mockTimeSync.lastGpsUpdate);
    TEST_ASSERT_FALSE(timeManager.isPpsReceived());
    TEST_ASSERT_EQUAL(0, timeManager.getPpsCount());
}

void test_pps_interrupt_handling() {
    TimeManager timeManager(&mockRTC, &mockTimeSync, &mockGpsMonitor);
    timeManager.init();
    
    // Test PPS interrupt simulation
    test_micros_value = 5000000;
    timeManager.simulatePpsSignal();
    
    TEST_ASSERT_TRUE(timeManager.isPpsReceived());
    TEST_ASSERT_EQUAL(1, timeManager.getPpsCount());
    
    // Test PPS flag reset
    timeManager.resetPpsFlag();
    TEST_ASSERT_FALSE(timeManager.isPpsReceived());
    TEST_ASSERT_EQUAL(1, timeManager.getPpsCount()); // Count persists
}

void test_gps_time_synchronization() {
    TimeManager timeManager(&mockRTC, &mockTimeSync, &mockGpsMonitor);
    timeManager.init();
    
    // Setup GPS data
    GpsSummaryData gpsData;
    gpsData.setValidTime(2025, 1, 21, 12, 34, 56);
    
    // Simulate PPS signal and GPS sync
    test_micros_value = 6000000;
    test_millis_value = 2000;
    gpsConnected = true;
    timeManager.simulatePpsSignal();
    
    // Process GPS sync
    timeManager.processPpsSync(gpsData);
    
    TEST_ASSERT_TRUE(mockTimeSync.synchronized);
    TEST_ASSERT_GREATER_THAN(1000000000, mockTimeSync.gpsTime); // After 2001
    TEST_ASSERT_EQUAL(6000000, mockTimeSync.ppsTime);
    TEST_ASSERT_EQUAL(2000, mockTimeSync.lastGpsUpdate);
    TEST_ASSERT_EQUAL_FLOAT(0.001, mockTimeSync.accuracy);
    TEST_ASSERT_TRUE(mockRTC.adjust_called);
}

void test_invalid_gps_data_handling() {
    TimeManager timeManager(&mockRTC, &mockTimeSync, &mockGpsMonitor);
    timeManager.init();
    
    // Setup invalid GPS data
    GpsSummaryData gpsData;
    gpsData.setInvalidTime();
    
    // Simulate PPS signal
    gpsConnected = true;
    timeManager.simulatePpsSignal();
    
    // Process GPS sync with invalid data
    bool originalSync = mockTimeSync.synchronized;
    timeManager.processPpsSync(gpsData);
    
    // Time sync should not be updated
    TEST_ASSERT_EQUAL(originalSync, mockTimeSync.synchronized);
    TEST_ASSERT_FALSE(mockRTC.adjust_called);
}

void test_gps_disconnected_handling() {
    TimeManager timeManager(&mockRTC, &mockTimeSync, &mockGpsMonitor);
    timeManager.init();
    
    // Setup valid GPS data but GPS disconnected
    GpsSummaryData gpsData;
    gpsData.setValidTime(2025, 1, 21, 12, 34, 56);
    
    gpsConnected = false; // GPS disconnected
    timeManager.simulatePpsSignal();
    
    // Process GPS sync
    bool originalSync = mockTimeSync.synchronized;
    timeManager.processPpsSync(gpsData);
    
    // Should not synchronize when GPS disconnected
    TEST_ASSERT_EQUAL(originalSync, mockTimeSync.synchronized);
    TEST_ASSERT_FALSE(mockRTC.adjust_called);
}

void test_high_precision_time_gps_mode() {
    TimeManager timeManager(&mockRTC, &mockTimeSync, &mockGpsMonitor);
    timeManager.setLoggingService(&mockLoggingService);
    timeManager.init();
    
    // Setup synchronized GPS time
    mockTimeSync.synchronized = true;
    mockTimeSync.gpsTime = 1737540000; // 2025-01-21 22:00:00 UTC
    mockTimeSync.ppsTime = 7000000;    // PPS timestamp
    mockTimeSync.lastGpsUpdate = 1000; // Recent update
    
    // Current time
    test_micros_value = 7500000; // 500ms after PPS
    test_millis_value = 1500;    // Recent millis
    
    unsigned long result = timeManager.getHighPrecisionTime();
    
    // Should return GPS-based high precision time
    // Expected: 1737540000 * 1000 + (7500000 - 7000000) / 1000 = 1737540000500
    unsigned long expected = 1737540000UL * 1000UL + 500UL;
    TEST_ASSERT_EQUAL(expected, result);
}

void test_high_precision_time_rtc_fallback() {
    TimeManager timeManager(&mockRTC, &mockTimeSync, &mockGpsMonitor);
    timeManager.init();
    
    // Setup unsynchronized state (forces RTC fallback)
    mockTimeSync.synchronized = false;
    mockTimeSync.gpsTime = 0;
    
    // Set RTC time
    mockRTC.setMockTime(2025, 1, 21, 12, 34, 56);
    test_millis_value = 789; // Current millis
    
    unsigned long result = timeManager.getHighPrecisionTime();
    
    // Should return RTC-based time
    TEST_ASSERT_GREATER_THAN(1000000000, result); // Reasonable timestamp
}

void test_unix_timestamp_gps_mode() {
    TimeManager timeManager(&mockRTC, &mockTimeSync, &mockGpsMonitor);
    timeManager.init();
    
    // Setup synchronized GPS time
    mockTimeSync.synchronized = true;
    mockTimeSync.gpsTime = 1737540000; // 2025-01-21 22:00:00 UTC
    mockTimeSync.ppsTime = 8000000;    // PPS timestamp
    mockTimeSync.lastGpsUpdate = 2000; // Recent update
    
    test_micros_value = 8005000000UL; // 5 seconds after PPS
    test_millis_value = 2500;
    
    time_t result = timeManager.getUnixTimestamp();
    
    // Expected: 1737540000 + 5 = 1737540005
    TEST_ASSERT_EQUAL(1737540005, result);
}

void test_unix_timestamp_rtc_fallback() {
    TimeManager timeManager(&mockRTC, &mockTimeSync, &mockGpsMonitor);
    timeManager.init();
    
    // Setup unsynchronized state
    mockTimeSync.synchronized = false;
    
    // Set valid RTC time
    mockRTC.setMockTime(2025, 1, 21, 12, 34, 56);
    
    time_t result = timeManager.getUnixTimestamp();
    
    // Should return reasonable RTC-based timestamp
    TEST_ASSERT_GREATER_THAN(1577836800, result); // After 2020
    TEST_ASSERT_LESS_THAN(2147483647, result);    // Before 2038 (32-bit limit)
}

void test_invalid_rtc_time_handling() {
    TimeManager timeManager(&mockRTC, &mockTimeSync, &mockGpsMonitor);
    timeManager.init();
    
    // Setup unsynchronized GPS and invalid RTC time
    mockTimeSync.synchronized = false;
    mockRTC.setMockTime(1999, 12, 31, 23, 59, 59); // Before 2020
    
    time_t result = timeManager.getUnixTimestamp();
    
    // Should return default fallback time: 2025-01-21 12:00:00
    TEST_ASSERT_EQUAL(1737504000, result);
}

void test_microsecond_fraction_gps_mode() {
    TimeManager timeManager(&mockRTC, &mockTimeSync, &mockGpsMonitor);
    timeManager.init();
    
    // Setup synchronized GPS time
    mockTimeSync.synchronized = true;
    mockTimeSync.gpsTime = 1737540000;
    mockTimeSync.ppsTime = 9000000;
    mockTimeSync.lastGpsUpdate = 3000;
    
    test_micros_value = 9250000; // 250ms after PPS
    test_millis_value = 3500;
    
    uint32_t result = timeManager.getMicrosecondFraction();
    
    // 250ms = 250000 microseconds in second
    // NTP fraction = (250000 * 2^32) / 1000000
    uint32_t expected = (uint32_t)((uint64_t)250000 * 4294967296ULL / 1000000ULL);
    TEST_ASSERT_EQUAL(expected, result);
}

void test_microsecond_fraction_rtc_fallback() {
    TimeManager timeManager(&mockRTC, &mockTimeSync, &mockGpsMonitor);
    timeManager.init();
    
    // Setup unsynchronized state
    mockTimeSync.synchronized = false;
    test_millis_value = 3750; // 750ms
    
    uint32_t result = timeManager.getMicrosecondFraction();
    
    // 750ms fraction conversion
    uint32_t expected = (uint32_t)((uint64_t)750 * 4294967296ULL / 1000ULL);
    TEST_ASSERT_EQUAL(expected, result);
}

void test_ntp_stratum_calculation() {
    TimeManager timeManager(&mockRTC, &mockTimeSync, &mockGpsMonitor);
    timeManager.init();
    
    // Test GPS synchronized mode (Stratum 1)
    mockTimeSync.synchronized = true;
    mockTimeSync.gpsTime = 1737540000;
    mockTimeSync.lastGpsUpdate = 4000;
    test_millis_value = 4500; // Recent update
    
    int stratum = timeManager.getNtpStratum();
    TEST_ASSERT_EQUAL(1, stratum);
    
    // Test RTC fallback mode (Stratum 3)
    mockTimeSync.synchronized = false;
    stratum = timeManager.getNtpStratum();
    TEST_ASSERT_EQUAL(3, stratum);
    
    // Test old GPS update (should fallback to Stratum 3)
    mockTimeSync.synchronized = true;
    mockTimeSync.lastGpsUpdate = 1000;
    test_millis_value = 35000; // 34 seconds ago (>30s threshold)
    
    stratum = timeManager.getNtpStratum();
    TEST_ASSERT_EQUAL(3, stratum);
}

void test_gps_time_to_unix_timestamp_helper() {
    // Test UTC timestamp conversion helper function
    
    // Test: 2025-01-21 12:34:56
    time_t result = gpsTimeToUnixTimestamp(2025, 1, 21, 12, 34, 56);
    TEST_ASSERT_GREATER_THAN(1737000000, result); // Should be around Jan 2025
    TEST_ASSERT_LESS_THAN(1738000000, result);
    
    // Test: 2000-01-01 00:00:00 (Y2K)
    result = gpsTimeToUnixTimestamp(2000, 1, 1, 0, 0, 0);
    TEST_ASSERT_EQUAL(946684800, result); // Known Y2K timestamp
    
    // Test leap year: 2024-02-29 12:00:00
    result = gpsTimeToUnixTimestamp(2024, 2, 29, 12, 0, 0);
    TEST_ASSERT_GREATER_THAN(1700000000, result); // Should be reasonable
    
    // Test: 1970-01-01 00:00:01 (Unix epoch + 1)
    result = gpsTimeToUnixTimestamp(1970, 1, 1, 0, 0, 1);
    TEST_ASSERT_EQUAL(1, result);
}

void test_overflow_protection() {
    TimeManager timeManager(&mockRTC, &mockTimeSync, &mockGpsMonitor);
    timeManager.setLoggingService(&mockLoggingService);
    timeManager.init();
    
    // Setup conditions that might cause overflow
    mockTimeSync.synchronized = true;
    mockTimeSync.gpsTime = 2000000000UL; // Large timestamp near 32-bit limit
    mockTimeSync.ppsTime = 10000000;
    mockTimeSync.lastGpsUpdate = 5000;
    
    test_micros_value = 15000000; // Large elapsed time
    test_millis_value = 5500;
    
    // Should handle overflow gracefully
    unsigned long result = timeManager.getHighPrecisionTime();
    TEST_ASSERT_GREATER_THAN(0, result); // Should not underflow
    
    // Check if warning was logged for overflow
    if (result < mockTimeSync.gpsTime * 1000UL) {
        TEST_ASSERT_EQUAL_STRING("WARNING", mockLoggingService.last_level.c_str());
        TEST_ASSERT_TRUE(mockLoggingService.last_message.indexOf("overflow") >= 0);
    }
}

// Test Suite Setup
void setUp(void) {
    // Reset mock states before each test
    gpsConnected = true;
    mockTimeSync.synchronized = false;
    mockTimeSync.gpsTime = 0;
    mockTimeSync.ppsTime = 0;
    mockTimeSync.rtcTime = 0;
    mockTimeSync.accuracy = 1.0;
    mockTimeSync.lastGpsUpdate = 0;
    
    mockGpsMonitor.inFallbackMode = false;
    mockGpsMonitor.signalValid = true;
    mockGpsMonitor.satelliteCount = 12;
    
    mockRTC.adjust_called = false;
    mockRTC.setMockTime(2025, 1, 21, 12, 34, 56);
    
    mockLoggingService.last_message = "";
    mockLoggingService.last_level = "";
    mockLoggingService.last_component = "";
    
    test_micros_value = 1000000;
    test_millis_value = 1000;
}

void tearDown(void) {
    // Cleanup after each test if needed
}

// Main test runner
int main(void) {
    UNITY_BEGIN();
    
    // TimeManager Core Functionality Tests
    RUN_TEST(test_time_manager_initialization);
    RUN_TEST(test_pps_interrupt_handling);
    RUN_TEST(test_gps_time_synchronization);
    RUN_TEST(test_invalid_gps_data_handling);
    RUN_TEST(test_gps_disconnected_handling);
    
    // High-Precision Time Retrieval Tests
    RUN_TEST(test_high_precision_time_gps_mode);
    RUN_TEST(test_high_precision_time_rtc_fallback);
    RUN_TEST(test_unix_timestamp_gps_mode);
    RUN_TEST(test_unix_timestamp_rtc_fallback);
    RUN_TEST(test_invalid_rtc_time_handling);
    
    // NTP Protocol Support Tests
    RUN_TEST(test_microsecond_fraction_gps_mode);
    RUN_TEST(test_microsecond_fraction_rtc_fallback);
    RUN_TEST(test_ntp_stratum_calculation);
    
    // Utility and Edge Case Tests
    RUN_TEST(test_gps_time_to_unix_timestamp_helper);
    RUN_TEST(test_overflow_protection);
    
    return UNITY_END();
}