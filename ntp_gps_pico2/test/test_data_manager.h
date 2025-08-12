#pragma once

#include "arduino_mock.h"
#include "../src/system/Result.h"
#include "../src/system/ErrorHandler.h"

/**
 * @file test_data_manager.h
 * @brief Comprehensive test data management system
 * 
 * Provides structured test data management for the new architecture testing,
 * including GPS data, network data, NTP data, and system state data.
 * Supports both individual component testing and integration scenarios.
 */

// ========== GPS Test Data Structures ==========

struct GpsTestData {
    // Basic GPS data
    bool fixAvailable = true;
    double latitude = 35.6762;     // Tokyo coordinates
    double longitude = 139.6503;
    double altitude = 40.0;        // meters
    unsigned long accuracy = 1000; // mm (1 meter)
    
    // Satellite data
    uint8_t satellites = 8;
    uint8_t fixType = 3;           // 3D fix
    float hdop = 1.5f;
    float vdop = 2.0f;
    
    // Timing data
    uint32_t timeOfWeek = 518400;  // Sunday midnight in seconds
    uint16_t year = 2024;
    uint8_t month = 1;
    uint8_t day = 1;
    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t second = 0;
    uint32_t nanoseconds = 500000000; // 0.5 seconds
    
    // Quality indicators
    bool timeValid = true;
    bool dateValid = true;
    bool fullyResolved = true;
    uint8_t confirmDate = 1;
    
    // PPS data
    bool ppsActive = true;
    unsigned long lastPpsTime = 10000;
    
    // QZSS disaster data
    bool dcxActive = false;
    uint8_t dcxType = 0;
    const char* dcxMessage = "";
    
    GpsTestData() = default;
    
    // Create GPS data for different scenarios
    static GpsTestData createNoFix() {
        GpsTestData data;
        data.fixAvailable = false;
        data.fixType = 0;
        data.satellites = 3;
        data.hdop = 99.0f;
        data.vdop = 99.0f;
        data.timeValid = false;
        data.ppsActive = false;
        return data;
    }
    
    static GpsTestData create2DFix() {
        GpsTestData data;
        data.fixType = 2;
        data.satellites = 4;
        data.hdop = 3.0f;
        data.vdop = 4.0f;
        data.altitude = 0.0;
        return data;
    }
    
    static GpsTestData create3DFix() {
        GpsTestData data; // Default is 3D fix
        return data;
    }
    
    static GpsTestData createHighAccuracy() {
        GpsTestData data;
        data.accuracy = 100; // 10cm accuracy
        data.satellites = 12;
        data.hdop = 0.8f;
        data.vdop = 1.2f;
        return data;
    }
    
    static GpsTestData createDcxAlert() {
        GpsTestData data;
        data.dcxActive = true;
        data.dcxType = 1;
        data.dcxMessage = "Test disaster alert message";
        return data;
    }
};

// ========== Network Test Data Structures ==========

struct NetworkTestData {
    // Connection status
    bool connected = true;
    bool dhcpEnabled = true;
    
    // IP configuration
    uint32_t ipAddress = 0xC0A80165;      // 192.168.1.101
    uint32_t subnetMask = 0xFFFFFF00;     // 255.255.255.0
    uint32_t gateway = 0xC0A80101;        // 192.168.1.1
    uint32_t dnsServer = 0x08080808;      // 8.8.8.8
    
    // MAC address
    uint8_t macAddress[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
    
    // Statistics
    unsigned long packetsSent = 1000;
    unsigned long packetsReceived = 950;
    unsigned long packetsDropped = 50;
    unsigned long bytesTransmitted = 100000;
    unsigned long bytesReceived = 95000;
    
    // Timing
    unsigned long connectionTime = 5000;
    unsigned long lastActivity = 1000;
    
    NetworkTestData() = default;
    
    static NetworkTestData createDisconnected() {
        NetworkTestData data;
        data.connected = false;
        data.ipAddress = 0;
        data.packetsSent = 0;
        data.packetsReceived = 0;
        data.connectionTime = 0;
        return data;
    }
    
    static NetworkTestData createStaticIP() {
        NetworkTestData data;
        data.dhcpEnabled = false;
        data.ipAddress = 0xC0A8010A;  // 192.168.1.10
        return data;
    }
    
    static NetworkTestData createHighTraffic() {
        NetworkTestData data;
        data.packetsSent = 100000;
        data.packetsReceived = 98000;
        data.packetsDropped = 2000;
        data.bytesTransmitted = 10000000;
        data.bytesReceived = 9800000;
        return data;
    }
};

// ========== NTP Test Data Structures ==========

struct NtpTestData {
    // NTP packet data
    uint32_t timestamp = 3816211200;      // 2021-01-01 00:00:00
    uint32_t fractionalSeconds = 0x80000000; // 0.5 seconds
    
    // NTP header
    uint8_t leapIndicator = 0;
    uint8_t versionNumber = 4;
    uint8_t mode = 4;                     // Server mode
    uint8_t stratum = 1;                  // Primary server
    int8_t pollInterval = 6;              // 64 seconds
    int8_t precision = -20;               // ~1 microsecond
    
    // Quality indicators
    uint32_t rootDelay = 0;
    uint32_t rootDispersion = 100;
    uint32_t referenceId = 0x47505300;    // "GPS\0"
    
    // Statistics
    unsigned long requestsReceived = 500;
    unsigned long responsesGenerated = 495;
    unsigned long requestsDropped = 5;
    float averageResponseTime = 0.5f;     // milliseconds
    
    // Quality metrics
    float clockOffset = 0.001f;           // 1ms offset
    float clockJitter = 0.0005f;          // 0.5ms jitter
    bool synchronized = true;
    
    NtpTestData() = default;
    
    static NtpTestData createStratum2() {
        NtpTestData data;
        data.stratum = 2;
        data.rootDelay = 1000;             // 1ms
        data.rootDispersion = 500;
        data.referenceId = 0xC0A80101;     // Reference to 192.168.1.1
        return data;
    }
    
    static NtpTestData createUnsynchronized() {
        NtpTestData data;
        data.stratum = 16;                 // Unsynchronized
        data.synchronized = false;
        data.clockOffset = 100.0f;         // Large offset
        data.clockJitter = 50.0f;          // High jitter
        return data;
    }
    
    static NtpTestData createHighLoad() {
        NtpTestData data;
        data.requestsReceived = 10000;
        data.responsesGenerated = 9800;
        data.requestsDropped = 200;
        data.averageResponseTime = 2.5f;
        return data;
    }
};

// ========== System State Test Data ==========

struct SystemStateTestData {
    // Hardware status
    bool gpsReady = true;
    bool networkReady = true;
    bool displayReady = true;
    bool rtcReady = true;
    bool storageReady = true;
    
    // Timing
    unsigned long systemUptime = 3600000; // 1 hour
    unsigned long lastGpsUpdate = 1000;
    unsigned long lastNetworkCheck = 500;
    unsigned long lastDisplayUpdate = 100;
    
    // System health
    float cpuTemperature = 35.5f;
    uint32_t freeMemory = 150000;         // bytes
    uint8_t cpuUsage = 25;                // percent
    
    // Error counters
    unsigned long totalErrors = 5;
    unsigned long gpsErrors = 1;
    unsigned long networkErrors = 2;
    unsigned long systemErrors = 2;
    
    SystemStateTestData() = default;
    
    static SystemStateTestData createHealthy() {
        SystemStateTestData data; // Default is healthy
        return data;
    }
    
    static SystemStateTestData createPartialFailure() {
        SystemStateTestData data;
        data.gpsReady = false;
        data.gpsErrors = 10;
        data.totalErrors = 15;
        data.lastGpsUpdate = 60000; // 1 minute ago
        return data;
    }
    
    static SystemStateTestData createLowMemory() {
        SystemStateTestData data;
        data.freeMemory = 10000;   // Low memory
        data.cpuUsage = 90;        // High CPU
        data.systemErrors = 20;
        return data;
    }
    
    static SystemStateTestData createOverheating() {
        SystemStateTestData data;
        data.cpuTemperature = 85.0f; // High temperature
        data.systemErrors = 50;
        return data;
    }
};

// ========== Test Scenario Definitions ==========

enum class TestScenarioCategory {
    INITIALIZATION,
    NORMAL_OPERATION,
    ERROR_HANDLING,
    RECOVERY,
    PERFORMANCE,
    STRESS_TEST,
    INTEGRATION
};

struct TestScenario {
    const char* name;
    const char* description;
    TestScenarioCategory category;
    bool expectedSuccess;
    ErrorType expectedError;
    unsigned long testDuration;
    
    // Test data for this scenario
    GpsTestData gpsData;
    NetworkTestData networkData;
    NtpTestData ntpData;
    SystemStateTestData systemData;
    
    TestScenario() : name(""), description(""), category(TestScenarioCategory::NORMAL_OPERATION),
                   expectedSuccess(true), expectedError(ErrorType::SYSTEM_ERROR), testDuration(5000) {}
    
    TestScenario(const char* n, const char* d, TestScenarioCategory cat,
                bool success = true, ErrorType error = ErrorType::SYSTEM_ERROR,
                unsigned long duration = 5000)
        : name(n), description(d), category(cat), expectedSuccess(success),
          expectedError(error), testDuration(duration) {}
    
    TestScenario& withGpsData(const GpsTestData& data) {
        gpsData = data;
        return *this;
    }
    
    TestScenario& withNetworkData(const NetworkTestData& data) {
        networkData = data;
        return *this;
    }
    
    TestScenario& withNtpData(const NtpTestData& data) {
        ntpData = data;
        return *this;
    }
    
    TestScenario& withSystemData(const SystemStateTestData& data) {
        systemData = data;
        return *this;
    }
};

// ========== Comprehensive Test Data Manager ==========

class ComprehensiveTestDataManager {
public:
    static ComprehensiveTestDataManager& getInstance() {
        static ComprehensiveTestDataManager instance;
        return instance;
    }
    
    // Get predefined test scenarios
    const TestScenario* getScenarios() const { return scenarios; }
    size_t getScenarioCount() const { return SCENARIO_COUNT; }
    
    // Find scenario by name
    const TestScenario* findScenario(const char* name) const {
        for (size_t i = 0; i < SCENARIO_COUNT; ++i) {
            if (strcmp(scenarios[i].name, name) == 0) {
                return &scenarios[i];
            }
        }
        return nullptr;
    }
    
    // Get scenarios by category
    size_t getScenariosByCategory(TestScenarioCategory category, 
                                 const TestScenario** results, 
                                 size_t maxResults) const {
        size_t count = 0;
        for (size_t i = 0; i < SCENARIO_COUNT && count < maxResults; ++i) {
            if (scenarios[i].category == category) {
                results[count++] = &scenarios[i];
            }
        }
        return count;
    }
    
    // Reset all test data to defaults
    void reset() {
        currentGpsData = GpsTestData{};
        currentNetworkData = NetworkTestData{};
        currentNtpData = NtpTestData{};
        currentSystemData = SystemStateTestData{};
        currentScenarioIndex = 0;
    }
    
    // Current test data (can be modified during tests)
    GpsTestData currentGpsData;
    NetworkTestData currentNetworkData;
    NtpTestData currentNtpData;
    SystemStateTestData currentSystemData;
    size_t currentScenarioIndex = 0;

private:
    ComprehensiveTestDataManager() {
        initializeScenarios();
    }
    
    void initializeScenarios();
    
    static const size_t SCENARIO_COUNT = 20;
    TestScenario scenarios[SCENARIO_COUNT];
};

// ========== Test Data Builder Pattern ==========

class TestDataBuilder {
public:
    TestDataBuilder& withHealthySystem() {
        systemData = SystemStateTestData::createHealthy();
        return *this;
    }
    
    TestDataBuilder& withGpsFix3D() {
        gpsData = GpsTestData::create3DFix();
        return *this;
    }
    
    TestDataBuilder& withNoGpsFix() {
        gpsData = GpsTestData::createNoFix();
        return *this;
    }
    
    TestDataBuilder& withNetworkConnected() {
        networkData = NetworkTestData{};
        return *this;
    }
    
    TestDataBuilder& withNetworkDisconnected() {
        networkData = NetworkTestData::createDisconnected();
        return *this;
    }
    
    TestDataBuilder& withNtpSynchronized() {
        ntpData = NtpTestData{};
        return *this;
    }
    
    TestDataBuilder& withNtpUnsynchronized() {
        ntpData = NtpTestData::createUnsynchronized();
        return *this;
    }
    
    TestScenario build(const char* name, const char* description,
                      bool expectedSuccess = true,
                      ErrorType expectedError = ErrorType::SYSTEM_ERROR) {
        TestScenario scenario(name, description, TestScenarioCategory::NORMAL_OPERATION,
                            expectedSuccess, expectedError);
        return scenario.withGpsData(gpsData)
                      .withNetworkData(networkData)
                      .withNtpData(ntpData)
                      .withSystemData(systemData);
    }
    
private:
    GpsTestData gpsData;
    NetworkTestData networkData;
    NtpTestData ntpData;
    SystemStateTestData systemData;
};

// ========== Test Utilities ==========

class TestDataUtilities {
public:
    // Create GPS data with specific accuracy
    static GpsTestData createGpsDataWithAccuracy(unsigned long accuracyMm) {
        auto data = GpsTestData::create3DFix();
        data.accuracy = accuracyMm;
        if (accuracyMm <= 500) {
            data.satellites = 12;
            data.hdop = 0.8f;
        } else if (accuracyMm <= 2000) {
            data.satellites = 8;
            data.hdop = 1.5f;
        } else {
            data.satellites = 5;
            data.hdop = 3.0f;
        }
        return data;
    }
    
    // Create network data with specific packet loss
    static NetworkTestData createNetworkDataWithPacketLoss(float lossPercent) {
        NetworkTestData data;
        data.packetsSent = 1000;
        data.packetsReceived = (unsigned long)(1000 * (1.0f - lossPercent / 100.0f));
        data.packetsDropped = data.packetsSent - data.packetsReceived;
        return data;
    }
    
    // Create NTP data with specific stratum
    static NtpTestData createNtpDataWithStratum(uint8_t stratum) {
        NtpTestData data;
        data.stratum = stratum;
        if (stratum == 1) {
            data.rootDelay = 0;
            data.rootDispersion = 100;
            data.referenceId = 0x47505300; // GPS
        } else if (stratum <= 15) {
            data.rootDelay = stratum * 100;
            data.rootDispersion = stratum * 200;
            data.referenceId = 0xC0A80101;
        } else {
            data.synchronized = false;
            data.rootDelay = 65535;
            data.rootDispersion = 65535;
        }
        return data;
    }
    
    // Simulate time passage
    static void advanceTime(unsigned long milliseconds) {
        mock_millis_counter += milliseconds;
        mock_micros_counter += milliseconds * 1000;
    }
    
    // Create Result with test data
    template<typename T>
    static Result<T, ErrorType> createTestResult(const T& value, bool success = true,
                                                ErrorType error = ErrorType::SYSTEM_ERROR) {
        if (success) {
            return Result<T, ErrorType>::ok(value);
        } else {
            return Result<T, ErrorType>::error(error);
        }
    }
};

// Global test data instance
extern ComprehensiveTestDataManager* g_testDataManager;

// Test data manager initialization
inline void initializeTestDataManager() {
    g_testDataManager = &ComprehensiveTestDataManager::getInstance();
}