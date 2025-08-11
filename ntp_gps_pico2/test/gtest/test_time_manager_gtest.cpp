#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <memory>

// Advanced TimeManager implementation for extended testing
struct TimeData {
    uint64_t unix_timestamp;
    uint32_t microseconds;
    uint64_t ntp_timestamp;
    bool time_valid;
    uint8_t time_source;  // 0=GPS, 1=RTC, 2=NTP, 3=SYSTEM
    float accuracy_ms;
    uint32_t last_sync_time;
    uint32_t sync_interval;
};

struct GPSTimeInfo {
    bool fix_available;
    uint8_t satellites_used;
    uint64_t gps_timestamp;
    uint32_t time_of_week;
    uint16_t week_number;
    bool leap_second_pending;
    int8_t leap_second_offset;
    float time_accuracy_ns;
    bool pps_synchronized;
    uint64_t last_pps_timestamp;
    uint32_t pps_count;
    float pps_jitter_us;
};

struct RTCTimeInfo {
    bool rtc_available;
    uint64_t rtc_timestamp;
    float temperature;
    bool battery_good;
    uint32_t drift_ppm;
    uint32_t last_calibration;
    bool time_lost;
};

// Mock interfaces
class MockGPSInterface {
public:
    MOCK_METHOD(bool, getTimeInfo, (GPSTimeInfo& info));
};

class MockRTCInterface {
public:
    MOCK_METHOD(bool, getTimeInfo, (RTCTimeInfo& info));
    MOCK_METHOD(bool, setTime, (uint64_t timestamp));
};

// Concrete mock implementations for complex scenarios
class ConcreteMockGPSInterface {
public:
    GPSTimeInfo gps_info;
    bool simulation_mode = false;
    uint32_t simulated_time_base = 1640995200;
    int error_rate = 0;
    
    ConcreteMockGPSInterface() {
        reset();
    }
    
    bool getTimeInfo(GPSTimeInfo& info) {
        if (error_rate > 0 && (std::rand() % 100) < error_rate) {
            return false;
        }
        
        if (simulation_mode) {
            simulateGPSTime();
        }
        
        info = gps_info;
        return gps_info.fix_available;
    }
    
    void simulateGPSTime() {
        static uint32_t sim_counter = 0;
        sim_counter++;
        
        gps_info.fix_available = true;
        gps_info.satellites_used = 8;
        gps_info.gps_timestamp = simulated_time_base + sim_counter;
        gps_info.time_of_week = (sim_counter % (7 * 24 * 3600));
        gps_info.week_number = 2000 + (sim_counter / (7 * 24 * 3600));
        gps_info.leap_second_pending = false;
        gps_info.leap_second_offset = 18;
        gps_info.time_accuracy_ns = 50.0f;
        gps_info.pps_synchronized = true;
        gps_info.last_pps_timestamp = (simulated_time_base + sim_counter) * 1000000ULL;
        gps_info.pps_count = sim_counter;
        gps_info.pps_jitter_us = 0.1f;
    }
    
    void reset() {
        std::memset(&gps_info, 0, sizeof(gps_info));
        gps_info.leap_second_offset = 18;
        simulation_mode = false;
        error_rate = 0;
    }
    
    void setFixAvailable(bool available) {
        gps_info.fix_available = available;
    }
    
    void setPPSSynchronized(bool sync) {
        gps_info.pps_synchronized = sync;
    }
    
    void setErrorRate(int percentage) {
        error_rate = (percentage > 100) ? 100 : percentage;
    }
};

class ConcreteMockRTCInterface {
public:
    RTCTimeInfo rtc_info;
    bool simulation_mode = false;
    uint32_t simulated_time_base = 1640995200;
    int error_rate = 0;
    
    ConcreteMockRTCInterface() {
        reset();
    }
    
    bool getTimeInfo(RTCTimeInfo& info) {
        if (error_rate > 0 && (std::rand() % 100) < error_rate) {
            return false;
        }
        
        if (simulation_mode) {
            simulateRTCTime();
        }
        
        info = rtc_info;
        return rtc_info.rtc_available;
    }
    
    bool setTime(uint64_t timestamp) {
        if (error_rate > 0 && (std::rand() % 100) < error_rate) {
            return false;
        }
        
        rtc_info.rtc_timestamp = timestamp;
        rtc_info.time_lost = false;
        return rtc_info.rtc_available;
    }
    
    void simulateRTCTime() {
        static uint32_t sim_counter = 0;
        sim_counter++;
        
        rtc_info.rtc_available = true;
        rtc_info.rtc_timestamp = simulated_time_base + sim_counter + 2;
        rtc_info.temperature = 25.0f + (sim_counter % 20) - 10;
        rtc_info.battery_good = true;
        rtc_info.drift_ppm = 20;
        rtc_info.last_calibration = simulated_time_base;
        rtc_info.time_lost = false;
    }
    
    void reset() {
        std::memset(&rtc_info, 0, sizeof(rtc_info));
        rtc_info.battery_good = true;
        rtc_info.drift_ppm = 20;
        simulation_mode = false;
        error_rate = 0;
    }
    
    void setAvailable(bool available) {
        rtc_info.rtc_available = available;
    }
    
    void setBatteryGood(bool good) {
        rtc_info.battery_good = good;
    }
    
    void setTimeLost(bool lost) {
        rtc_info.time_lost = lost;
    }
    
    void setErrorRate(int percentage) {
        error_rate = (percentage > 100) ? 100 : percentage;
    }
};

// Extended TimeManager with advanced features
class ExtendedTimeManager {
private:
    ConcreteMockGPSInterface* gps;
    ConcreteMockRTCInterface* rtc;
    TimeData current_time;
    
    uint32_t last_gps_sync = 0;
    uint32_t last_rtc_sync = 0;
    uint32_t sync_failures = 0;
    bool disciplined_clock = false;
    float clock_offset = 0.0f;
    float clock_drift = 0.0f;
    uint32_t discipline_window = 300;
    
    float time_uncertainty = 1000.0f;
    uint32_t consecutive_good_syncs = 0;
    uint32_t max_uncertainty = 5000;
    
    bool leap_second_scheduled = false;
    uint64_t leap_second_time = 0;
    int8_t leap_second_direction = 0;
    
    int16_t timezone_offset_minutes = 0;
    bool dst_active = false;
    char time_format[20] = "%Y-%m-%d %H:%M:%S";
    
public:
    ExtendedTimeManager(ConcreteMockGPSInterface* gps_interface, ConcreteMockRTCInterface* rtc_interface) 
        : gps(gps_interface), rtc(rtc_interface) {
        std::memset(&current_time, 0, sizeof(current_time));
        current_time.sync_interval = 60;
        current_time.time_source = 3;
    }
    
    uint64_t getSystemTime() {
        static uint64_t base_time = 1640995200;
        static uint32_t counter = 0;
        return base_time + (++counter);
    }
    
    uint32_t getSystemMicroseconds() {
        static uint32_t micro_counter = 0;
        return (micro_counter += 1234) % 1000000;
    }
    
    bool initialize() {
        if (synchronizeWithGPS()) {
            return true;
        } else if (synchronizeWithRTC()) {
            return true;
        } else {
            current_time.unix_timestamp = getSystemTime();
            current_time.microseconds = getSystemMicroseconds();
            current_time.time_valid = false;
            current_time.time_source = 3;
            current_time.accuracy_ms = 1000.0f;
            return false;
        }
    }
    
    bool synchronizeWithGPS() {
        GPSTimeInfo gps_info;
        if (!gps->getTimeInfo(gps_info) || !gps_info.fix_available) {
            return false;
        }
        
        uint64_t gps_unix_time = convertGPSToUnix(gps_info.gps_timestamp, gps_info.leap_second_offset);
        
        if (disciplined_clock && consecutive_good_syncs > 10) {
            float predicted_offset = clock_drift * (getSystemTime() - last_gps_sync);
            gps_unix_time += static_cast<uint64_t>(predicted_offset);
        }
        
        current_time.unix_timestamp = gps_unix_time;
        current_time.microseconds = 0;
        current_time.ntp_timestamp = convertUnixToNTP(gps_unix_time, 0);
        current_time.time_valid = true;
        current_time.time_source = 0;
        current_time.accuracy_ms = gps_info.time_accuracy_ns / 1000000.0f;
        current_time.last_sync_time = getSystemTime();
        
        if (last_gps_sync > 0) {
            float time_diff = static_cast<float>(gps_unix_time - last_gps_sync);
            float system_diff = static_cast<float>(getSystemTime() - last_gps_sync);
            clock_offset = time_diff - system_diff;
            
            if (consecutive_good_syncs > 5) {
                clock_drift = clock_offset / system_diff;
                disciplined_clock = true;
            }
        }
        
        last_gps_sync = getSystemTime();
        consecutive_good_syncs++;
        sync_failures = 0;
        
        if (gps_info.pps_synchronized) {
            time_uncertainty = gps_info.time_accuracy_ns / 1000000.0f;
        } else {
            time_uncertainty = 100.0f;
        }
        
        rtc->setTime(gps_unix_time);
        
        return true;
    }
    
    bool synchronizeWithRTC() {
        RTCTimeInfo rtc_info;
        if (!rtc->getTimeInfo(rtc_info) || !rtc_info.rtc_available || rtc_info.time_lost) {
            return false;
        }
        
        uint64_t compensated_time = rtc_info.rtc_timestamp;
        if (rtc_info.temperature != 0.0f) {
            float temp_error = (rtc_info.temperature - 25.0f) * -0.04f;
            uint32_t time_since_cal = getSystemTime() - rtc_info.last_calibration;
            float compensation = temp_error * time_since_cal / 1000000.0f;
            compensated_time += static_cast<uint64_t>(compensation);
        }
        
        current_time.unix_timestamp = compensated_time;
        current_time.microseconds = 0;
        current_time.ntp_timestamp = convertUnixToNTP(compensated_time, 0);
        current_time.time_valid = rtc_info.battery_good;
        current_time.time_source = 1;
        current_time.accuracy_ms = calculateRTCAccuracy(rtc_info);
        current_time.last_sync_time = getSystemTime();
        
        last_rtc_sync = getSystemTime();
        time_uncertainty = current_time.accuracy_ms;
        
        return true;
    }
    
    void update() {
        uint32_t current_system_time = getSystemTime();
        
        if (current_system_time - current_time.last_sync_time >= current_time.sync_interval) {
            if (!synchronizeWithGPS()) {
                if (!synchronizeWithRTC()) {
                    sync_failures++;
                    time_uncertainty *= 1.1f;
                    if (time_uncertainty > max_uncertainty) {
                        current_time.time_valid = false;
                    }
                }
            }
        }
        
        uint32_t time_elapsed = current_system_time - current_time.last_sync_time;
        current_time.unix_timestamp += time_elapsed;
        current_time.microseconds = getSystemMicroseconds();
        current_time.ntp_timestamp = convertUnixToNTP(current_time.unix_timestamp, current_time.microseconds);
        
        if (leap_second_scheduled && current_time.unix_timestamp >= leap_second_time) {
            handleLeapSecond();
        }
        
        updateTimeQuality();
    }
    
    // Time conversion utilities
    uint64_t convertUnixToNTP(uint64_t unix_time, uint32_t microseconds) {
        const uint64_t NTP_EPOCH_OFFSET = 2208988800ULL;
        uint64_t ntp_seconds = unix_time + NTP_EPOCH_OFFSET;
        uint64_t ntp_fraction = (static_cast<uint64_t>(microseconds) << 32) / 1000000ULL;
        return (ntp_seconds << 32) | ntp_fraction;
    }
    
    uint64_t convertNTPToUnix(uint64_t ntp_time) {
        const uint64_t NTP_EPOCH_OFFSET = 2208988800ULL;
        return (ntp_time >> 32) - NTP_EPOCH_OFFSET;
    }
    
    uint64_t convertGPSToUnix(uint64_t gps_time, int8_t leap_seconds) {
        const uint64_t GPS_EPOCH_OFFSET = 315964800ULL;
        return gps_time + GPS_EPOCH_OFFSET - leap_seconds;
    }
    
    // Configuration methods
    void setSyncInterval(uint32_t interval_seconds) {
        if (interval_seconds > 0) {
            current_time.sync_interval = interval_seconds;
        }
    }
    
    void setTimezone(int16_t offset_minutes) {
        timezone_offset_minutes = offset_minutes;
    }
    
    void setDST(bool active) {
        dst_active = active;
    }
    
    void setTimeFormat(const char* format) {
        if (format && std::strlen(format) < sizeof(time_format)) {
            std::strcpy(time_format, format);
        }
    }
    
    void scheduleLeapSecond(uint64_t leap_time, int8_t direction) {
        leap_second_scheduled = true;
        leap_second_time = leap_time;
        leap_second_direction = direction;
    }
    
    // Getters
    TimeData getCurrentTime() const {
        return current_time;
    }
    
    uint64_t getUnixTimestamp() const {
        return current_time.unix_timestamp;
    }
    
    uint64_t getNTPTimestamp() const {
        return current_time.ntp_timestamp;
    }
    
    uint32_t getMicroseconds() const {
        return current_time.microseconds;
    }
    
    bool isTimeValid() const {
        return current_time.time_valid;
    }
    
    uint8_t getTimeSource() const {
        return current_time.time_source;
    }
    
    float getAccuracy() const {
        return current_time.accuracy_ms;
    }
    
    float getTimeUncertainty() const {
        return time_uncertainty;
    }
    
    uint32_t getSyncFailures() const {
        return sync_failures;
    }
    
    bool isDisciplinedClock() const {
        return disciplined_clock;
    }
    
    float getClockOffset() const {
        return clock_offset;
    }
    
    float getClockDrift() const {
        return clock_drift;
    }
    
    bool isLeapSecondScheduled() const {
        return leap_second_scheduled;
    }
    
    uint32_t getConsecutiveGoodSyncs() const {
        return consecutive_good_syncs;
    }
    
    void formatTime(char* buffer, size_t buffer_size, bool local_time = false) {
        if (!buffer || buffer_size < 20) return;
        
        uint64_t display_time = current_time.unix_timestamp;
        
        if (local_time) {
            display_time += timezone_offset_minutes * 60;
            if (dst_active) {
                display_time += 3600;
            }
        }
        
        uint32_t days = display_time / (24 * 3600);
        uint32_t seconds_today = display_time % (24 * 3600);
        uint32_t hours = seconds_today / 3600;
        uint32_t minutes = (seconds_today % 3600) / 60;
        uint32_t seconds = seconds_today % 60;
        
        uint32_t year = 1970 + days / 365;
        uint32_t month = (days % 365) / 30 + 1;
        uint32_t day = (days % 365) % 30 + 1;
        
        std::snprintf(buffer, buffer_size, "%04d-%02d-%02d %02d:%02d:%02d", 
                 year, month, day, hours, minutes, seconds);
    }
    
    enum TimeQuality {
        EXCELLENT = 0,
        GOOD = 1,
        FAIR = 2,
        POOR = 3,
        INVALID = 4
    };
    
    TimeQuality getTimeQuality() const {
        if (!current_time.time_valid) {
            return INVALID;
        }
        
        if (current_time.time_source == 0) { // GPS
            if (time_uncertainty < 1.0f) {
                return EXCELLENT;
            } else if (time_uncertainty < 100.0f) {
                return GOOD;
            }
        }
        
        if (current_time.accuracy_ms < 1000.0f) {
            return FAIR;
        }
        
        return POOR;
    }
    
private:
    float calculateRTCAccuracy(const RTCTimeInfo& rtc_info) {
        float base_accuracy = 1000.0f;
        
        uint32_t time_since_sync = getSystemTime() - last_gps_sync;
        if (last_gps_sync > 0) {
            float drift_error = (rtc_info.drift_ppm / 1000000.0f) * time_since_sync * 1000.0f;
            base_accuracy += drift_error;
        }
        
        if (std::abs(rtc_info.temperature - 25.0f) > 10.0f) {
            base_accuracy *= 1.5f;
        }
        
        return base_accuracy;
    }
    
    void handleLeapSecond() {
        if (leap_second_direction > 0) {
            current_time.unix_timestamp += 1;
        }
        
        leap_second_scheduled = false;
        rtc->setTime(current_time.unix_timestamp);
    }
    
    void updateTimeQuality() {
        uint32_t time_since_sync = getSystemTime() - current_time.last_sync_time;
        
        if (time_since_sync > 3600) {
            consecutive_good_syncs = 0;
            disciplined_clock = false;
        }
        
        if (time_since_sync > 86400) {
            current_time.time_valid = false;
        }
    }
};

// Test fixture class
class TimeManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockGPS = std::make_unique<ConcreteMockGPSInterface>();
        mockRTC = std::make_unique<ConcreteMockRTCInterface>();
        timeManager = std::make_unique<ExtendedTimeManager>(mockGPS.get(), mockRTC.get());
    }
    
    void TearDown() override {
        timeManager.reset();
        mockRTC.reset();
        mockGPS.reset();
    }
    
    std::unique_ptr<ConcreteMockGPSInterface> mockGPS;
    std::unique_ptr<ConcreteMockRTCInterface> mockRTC;
    std::unique_ptr<ExtendedTimeManager> timeManager;
};

// Basic Time Manager Tests
TEST_F(TimeManagerTest, InitializationWithGPS) {
    mockGPS->setFixAvailable(true);
    mockGPS->simulation_mode = true;
    
    EXPECT_TRUE(timeManager->initialize());
    EXPECT_TRUE(timeManager->isTimeValid());
    EXPECT_EQ(0, timeManager->getTimeSource()); // GPS
}

TEST_F(TimeManagerTest, InitializationWithRTC) {
    mockGPS->setFixAvailable(false);
    mockRTC->setAvailable(true);
    mockRTC->simulation_mode = true;
    
    EXPECT_TRUE(timeManager->initialize());
    EXPECT_TRUE(timeManager->isTimeValid());
    EXPECT_EQ(1, timeManager->getTimeSource()); // RTC
}

TEST_F(TimeManagerTest, InitializationNoSource) {
    mockGPS->setFixAvailable(false);
    mockRTC->setAvailable(false);
    
    EXPECT_FALSE(timeManager->initialize());
    EXPECT_FALSE(timeManager->isTimeValid());
    EXPECT_EQ(3, timeManager->getTimeSource()); // SYSTEM
}

// GPS Synchronization Tests
TEST_F(TimeManagerTest, GPSSynchronization) {
    mockGPS->setFixAvailable(true);
    mockGPS->simulation_mode = true;
    
    EXPECT_TRUE(timeManager->synchronizeWithGPS());
    EXPECT_EQ(0, timeManager->getTimeSource());
    EXPECT_LT(timeManager->getAccuracy(), 1.0f);
}

TEST_F(TimeManagerTest, GPSSyncFailure) {
    mockGPS->setFixAvailable(false);
    
    EXPECT_FALSE(timeManager->synchronizeWithGPS());
}

TEST_F(TimeManagerTest, GPSWithPPS) {
    mockGPS->setFixAvailable(true);
    mockGPS->setPPSSynchronized(true);
    mockGPS->simulation_mode = true;
    
    EXPECT_TRUE(timeManager->synchronizeWithGPS());
    EXPECT_LT(timeManager->getAccuracy(), 0.1f);
}

// RTC Synchronization Tests
TEST_F(TimeManagerTest, RTCSynchronization) {
    mockRTC->setAvailable(true);
    mockRTC->simulation_mode = true;
    
    EXPECT_TRUE(timeManager->synchronizeWithRTC());
    EXPECT_EQ(1, timeManager->getTimeSource());
}

TEST_F(TimeManagerTest, RTCSyncFailure) {
    mockRTC->setAvailable(false);
    
    EXPECT_FALSE(timeManager->synchronizeWithRTC());
}

TEST_F(TimeManagerTest, RTCBatteryFailure) {
    mockRTC->setAvailable(true);
    mockRTC->setBatteryGood(false);
    mockRTC->setTimeLost(true);
    
    EXPECT_FALSE(timeManager->synchronizeWithRTC());
}

// Time Conversion Tests
TEST_F(TimeManagerTest, UnixToNTPConversion) {
    uint64_t unix_time = 1640995200; // 2022-01-01 00:00:00 UTC
    uint32_t microseconds = 500000;  // 0.5 seconds
    
    uint64_t ntp_time = timeManager->convertUnixToNTP(unix_time, microseconds);
    
    uint64_t expected_seconds = unix_time + 2208988800ULL;
    uint64_t actual_seconds = ntp_time >> 32;
    EXPECT_EQ(expected_seconds, actual_seconds);
    
    uint32_t actual_fraction = ntp_time & 0xFFFFFFFF;
    uint32_t expected_fraction = (static_cast<uint64_t>(microseconds) << 32) / 1000000ULL;
    EXPECT_EQ(expected_fraction, actual_fraction);
}

TEST_F(TimeManagerTest, NTPToUnixConversion) {
    uint64_t ntp_time = (3849283200ULL << 32) | 0x80000000;
    
    uint64_t unix_time = timeManager->convertNTPToUnix(ntp_time);
    
    EXPECT_EQ(1640995200U, unix_time);
}

TEST_F(TimeManagerTest, GPSToUnixConversion) {
    uint64_t gps_time = 1325376000;
    int8_t leap_seconds = 18;
    
    uint64_t unix_time = timeManager->convertGPSToUnix(gps_time, leap_seconds);
    
    EXPECT_GT(unix_time, 1640000000U);
    EXPECT_LT(unix_time, 1641000000U);
}

// Clock Discipline Tests
TEST_F(TimeManagerTest, ClockDiscipline) {
    mockGPS->setFixAvailable(true);
    mockGPS->simulation_mode = true;
    
    timeManager->initialize();
    
    for (int i = 0; i < 12; i++) {
        timeManager->synchronizeWithGPS();
    }
    
    EXPECT_TRUE(timeManager->isDisciplinedClock());
    EXPECT_GE(timeManager->getConsecutiveGoodSyncs(), 10U);
}

TEST_F(TimeManagerTest, SyncIntervalSetting) {
    timeManager->setSyncInterval(300);
    
    TimeData time_data = timeManager->getCurrentTime();
    EXPECT_EQ(300U, time_data.sync_interval);
}

// Error Handling Tests
TEST_F(TimeManagerTest, GPSErrorHandling) {
    mockGPS->setFixAvailable(true);
    mockGPS->setErrorRate(50);
    
    int success_count = 0;
    int failure_count = 0;
    
    for (int i = 0; i < 20; i++) {
        if (timeManager->synchronizeWithGPS()) {
            success_count++;
        } else {
            failure_count++;
        }
    }
    
    EXPECT_GT(success_count, 0);
    EXPECT_GT(failure_count, 0);
    EXPECT_GT(timeManager->getSyncFailures(), 0U);
}

TEST_F(TimeManagerTest, RTCErrorHandling) {
    mockRTC->setAvailable(true);
    mockRTC->setErrorRate(30);
    
    int success_count = 0;
    int failure_count = 0;
    
    for (int i = 0; i < 20; i++) {
        if (timeManager->synchronizeWithRTC()) {
            success_count++;
        } else {
            failure_count++;
        }
    }
    
    EXPECT_GT(success_count, 0);
    EXPECT_GT(failure_count, 0);
}

// Time Quality Assessment Tests
TEST_F(TimeManagerTest, TimeQualityExcellent) {
    mockGPS->setFixAvailable(true);
    mockGPS->setPPSSynchronized(true);
    mockGPS->simulation_mode = true;
    
    timeManager->synchronizeWithGPS();
    
    ExtendedTimeManager::TimeQuality quality = timeManager->getTimeQuality();
    EXPECT_EQ(ExtendedTimeManager::EXCELLENT, quality);
}

TEST_F(TimeManagerTest, TimeQualityGood) {
    mockGPS->setFixAvailable(true);
    mockGPS->setPPSSynchronized(false);
    mockGPS->simulation_mode = true;
    
    timeManager->synchronizeWithGPS();
    
    ExtendedTimeManager::TimeQuality quality = timeManager->getTimeQuality();
    EXPECT_LE(quality, ExtendedTimeManager::GOOD);
}

TEST_F(TimeManagerTest, TimeQualityInvalid) {
    mockGPS->setFixAvailable(false);
    mockRTC->setAvailable(false);
    
    timeManager->initialize();
    
    ExtendedTimeManager::TimeQuality quality = timeManager->getTimeQuality();
    EXPECT_EQ(ExtendedTimeManager::INVALID, quality);
}

// Advanced Features Tests
TEST_F(TimeManagerTest, TimezoneSetting) {
    timeManager->setTimezone(540); // +9 hours (JST)
    
    mockGPS->setFixAvailable(true);
    mockGPS->simulation_mode = true;
    timeManager->synchronizeWithGPS();
    
    char utc_time[32], local_time[32];
    timeManager->formatTime(utc_time, sizeof(utc_time), false);
    timeManager->formatTime(local_time, sizeof(local_time), true);
    
    EXPECT_STRNE(utc_time, local_time);
}

TEST_F(TimeManagerTest, DSTSetting) {
    timeManager->setTimezone(480); // +8 hours
    timeManager->setDST(true);     // Enable DST
    
    mockGPS->setFixAvailable(true);
    mockGPS->simulation_mode = true;
    timeManager->synchronizeWithGPS();
    
    char time_buffer[32];
    timeManager->formatTime(time_buffer, sizeof(time_buffer), true);
    
    EXPECT_NE(nullptr, time_buffer);
    EXPECT_GT(std::strlen(time_buffer), 0U);
}

TEST_F(TimeManagerTest, LeapSecondScheduling) {
    uint64_t leap_time = 1640995200 + 3600;
    timeManager->scheduleLeapSecond(leap_time, 1);
    
    EXPECT_TRUE(timeManager->isLeapSecondScheduled());
}

TEST_F(TimeManagerTest, TimeFormatting) {
    mockGPS->setFixAvailable(true);
    mockGPS->simulation_mode = true;
    timeManager->synchronizeWithGPS();
    
    char time_str[32];
    timeManager->formatTime(time_str, sizeof(time_str), false);
    
    EXPECT_GE(std::strlen(time_str), 19U);
    EXPECT_NE(nullptr, std::strchr(time_str, '-'));
    EXPECT_NE(nullptr, std::strchr(time_str, ':'));
}

// Update and Maintenance Tests
TEST_F(TimeManagerTest, PeriodicUpdate) {
    mockGPS->setFixAvailable(true);
    mockGPS->simulation_mode = true;
    
    timeManager->initialize();
    timeManager->setSyncInterval(1);
    
    uint64_t initial_timestamp = timeManager->getUnixTimestamp();
    
    for (int i = 0; i < 10; i++) {
        timeManager->update();
    }
    
    uint64_t final_timestamp = timeManager->getUnixTimestamp();
    
    EXPECT_GE(final_timestamp, initial_timestamp);
}

TEST_F(TimeManagerTest, TimeUncertaintyIncrease) {
    mockGPS->setFixAvailable(true);
    mockGPS->simulation_mode = true;
    
    timeManager->synchronizeWithGPS();
    float initial_uncertainty = timeManager->getTimeUncertainty();
    
    mockGPS->setFixAvailable(false);
    
    for (int i = 0; i < 10; i++) {
        timeManager->update();
    }
    
    float final_uncertainty = timeManager->getTimeUncertainty();
    EXPECT_GE(final_uncertainty, initial_uncertainty);
}

// Parameterized Tests
class TimeSourceTest : public TimeManagerTest, 
                      public ::testing::WithParamInterface<std::tuple<bool, bool, uint8_t>> {};

TEST_P(TimeSourceTest, TimeSourcePriority) {
    auto [gps_available, rtc_available, expected_source] = GetParam();
    
    mockGPS->setFixAvailable(gps_available);
    mockGPS->simulation_mode = gps_available;
    mockRTC->setAvailable(rtc_available);
    mockRTC->simulation_mode = rtc_available;
    
    timeManager->initialize();
    
    if (gps_available || rtc_available) {
        EXPECT_TRUE(timeManager->isTimeValid());
        EXPECT_EQ(expected_source, timeManager->getTimeSource());
    } else {
        EXPECT_FALSE(timeManager->isTimeValid());
        EXPECT_EQ(3, timeManager->getTimeSource()); // SYSTEM
    }
}

INSTANTIATE_TEST_SUITE_P(
    TimeSourcePriority,
    TimeSourceTest,
    ::testing::Values(
        std::make_tuple(true, true, 0),   // GPS available -> GPS (0)
        std::make_tuple(true, false, 0),  // GPS only -> GPS (0)
        std::make_tuple(false, true, 1),  // RTC only -> RTC (1)
        std::make_tuple(false, false, 3)  // None -> SYSTEM (3)
    )
);

// Integration Tests
TEST_F(TimeManagerTest, GPSRTCFailover) {
    mockGPS->setFixAvailable(true);
    mockGPS->simulation_mode = true;
    mockRTC->setAvailable(true);
    mockRTC->simulation_mode = true;
    
    timeManager->initialize();
    EXPECT_EQ(0, timeManager->getTimeSource()); // Should use GPS
    
    mockGPS->setFixAvailable(false);
    
    timeManager->update();
    timeManager->setSyncInterval(1);
    timeManager->update();
    
    EXPECT_TRUE(timeManager->isTimeValid() || timeManager->getSyncFailures() > 0);
}

TEST_F(TimeManagerTest, CompleteFailureRecovery) {
    mockGPS->setFixAvailable(false);
    mockRTC->setAvailable(false);
    
    EXPECT_FALSE(timeManager->initialize());
    
    mockGPS->setFixAvailable(true);
    mockGPS->simulation_mode = true;
    
    timeManager->update();
    timeManager->setSyncInterval(1);
    timeManager->update();
    
    for (int i = 0; i < 5; i++) {
        timeManager->update();
        if (timeManager->isTimeValid()) break;
    }
    
    EXPECT_TRUE(timeManager->isTimeValid() || timeManager->getSyncFailures() < 5);
}