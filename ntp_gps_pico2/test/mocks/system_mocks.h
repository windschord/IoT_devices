#pragma once

#include "../arduino_mock.h"
#include "../../src/system/Result.h"
#include "../../src/system/ErrorHandler.h"
#include "../../src/interfaces/IService.h"
#include "../../src/interfaces/IHardwareInterface.h"

// Forward declarations to avoid circular dependencies
class SystemState;
class ServiceContainer;
class SystemInitializer;
class MainLoop;

/**
 * @file system_mocks.h
 * @brief Mock classes for the new system architecture (SystemInitializer, MainLoop, SystemState, DI)
 * 
 * This file provides mock implementations for testing the refactored system architecture
 * including dependency injection, Result types, and modular service management.
 */

// ========== Mock Service Interface ==========

class MockService : public IService {
public:
    mutable bool initializeCalled = false;
    mutable bool startCalled = false;
    mutable bool stopCalled = false;
    mutable bool resetCalled = false;
    mutable bool isInitialized_ = false;
    mutable bool isRunning_ = false;
    mutable ErrorType lastError = ErrorType::NONE;
    mutable bool shouldFailInitialize = false;
    mutable bool shouldFailStart = false;

    bool initialize() override {
        initializeCalled = true;
        if (shouldFailInitialize) {
            lastError = ErrorType::SYSTEM_ERROR;
            return false;
        }
        isInitialized_ = true;
        return true;
    }

    bool start() override {
        startCalled = true;
        if (shouldFailStart) {
            lastError = ErrorType::SYSTEM_ERROR;
            return false;
        }
        isRunning_ = true;
        return true;
    }

    void stop() override {
        stopCalled = true;
        isRunning_ = false;
    }

    void reset() override {
        resetCalled = true;
        isInitialized_ = false;
        isRunning_ = false;
        lastError = ErrorType::NONE;
    }

    bool isRunning() const override { return isRunning_; }
    const char* getName() const override { return "MockService"; }
};

// ========== Mock Hardware Interface ==========

class MockHardwareInterface : public IHardwareInterface {
public:
    mutable bool initializeCalled = false;
    mutable bool resetCalled = false;
    mutable bool isReady_ = false;
    mutable const char* lastError = nullptr;
    mutable bool shouldFailInitialize = false;

    bool initialize() override {
        initializeCalled = true;
        if (shouldFailInitialize) {
            lastError = "Mock initialization failure";
            return false;
        }
        isReady_ = true;
        return true;
    }

    bool reset() override {
        resetCalled = true;
        isReady_ = false;
        lastError = nullptr;
        return true;
    }

    bool isReady() const override { return isReady_; }
    const char* getLastError() const override { return lastError; }
    const char* getHardwareName() const override { return "MockHardware"; }
};

// ========== Mock System State ==========

class MockSystemState {
public:
    mutable bool hardwareStatusUpdated = false;
    mutable bool statisticsUpdated = false;
    mutable unsigned long mockUptime = 12345;
    mutable unsigned long mockNtpRequests = 100;
    mutable float mockAccuracy = 0.5f;

    static MockSystemState& getInstance() {
        static MockSystemState instance;
        return instance;
    }

    // Mock hardware status
    struct MockHardwareStatus {
        bool gpsReady = true;
        bool networkReady = true;
        bool displayReady = true;
        bool rtcReady = true;
        bool storageReady = true;
        unsigned long lastGpsUpdate = 10000;
        unsigned long lastNetworkCheck = 5000;
        float cpuTemperature = 25.5f;
        uint32_t freeMemory = 200000;
    };

    // Mock system statistics
    struct MockSystemStatistics {
        unsigned long systemUptime = 12345;
        unsigned long ntpRequestsTotal = 100;
        unsigned long ntpResponsesTotal = 95;
        unsigned long ntpDroppedTotal = 5;
        unsigned long gpsFixCount = 50;
        unsigned long ppsCount = 1000;
        unsigned long errorCount = 2;
        unsigned long restartCount = 1;
        float averageResponseTime = 1.5f;
        float currentAccuracy = 0.5f;
    };

    MockHardwareStatus hardwareStatus;
    MockSystemStatistics systemStatistics;

    MockHardwareStatus& getHardwareStatus() { 
        hardwareStatusUpdated = true;
        return hardwareStatus; 
    }
    
    const MockHardwareStatus& getHardwareStatus() const { 
        return hardwareStatus; 
    }

    MockSystemStatistics& getSystemStatistics() { 
        statisticsUpdated = true;
        return systemStatistics; 
    }
    
    const MockSystemStatistics& getSystemStatistics() const { 
        return systemStatistics; 
    }

    // Mock state variables
    volatile unsigned long lastPps = 5000;
    volatile bool ppsReceived = true;
    bool gpsConnected = true;
    bool webServerStarted = true;

    // Getters
    volatile unsigned long getLastPps() const { return lastPps; }
    volatile bool isPpsReceived() const { return ppsReceived; }
    bool isGpsConnected() const { return gpsConnected; }
    bool isWebServerStarted() const { return webServerStarted; }

    // Setters
    void setLastPps(unsigned long value) { lastPps = value; }
    void setPpsReceived(bool value) { ppsReceived = value; }
    void setGpsConnected(bool value) { gpsConnected = value; }
    void setWebServerStarted(bool value) { webServerStarted = value; }

    // Statistics methods
    void incrementNtpRequests() { systemStatistics.ntpRequestsTotal++; }
    void incrementNtpResponses() { systemStatistics.ntpResponsesTotal++; }
    void incrementGpsFixCount() { systemStatistics.gpsFixCount++; }
    void updateAccuracy(float accuracy) { systemStatistics.currentAccuracy = accuracy; }

    // Mock thread safety
    void lockState() { /* Mock implementation */ }
    void unlockState() { /* Mock implementation */ }
};

// ========== Mock Service Container ==========

class MockServiceContainer {
public:
    mutable bool registerServiceCalled = false;
    mutable bool registerHardwareCalled = false;
    mutable bool initializeAllCalled = false;
    mutable bool startAllCalled = false;
    mutable bool stopAllCalled = false;
    mutable int serviceCount = 0;
    mutable int hardwareCount = 0;
    mutable bool shouldFailInitialize = false;
    mutable bool shouldFailStart = false;

    static MockServiceContainer& getInstance() {
        static MockServiceContainer instance;
        return instance;
    }

    // Mock service factories
    static IService* mockServiceFactory() {
        return new MockService();
    }

    static IHardwareInterface* mockHardwareFactory() {
        return new MockHardwareInterface();
    }

    bool registerService(const char* name, IService* (*factory)()) {
        registerServiceCalled = true;
        serviceCount++;
        return true;
    }

    bool registerHardware(const char* name, IHardwareInterface* (*factory)()) {
        registerHardwareCalled = true;
        hardwareCount++;
        return true;
    }

    IService* getService(const char* name) {
        static MockService mockService;
        return &mockService;
    }

    IHardwareInterface* getHardware(const char* name) {
        static MockHardwareInterface mockHardware;
        return &mockHardware;
    }

    template<typename T>
    T* getService(const char* name) {
        return static_cast<T*>(getService(name));
    }

    template<typename T>
    T* getHardware(const char* name) {
        return static_cast<T*>(getHardware(name));
    }

    bool initializeAll() {
        initializeAllCalled = true;
        return !shouldFailInitialize;
    }

    bool startAll() {
        startAllCalled = true;
        return !shouldFailStart;
    }

    void stopAll() {
        stopAllCalled = true;
    }

    int getServiceCount() const { return serviceCount; }
    int getHardwareCount() const { return hardwareCount; }

    void clear() {
        serviceCount = 0;
        hardwareCount = 0;
        registerServiceCalled = false;
        registerHardwareCalled = false;
        initializeAllCalled = false;
        startAllCalled = false;
        stopAllCalled = false;
    }
};

// ========== Mock System Initializer ==========

class MockSystemInitializer {
public:
    mutable bool initializeCalled = false;
    mutable bool shouldFail = false;
    mutable int initStepsCompleted = 0;
    mutable const char* lastErrorMessage = nullptr;

    struct MockInitializationResult {
        bool success = true;
        int stepsCompleted = 11;  // Total initialization steps
        const char* errorMessage = nullptr;
        ErrorType errorType = ErrorType::NONE;

        bool isSuccess() const { return success; }
        bool hasError() const { return !success; }
    };

    MockInitializationResult initialize() {
        initializeCalled = true;
        MockInitializationResult result;
        
        if (shouldFail) {
            result.success = false;
            result.stepsCompleted = initStepsCompleted;
            result.errorMessage = "Mock initialization failure";
            result.errorType = ErrorType::SYSTEM_ERROR;
            lastErrorMessage = result.errorMessage;
        } else {
            result.success = true;
            result.stepsCompleted = 11;
            initStepsCompleted = 11;
        }
        
        return result;
    }

    void reset() {
        initializeCalled = false;
        shouldFail = false;
        initStepsCompleted = 0;
        lastErrorMessage = nullptr;
    }
};

// ========== Mock Main Loop ==========

class MockMainLoop {
public:
    mutable bool executeCalled = false;
    mutable bool highPriorityProcessed = false;
    mutable bool mediumPriorityProcessed = false;
    mutable bool lowPriorityProcessed = false;
    mutable unsigned long executionCount = 0;
    mutable unsigned long mockCurrentTime = 10000;

    void execute() {
        executeCalled = true;
        executionCount++;
        
        // Mock priority-based processing
        highPriorityProcessed = true;
        
        if (executionCount % 10 == 0) {
            mediumPriorityProcessed = true;
        }
        
        if (executionCount % 100 == 0) {
            lowPriorityProcessed = true;
        }
    }

    unsigned long getCurrentTime() const {
        return mockCurrentTime;
    }

    void setCurrentTime(unsigned long time) {
        mockCurrentTime = time;
    }

    void reset() {
        executeCalled = false;
        highPriorityProcessed = false;
        mediumPriorityProcessed = false;
        lowPriorityProcessed = false;
        executionCount = 0;
        mockCurrentTime = 10000;
    }
};

// ========== Test Data Structures ==========

/**
 * @brief Test scenario data for systematic testing
 */
struct TestScenario {
    const char* name;
    const char* description;
    bool expectedSuccess;
    ErrorType expectedError;
    unsigned long testDuration;
    
    TestScenario(const char* n, const char* d, bool success = true, 
                ErrorType error = ErrorType::NONE, unsigned long duration = 1000)
        : name(n), description(d), expectedSuccess(success), 
          expectedError(error), testDuration(duration) {}
};

/**
 * @brief Test data container for structured test management
 */
class TestDataManager {
public:
    // GPS test data
    struct GpsTestData {
        bool fixAvailable = true;
        double latitude = 35.6762;
        double longitude = 139.6503;
        unsigned long accuracy = 1000; // mm
        uint8_t satellites = 8;
        uint8_t fixType = 3;
    };

    // Network test data
    struct NetworkTestData {
        bool connected = true;
        uint32_t ipAddress = 0xC0A80101; // 192.168.1.1
        uint16_t port = 80;
        unsigned long packetsSent = 100;
        unsigned long packetsReceived = 95;
    };

    // NTP test data
    struct NtpTestData {
        uint32_t timestamp = 3816211200; // 2021-01-01 00:00:00
        uint32_t fractionalSeconds = 0x80000000;
        int8_t stratum = 1;
        int8_t precision = -20;
        uint32_t rootDelay = 0;
        uint32_t rootDispersion = 100;
    };

    static TestDataManager& getInstance() {
        static TestDataManager instance;
        return instance;
    }

    GpsTestData gpsData;
    NetworkTestData networkData;
    NtpTestData ntpData;

    // Test scenarios
    static const TestScenario COMMON_SCENARIOS[];
    static const size_t SCENARIO_COUNT;

    void reset() {
        gpsData = GpsTestData{};
        networkData = NetworkTestData{};
        ntpData = NtpTestData{};
    }
};

// Common test scenarios
const TestScenario TestDataManager::COMMON_SCENARIOS[] = {
    {"normal_operation", "Normal system operation test", true, ErrorType::NONE, 5000},
    {"gps_failure", "GPS failure recovery test", false, ErrorType::GPS_ERROR, 3000},
    {"network_failure", "Network failure recovery test", false, ErrorType::NETWORK_ERROR, 3000},
    {"system_overload", "System overload handling test", true, ErrorType::NONE, 10000},
    {"power_cycle", "Power cycle recovery test", true, ErrorType::NONE, 2000},
    {"hardware_init_failure", "Hardware initialization failure test", false, ErrorType::HARDWARE_ERROR, 1000}
};

const size_t TestDataManager::SCENARIO_COUNT = sizeof(TestDataManager::COMMON_SCENARIOS) / sizeof(TestScenario);

// ========== Mock Utilities ==========

/**
 * @brief Utility class for mock setup and teardown
 */
class MockTestHelper {
public:
    static void setupSystemMocks() {
        MockSystemState::getInstance();
        MockServiceContainer::getInstance().clear();
    }

    static void teardownSystemMocks() {
        MockServiceContainer::getInstance().clear();
        TestDataManager::getInstance().reset();
    }

    static void simulateSystemTime(unsigned long timeMs) {
        mock_millis_counter = timeMs;
        mock_micros_counter = timeMs * 1000;
    }

    static Result<bool, ErrorType> createMockResult(bool success, ErrorType error = ErrorType::NONE) {
        if (success) {
            return Result<bool, ErrorType>::ok(true);
        } else {
            return Result<bool, ErrorType>::error(error);
        }
    }

    static SystemResult createMockSystemResult(bool success, ErrorType error = ErrorType::NONE) {
        if (success) {
            return SystemResult::ok();
        } else {
            return SystemResult::error(error);
        }
    }
};

// ========== Global Mock Instances ==========

// These can be used in tests for dependency injection
extern MockSystemState* g_mockSystemState;
extern MockServiceContainer* g_mockServiceContainer;
extern MockSystemInitializer* g_mockSystemInitializer;
extern MockMainLoop* g_mockMainLoop;

// Mock initialization function
inline void initializeMocks() {
    g_mockSystemState = &MockSystemState::getInstance();
    g_mockServiceContainer = &MockServiceContainer::getInstance();
    g_mockSystemInitializer = new MockSystemInitializer();
    g_mockMainLoop = new MockMainLoop();
}

inline void cleanupMocks() {
    delete g_mockSystemInitializer;
    delete g_mockMainLoop;
    g_mockSystemInitializer = nullptr;
    g_mockMainLoop = nullptr;
}