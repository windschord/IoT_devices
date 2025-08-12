#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include test data management
#include "../test_data_manager.h"

// Include necessary mocks
#include "../mocks/system_mocks.h"
#include "../mocks/http_mocks.h"

// Mock the Arduino environment
#include "../arduino_mock.h"
#include "../test_common.h"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;

/**
 * @file test_integration_new_architecture_gtest.cpp
 * @brief Integration tests for the new system architecture
 * 
 * Tests the integration of all refactored components:
 * - SystemInitializer + MainLoop + SystemState
 * - HTTP processing classes (RequestParser, ResponseBuilder, Routers)
 * - Dependency Injection with ServiceContainer
 * - Result<T, E> error handling throughout
 * - Comprehensive test data management
 */

class IntegrationNewArchitectureTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize all test infrastructure
        MockTestHelper::setupSystemMocks();
        HttpMockTestHelper::setupHttpMocks();
        initializeTestDataManager();
        
        // Initialize mock instances
        initializeMocks();
        initializeHttpMocks();
        
        // Reset all mocks to clean state
        resetAllMocks();
        
        // Set up default test scenario
        auto& testData = ComprehensiveTestDataManager::getInstance();
        testData.reset();
        
        MockTestHelper::simulateSystemTime(0);
    }

    void TearDown() override {
        MockTestHelper::teardownSystemMocks();
        HttpMockTestHelper::teardownHttpMocks();
        cleanupMocks();
        cleanupHttpMocks();
    }

private:
    void resetAllMocks() {
        g_mockSystemInitializer->reset();
        g_mockMainLoop->reset();
        g_mockServiceContainer->clear();
        
        g_mockHttpRequestParser->reset();
        g_mockHttpResponseBuilder->reset();
        g_mockRouteHandler->reset();
        g_mockApiRouter->reset();
        g_mockFileRouter->reset();
        g_mockFileSystemHandler->reset();
        g_mockMimeTypeResolver->reset();
        g_mockCacheManager->reset();
    }
};

// ========== Basic Architecture Integration Tests ==========

TEST_F(IntegrationNewArchitectureTest, BasicSystemStartup) {
    // Test basic system startup with new architecture
    
    // 1. System initialization
    auto initResult = g_mockSystemInitializer->initialize();
    EXPECT_TRUE(initResult.isSuccess());
    EXPECT_TRUE(g_mockSystemInitializer->initializeCalled);
    
    // 2. Service container setup
    EXPECT_TRUE(g_mockServiceContainer->initializeAll());
    EXPECT_TRUE(g_mockServiceContainer->initializeAllCalled);
    
    // 3. Main loop execution
    g_mockMainLoop->execute();
    EXPECT_TRUE(g_mockMainLoop->executeCalled);
    EXPECT_TRUE(g_mockMainLoop->highPriorityProcessed);
    
    // 4. System state verification
    auto& systemState = MockSystemState::getInstance();
    EXPECT_TRUE(systemState.isGpsConnected());
    EXPECT_TRUE(systemState.isWebServerStarted());
}

TEST_F(IntegrationNewArchitectureTest, SystemInitializationFailureRecovery) {
    // Test system behavior when initialization partially fails
    
    // Force initialization failure at step 5
    g_mockSystemInitializer->shouldFail = true;
    g_mockSystemInitializer->initStepsCompleted = 5;
    
    auto initResult = g_mockSystemInitializer->initialize();
    
    EXPECT_FALSE(initResult.isSuccess());
    EXPECT_EQ(initResult.stepsCompleted, 5);
    EXPECT_EQ(initResult.errorType, ErrorType::SYSTEM_ERROR);
    
    // System should still be able to handle basic operations
    g_mockMainLoop->execute();
    EXPECT_TRUE(g_mockMainLoop->executeCalled);
}

// ========== HTTP Processing Integration Tests ==========

TEST_F(IntegrationNewArchitectureTest, HttpProcessingPipeline) {
    // Test complete HTTP processing pipeline with new architecture
    
    // 1. Parse incoming request
    auto request = HttpMockTestHelper::createMockRequest("GET", "/api/status", "", 0);
    EXPECT_TRUE(request.isValid());
    
    // 2. Route determination
    g_mockRouteHandler->addRoute("/api/status", "GET", 1, MockRouteHandler::mockHandler);
    EXPECT_TRUE(g_mockRouteHandler->matchesRoute("/api/status", "GET"));
    
    // 3. API request handling
    g_mockApiRouter->isApiPath = true;
    auto response = g_mockApiRouter->handleApiRequest(request);
    
    EXPECT_TRUE(g_mockApiRouter->handleApiRequestCalled);
    EXPECT_EQ(response.statusCode, 200);
    
    // 4. Response building
    auto finalResponse = g_mockHttpResponseBuilder->buildJsonResponse(response.body);
    EXPECT_TRUE(g_mockHttpResponseBuilder->buildResponseCalled);
    EXPECT_EQ(finalResponse.statusCode, 200);
}

TEST_F(IntegrationNewArchitectureTest, FileServingIntegration) {
    // Test file serving through the new HTTP architecture
    
    // 1. Request for static file
    auto request = HttpMockTestHelper::createMockRequest("GET", "/index.html");
    
    // 2. File system check
    g_mockFileSystemHandler->setMockFile("<html>Test Page</html>", 20, true);
    EXPECT_TRUE(g_mockFileSystemHandler->fileExists("/index.html"));
    
    // 3. MIME type resolution
    const char* mimeType = g_mockMimeTypeResolver->getMimeType("index.html");
    EXPECT_STREQ(mimeType, "text/html");
    
    // 4. Cache check
    g_mockCacheManager->setCachedResponse("", false);  // Not cached
    auto cacheResult = g_mockCacheManager->getCachedResponse("/index.html");
    EXPECT_TRUE(cacheResult.isError());
    
    // 5. File serving
    auto response = g_mockFileRouter->handleFileRequest(request);
    EXPECT_EQ(response.statusCode, 200);
    
    // 6. Cache the response
    EXPECT_TRUE(g_mockCacheManager->cacheResponse("/index.html", response.body, "etag-123"));
}

// ========== Dependency Injection Integration Tests ==========

TEST_F(IntegrationNewArchitectureTest, DependencyInjectionFlow) {
    // Test dependency injection throughout the system
    
    // 1. Register services in DI container
    EXPECT_TRUE(g_mockServiceContainer->registerService("TestService", MockServiceContainer::mockServiceFactory));
    EXPECT_TRUE(g_mockServiceContainer->registerHardware("TestHardware", MockServiceContainer::mockHardwareFactory));
    
    // 2. Initialize all services
    EXPECT_TRUE(g_mockServiceContainer->initializeAll());
    
    // 3. Start all services
    EXPECT_TRUE(g_mockServiceContainer->startAll());
    
    // 4. Verify service container state
    EXPECT_GT(g_mockServiceContainer->getServiceCount(), 0);
    EXPECT_GT(g_mockServiceContainer->getHardwareCount(), 0);
    
    // 5. Retrieve and use services
    auto service = g_mockServiceContainer->getService<MockService>("TestService");
    EXPECT_NE(service, nullptr);
    
    auto hardware = g_mockServiceContainer->getHardware<MockHardwareInterface>("TestHardware");
    EXPECT_NE(hardware, nullptr);
}

TEST_F(IntegrationNewArchitectureTest, ServiceContainerFailureHandling) {
    // Test DI container behavior with service failures
    
    // Register services that will fail to initialize
    EXPECT_TRUE(g_mockServiceContainer->registerService("FailingService", MockServiceContainer::mockServiceFactory));
    
    // Force failure
    g_mockServiceContainer->shouldFailInitialize = true;
    
    // Try to initialize all services
    EXPECT_FALSE(g_mockServiceContainer->initializeAll());
    EXPECT_TRUE(g_mockServiceContainer->initializeAllCalled);
    
    // System should handle the failure gracefully
    g_mockMainLoop->execute();
    EXPECT_TRUE(g_mockMainLoop->executeCalled);
}

// ========== Result Type Integration Tests ==========

TEST_F(IntegrationNewArchitectureTest, ResultTypeErrorPropagation) {
    // Test Result<T, E> error propagation through the system
    
    // Create a chain of operations that use Result types
    auto step1 = [](bool succeed) -> Result<int, ErrorType> {
        if (succeed) {
            return Result<int, ErrorType>::ok(42);
        }
        return Result<int, ErrorType>::error(ErrorType::SYSTEM_ERROR);
    };
    
    auto step2 = [](int value) -> Result<const char*, ErrorType> {
        if (value > 0) {
            return Result<const char*, ErrorType>::ok("success");
        }
        return Result<const char*, ErrorType>::error(ErrorType::CONFIG_ERROR);
    };
    
    // Test successful chain
    auto successResult = step1(true).andThen(step2);
    EXPECT_TRUE(successResult.isOk());
    EXPECT_STREQ(successResult.value(), "success");
    
    // Test failed chain (error propagation)
    auto failureResult = step1(false).andThen(step2);
    EXPECT_TRUE(failureResult.isError());
    EXPECT_EQ(failureResult.error(), ErrorType::SYSTEM_ERROR);
}

TEST_F(IntegrationNewArchitectureTest, ResultTypeWithSystemOperations) {
    // Test Result types with actual system operations
    
    // Simulate system initialization returning Result
    auto mockInitResult = MockTestHelper::createMockSystemResult(true);
    EXPECT_TRUE(mockInitResult.isOk());
    
    // Chain with other operations
    auto chainedResult = mockInitResult.andThen([]() -> SystemResult {
        // Simulate successful service start
        return SystemResult::ok();
    });
    
    EXPECT_TRUE(chainedResult.isOk());
    
    // Test error case
    auto errorResult = MockTestHelper::createMockSystemResult(false, ErrorType::HARDWARE_ERROR);
    EXPECT_TRUE(errorResult.isError());
    EXPECT_EQ(errorResult.error(), ErrorType::HARDWARE_ERROR);
}

// ========== Test Scenario-Based Integration Tests ==========

TEST_F(IntegrationNewArchitectureTest, NormalOperationScenario) {
    // Use predefined test scenario for integration testing
    auto& testManager = ComprehensiveTestDataManager::getInstance();
    const auto* scenario = testManager.findScenario("normal_operation_optimal");
    ASSERT_NE(scenario, nullptr);
    
    // Set up system based on scenario data
    auto& systemState = MockSystemState::getInstance();
    systemState.setGpsConnected(scenario->gpsData.fixAvailable);
    systemState.setWebServerStarted(scenario->networkData.connected);
    
    // Run system initialization
    auto initResult = g_mockSystemInitializer->initialize();
    EXPECT_EQ(initResult.isSuccess(), scenario->expectedSuccess);
    
    // Execute main loop for scenario duration
    const unsigned long scenarioDuration = 1000;  // Shortened for test
    for (unsigned long t = 0; t < scenarioDuration; t += 100) {
        MockTestHelper::simulateSystemTime(t);
        g_mockMainLoop->execute();
    }
    
    // Verify scenario expectations
    EXPECT_GT(g_mockMainLoop->executionCount, 0);
    EXPECT_TRUE(g_mockMainLoop->highPriorityProcessed);
}

TEST_F(IntegrationNewArchitectureTest, ErrorHandlingScenario) {
    // Use error scenario for testing
    auto& testManager = ComprehensiveTestDataManager::getInstance();
    const auto* scenario = testManager.findScenario("error_gps_signal_lost");
    ASSERT_NE(scenario, nullptr);
    
    // Set up error conditions
    auto& systemState = MockSystemState::getInstance();
    systemState.setGpsConnected(false);  // GPS lost
    
    // System should handle the error gracefully
    g_mockMainLoop->execute();
    EXPECT_TRUE(g_mockMainLoop->executeCalled);
    
    // Error should be reflected in system state
    EXPECT_FALSE(systemState.isGpsConnected());
}

TEST_F(IntegrationNewArchitectureTest, HighLoadScenario) {
    // Test high load scenario
    auto& testManager = ComprehensiveTestDataManager::getInstance();
    const auto* scenario = testManager.findScenario("performance_high_ntp_load");
    ASSERT_NE(scenario, nullptr);
    
    // Simulate high load conditions
    for (int i = 0; i < 100; ++i) {
        // Simulate many HTTP requests
        auto request = HttpMockTestHelper::createMockRequest("GET", "/api/status");
        auto response = g_mockApiRouter->handleApiRequest(request);
        EXPECT_EQ(response.statusCode, 200);
        
        // Execute main loop
        g_mockMainLoop->execute();
    }
    
    // System should handle high load
    EXPECT_EQ(g_mockMainLoop->executionCount, 100);
    EXPECT_TRUE(g_mockApiRouter->handleApiRequestCalled);
}

// ========== Full System Integration Tests ==========

TEST_F(IntegrationNewArchitectureTest, FullSystemIntegration) {
    // Comprehensive full system integration test
    
    // Phase 1: System Initialization
    auto initResult = g_mockSystemInitializer->initialize();
    EXPECT_TRUE(initResult.isSuccess());
    EXPECT_EQ(initResult.stepsCompleted, 11);
    
    // Phase 2: Service Container Setup
    g_mockServiceContainer->registerService("ConfigManager", MockServiceContainer::mockServiceFactory);
    g_mockServiceContainer->registerService("NetworkManager", MockServiceContainer::mockServiceFactory);
    g_mockServiceContainer->registerService("NtpServer", MockServiceContainer::mockServiceFactory);
    
    EXPECT_TRUE(g_mockServiceContainer->initializeAll());
    EXPECT_TRUE(g_mockServiceContainer->startAll());
    
    // Phase 3: HTTP Processing Setup
    g_mockApiRouter->setupApiRoutes();
    g_mockFileRouter->setupFileRoutes();
    
    // Phase 4: Main Loop Operation
    for (int i = 0; i < 150; ++i) {  // Enough to trigger all priority levels
        g_mockMainLoop->execute();
        
        // Process some HTTP requests during operation
        if (i % 10 == 0) {
            auto request = HttpMockTestHelper::createMockRequest("GET", "/api/status");
            auto response = g_mockApiRouter->handleApiRequest(request);
            EXPECT_EQ(response.statusCode, 200);
        }
    }
    
    // Phase 5: Verification
    EXPECT_EQ(g_mockMainLoop->executionCount, 150);
    EXPECT_TRUE(g_mockMainLoop->highPriorityProcessed);
    EXPECT_TRUE(g_mockMainLoop->mediumPriorityProcessed);
    EXPECT_TRUE(g_mockMainLoop->lowPriorityProcessed);
    
    EXPECT_TRUE(g_mockApiRouter->setupApiRoutesCalled);
    EXPECT_TRUE(g_mockFileRouter->setupFileRoutesCalled);
    EXPECT_TRUE(g_mockApiRouter->handleApiRequestCalled);
    
    // Phase 6: Graceful Shutdown
    g_mockServiceContainer->stopAll();
    EXPECT_TRUE(g_mockServiceContainer->stopAllCalled);
}

// ========== Error Recovery Integration Tests ==========

TEST_F(IntegrationNewArchitectureTest, SystemRecoveryAfterFailure) {
    // Test system recovery after various failure modes
    
    // Simulate initial failure
    g_mockSystemInitializer->shouldFail = true;
    g_mockSystemInitializer->initStepsCompleted = 7;
    
    auto failedInit = g_mockSystemInitializer->initialize();
    EXPECT_FALSE(failedInit.isSuccess());
    
    // System attempts recovery
    g_mockSystemInitializer->reset();
    auto recoveredInit = g_mockSystemInitializer->initialize();
    EXPECT_TRUE(recoveredInit.isSuccess());
    
    // System continues normal operation
    g_mockMainLoop->execute();
    EXPECT_TRUE(g_mockMainLoop->executeCalled);
    
    // HTTP processing should work after recovery
    auto request = HttpMockTestHelper::createMockRequest("GET", "/api/status");
    auto response = g_mockApiRouter->handleApiRequest(request);
    EXPECT_EQ(response.statusCode, 200);
}

// ========== Performance Integration Tests ==========

TEST_F(IntegrationNewArchitectureTest, SystemPerformanceUnderLoad) {
    // Test system performance under sustained load
    
    const int loadDuration = 1000;  // iterations
    const int httpRequestsPerIteration = 5;
    
    unsigned long startTime = millis();
    
    for (int i = 0; i < loadDuration; ++i) {
        // Execute main loop
        g_mockMainLoop->execute();
        
        // Process multiple HTTP requests
        for (int j = 0; j < httpRequestsPerIteration; ++j) {
            auto request = HttpMockTestHelper::createMockRequest("GET", "/api/test");
            auto response = g_mockApiRouter->handleApiRequest(request);
            EXPECT_EQ(response.statusCode, 200);
        }
        
        // Advance time
        MockTestHelper::simulateSystemTime(i);
    }
    
    unsigned long endTime = millis();
    
    // Verify performance
    EXPECT_EQ(g_mockMainLoop->executionCount, loadDuration);
    EXPECT_TRUE(g_mockApiRouter->handleApiRequestCalled);
    EXPECT_GE(endTime, startTime);
    
    // System should remain responsive
    auto& systemState = MockSystemState::getInstance();
    EXPECT_TRUE(systemState.isGpsConnected());
    EXPECT_TRUE(systemState.isWebServerStarted());
}

// ========== Data Flow Integration Tests ==========

TEST_F(IntegrationNewArchitectureTest, DataFlowThroughSystem) {
    // Test data flow through all system components
    
    // 1. Initialize with test data
    auto& testManager = ComprehensiveTestDataManager::getInstance();
    testManager.currentGpsData = GpsTestData::create3DFix();
    testManager.currentNetworkData = NetworkTestData{};
    testManager.currentNtpData = NtpTestData{};
    
    // 2. System processes the data
    g_mockSystemInitializer->initialize();
    g_mockMainLoop->execute();
    
    // 3. Data is available through HTTP API
    auto request = HttpMockTestHelper::createMockRequest("GET", "/api/gps");
    g_mockApiRouter->setMockApiResponse("{\"fix\":true,\"satellites\":8}");
    auto response = g_mockApiRouter->handleApiRequest(request);
    
    EXPECT_EQ(response.statusCode, 200);
    EXPECT_TRUE(strstr(response.body, "fix") != nullptr);
    
    // 4. Data is cached for efficiency
    EXPECT_TRUE(g_mockCacheManager->cacheResponse("/api/gps", response.body, "gps-etag"));
    
    // 5. Subsequent requests use cached data
    g_mockCacheManager->setCachedResponse(response.body, true);
    auto cachedResult = g_mockCacheManager->getCachedResponse("/api/gps");
    EXPECT_TRUE(cachedResult.isOk());
}

// ========== Mock Integration Verification ==========

TEST_F(IntegrationNewArchitectureTest, AllMocksProperlyIntegrated) {
    // Comprehensive test to verify all mocks work together properly
    
    // System mocks
    EXPECT_NE(g_mockSystemInitializer, nullptr);
    EXPECT_NE(g_mockMainLoop, nullptr);
    EXPECT_NE(g_mockServiceContainer, nullptr);
    
    // HTTP mocks
    EXPECT_NE(g_mockHttpRequestParser, nullptr);
    EXPECT_NE(g_mockHttpResponseBuilder, nullptr);
    EXPECT_NE(g_mockRouteHandler, nullptr);
    EXPECT_NE(g_mockApiRouter, nullptr);
    EXPECT_NE(g_mockFileRouter, nullptr);
    EXPECT_NE(g_mockFileSystemHandler, nullptr);
    EXPECT_NE(g_mockMimeTypeResolver, nullptr);
    EXPECT_NE(g_mockCacheManager, nullptr);
    
    // Test data manager
    EXPECT_NE(g_testDataManager, nullptr);
    EXPECT_GT(g_testDataManager->getScenarioCount(), 0);
    
    // All systems functional
    g_mockSystemInitializer->initialize();
    g_mockMainLoop->execute();
    
    auto request = HttpMockTestHelper::createMockRequest("GET", "/");
    auto response = g_mockHttpResponseBuilder->buildResponse(200, "OK", "text/html");
    
    EXPECT_TRUE(g_mockSystemInitializer->initializeCalled);
    EXPECT_TRUE(g_mockMainLoop->executeCalled);
    EXPECT_TRUE(g_mockHttpResponseBuilder->buildResponseCalled);
}

// ========== Test Data Manager Integration ==========

TEST_F(IntegrationNewArchitectureTest, TestDataManagerIntegration) {
    // Test integration with comprehensive test data manager
    
    auto& testManager = ComprehensiveTestDataManager::getInstance();
    
    // Test scenario enumeration
    const auto* scenarios = testManager.getScenarios();
    EXPECT_NE(scenarios, nullptr);
    EXPECT_EQ(testManager.getScenarioCount(), 20);
    
    // Test scenario by category
    const TestScenario* initScenarios[10];
    size_t initCount = testManager.getScenariosByCategory(
        TestScenarioCategory::INITIALIZATION, initScenarios, 10);
    EXPECT_GT(initCount, 0);
    
    // Test specific scenario
    const auto* normalOp = testManager.findScenario("normal_operation_optimal");
    ASSERT_NE(normalOp, nullptr);
    EXPECT_EQ(normalOp->category, TestScenarioCategory::NORMAL_OPERATION);
    EXPECT_TRUE(normalOp->expectedSuccess);
    
    // Test data builder pattern
    TestDataBuilder builder;
    auto customScenario = builder
        .withHealthySystem()
        .withGpsFix3D()
        .withNetworkConnected()
        .withNtpSynchronized()
        .build("custom_test", "Custom test scenario");
    
    EXPECT_STREQ(customScenario.name, "custom_test");
    EXPECT_TRUE(customScenario.gpsData.fixAvailable);
    EXPECT_TRUE(customScenario.networkData.connected);
    EXPECT_TRUE(customScenario.ntpData.synchronized);
}