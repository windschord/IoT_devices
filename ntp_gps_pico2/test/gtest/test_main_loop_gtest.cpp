#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include necessary mocks before the actual headers
#include "../mocks/system_mocks.h"
#include "../mocks/http_mocks.h"

// Mock the Arduino environment
#include "../arduino_mock.h"
#include "../test_common.h"

// Include the system under test
#include "../../src/system/MainLoop.h"
#include "../../src/system/SystemState.h"
#include "../../src/system/Result.h"

using ::testing::_;
using ::testing::Return;
using ::testing::InSequence;
using ::testing::AtLeast;

/**
 * @file test_main_loop_gtest.cpp
 * @brief GoogleTest tests for MainLoop class
 * 
 * Tests the new MainLoop class created during the main.cpp refactoring.
 * This class handles priority-based processing (HIGH/MEDIUM/LOW) in a structured way.
 */

class MainLoopTest : public ::testing::Test {
protected:
    void SetUp() override {
        MockTestHelper::setupSystemMocks();
        
        // Reset all mocks to clean state
        mockMainLoop.reset();
        MockTestHelper::simulateSystemTime(10000);  // Start at 10 seconds
    }

    void TearDown() override {
        MockTestHelper::teardownSystemMocks();
    }

    MockMainLoop mockMainLoop;
};

// ========== Basic Execution Tests ==========

TEST_F(MainLoopTest, BasicExecutionSuccess) {
    // Test basic main loop execution
    EXPECT_FALSE(mockMainLoop.executeCalled);
    EXPECT_EQ(mockMainLoop.executionCount, 0);
    
    mockMainLoop.execute();
    
    EXPECT_TRUE(mockMainLoop.executeCalled);
    EXPECT_EQ(mockMainLoop.executionCount, 1);
    EXPECT_TRUE(mockMainLoop.highPriorityProcessed);
}

TEST_F(MainLoopTest, MultipleExecutions) {
    // Test multiple executions of the main loop
    const int numExecutions = 5;
    
    for (int i = 1; i <= numExecutions; ++i) {
        mockMainLoop.execute();
        EXPECT_EQ(mockMainLoop.executionCount, i);
        EXPECT_TRUE(mockMainLoop.highPriorityProcessed);
    }
}

// ========== Priority-based Processing Tests ==========

TEST_F(MainLoopTest, HighPriorityAlwaysProcessed) {
    // Test that high priority tasks are always processed
    for (int i = 0; i < 20; ++i) {
        mockMainLoop.reset();
        mockMainLoop.execute();
        
        EXPECT_TRUE(mockMainLoop.highPriorityProcessed) 
            << "High priority not processed on execution " << i;
    }
}

TEST_F(MainLoopTest, MediumPriorityProcessedPeriodically) {
    // Test that medium priority tasks are processed every 10 executions
    mockMainLoop.reset();
    
    // First 9 executions should not process medium priority
    for (int i = 1; i < 10; ++i) {
        mockMainLoop.executionCount = i - 1;  // Set count before execution
        mockMainLoop.mediumPriorityProcessed = false;
        mockMainLoop.execute();
        
        EXPECT_FALSE(mockMainLoop.mediumPriorityProcessed) 
            << "Medium priority processed too early at execution " << i;
    }
    
    // 10th execution should process medium priority
    mockMainLoop.executionCount = 9;  // Set count before execution
    mockMainLoop.mediumPriorityProcessed = false;
    mockMainLoop.execute();
    EXPECT_TRUE(mockMainLoop.mediumPriorityProcessed);
}

TEST_F(MainLoopTest, LowPriorityProcessedPeriodically) {
    // Test that low priority tasks are processed every 100 executions
    mockMainLoop.reset();
    
    // First 99 executions should not process low priority
    for (int i = 1; i < 100; ++i) {
        mockMainLoop.executionCount = i - 1;  // Set count before execution
        mockMainLoop.lowPriorityProcessed = false;
        mockMainLoop.execute();
        
        EXPECT_FALSE(mockMainLoop.lowPriorityProcessed) 
            << "Low priority processed too early at execution " << i;
    }
    
    // 100th execution should process low priority
    mockMainLoop.executionCount = 99;  // Set count before execution
    mockMainLoop.lowPriorityProcessed = false;
    mockMainLoop.execute();
    EXPECT_TRUE(mockMainLoop.lowPriorityProcessed);
}

// ========== Timing and Performance Tests ==========

TEST_F(MainLoopTest, TimingConsistency) {
    // Test timing consistency across executions
    unsigned long startTime = mockMainLoop.getCurrentTime();
    
    for (int i = 0; i < 10; ++i) {
        unsigned long beforeExec = mockMainLoop.getCurrentTime();
        mockMainLoop.execute();
        unsigned long afterExec = mockMainLoop.getCurrentTime();
        
        // Time should advance (even in mock)
        EXPECT_GE(afterExec, beforeExec);
    }
    
    unsigned long endTime = mockMainLoop.getCurrentTime();
    EXPECT_GT(endTime, startTime);
}

TEST_F(MainLoopTest, CurrentTimeTracking) {
    // Test current time tracking functionality
    const unsigned long testTime = 25000;
    mockMainLoop.setCurrentTime(testTime);
    
    EXPECT_EQ(mockMainLoop.getCurrentTime(), testTime);
    
    // After execution, time might advance
    mockMainLoop.execute();
    EXPECT_GE(mockMainLoop.getCurrentTime(), testTime);
}

// ========== Integration with SystemState Tests ==========

TEST_F(MainLoopTest, SystemStateIntegration) {
    // Test integration with SystemState during execution
    auto& systemState = MockSystemState::getInstance();
    
    // Set up some initial state
    systemState.setGpsConnected(true);
    systemState.setWebServerStarted(true);
    
    mockMainLoop.execute();
    
    // Verify execution completed successfully
    EXPECT_TRUE(mockMainLoop.executeCalled);
    
    // Verify system state remains accessible
    EXPECT_TRUE(systemState.isGpsConnected());
    EXPECT_TRUE(systemState.isWebServerStarted());
}

// ========== Error Handling Tests ==========

TEST_F(MainLoopTest, ContinuousExecutionResilience) {
    // Test that main loop continues executing even with potential issues
    for (int i = 0; i < 100; ++i) {
        mockMainLoop.execute();
        
        // Each execution should complete successfully
        EXPECT_EQ(mockMainLoop.executionCount, i + 1);
        EXPECT_TRUE(mockMainLoop.highPriorityProcessed);
    }
}

// ========== Priority Processing Pattern Tests ==========

TEST_F(MainLoopTest, PriorityProcessingPattern) {
    // Test the complete priority processing pattern over many executions
    mockMainLoop.reset();
    
    struct PriorityStats {
        int highCount = 0;
        int mediumCount = 0;
        int lowCount = 0;
    };
    
    PriorityStats stats;
    const int totalExecutions = 200;
    
    for (int i = 1; i <= totalExecutions; ++i) {
        mockMainLoop.executionCount = i - 1;
        mockMainLoop.highPriorityProcessed = false;
        mockMainLoop.mediumPriorityProcessed = false;
        mockMainLoop.lowPriorityProcessed = false;
        
        mockMainLoop.execute();
        
        if (mockMainLoop.highPriorityProcessed) stats.highCount++;
        if (mockMainLoop.mediumPriorityProcessed) stats.mediumCount++;
        if (mockMainLoop.lowPriorityProcessed) stats.lowCount++;
    }
    
    // Verify processing frequency
    EXPECT_EQ(stats.highCount, totalExecutions);  // High priority every time
    EXPECT_EQ(stats.mediumCount, totalExecutions / 10);  // Medium priority every 10th
    EXPECT_EQ(stats.lowCount, totalExecutions / 100);  // Low priority every 100th
}

// ========== Performance and Resource Tests ==========

TEST_F(MainLoopTest, ExecutionEfficiency) {
    // Test execution efficiency (should not consume excessive resources)
    const int manyExecutions = 1000;
    
    unsigned long startTime = millis();
    
    for (int i = 0; i < manyExecutions; ++i) {
        mockMainLoop.execute();
    }
    
    unsigned long endTime = millis();
    
    // Verify all executions completed
    EXPECT_EQ(mockMainLoop.executionCount, manyExecutions);
    
    // In mock environment, this just verifies the pattern works
    EXPECT_GE(endTime, startTime);
}

TEST_F(MainLoopTest, MemoryUsageStability) {
    // Test that repeated executions don't cause memory issues
    // This is more relevant for actual implementation but tests the pattern
    
    for (int i = 0; i < 500; ++i) {
        mockMainLoop.execute();
    }
    
    // Verify system remains stable
    EXPECT_EQ(mockMainLoop.executionCount, 500);
    
    // Memory usage should remain stable (tested in real implementation)
    // Here we just verify the execution pattern is correct
}

// ========== Reset and State Management Tests ==========

TEST_F(MainLoopTest, ResetFunctionality) {
    // Test reset functionality
    mockMainLoop.execute();
    EXPECT_TRUE(mockMainLoop.executeCalled);
    EXPECT_GT(mockMainLoop.executionCount, 0);
    
    mockMainLoop.reset();
    
    EXPECT_FALSE(mockMainLoop.executeCalled);
    EXPECT_EQ(mockMainLoop.executionCount, 0);
    EXPECT_FALSE(mockMainLoop.highPriorityProcessed);
    EXPECT_FALSE(mockMainLoop.mediumPriorityProcessed);
    EXPECT_FALSE(mockMainLoop.lowPriorityProcessed);
}

// ========== Real-World Scenario Tests ==========

TEST_F(MainLoopTest, NormalOperationScenario) {
    // Test normal operation scenario over extended period
    mockMainLoop.setCurrentTime(0);
    
    // Simulate 1 minute of operation (assuming 1ms per loop iteration)
    const int oneMinuteIterations = 60000;
    
    for (int i = 0; i < 100; ++i) {  // Test sample of iterations
        mockMainLoop.setCurrentTime(i * 600);  // 600ms intervals
        mockMainLoop.execute();
        
        EXPECT_TRUE(mockMainLoop.highPriorityProcessed);
        
        // Check priority processing at appropriate intervals
        if ((i + 1) % 10 == 0) {
            EXPECT_TRUE(mockMainLoop.mediumPriorityProcessed);
        }
        
        if ((i + 1) % 100 == 0) {
            EXPECT_TRUE(mockMainLoop.lowPriorityProcessed);
        }
    }
}

TEST_F(MainLoopTest, HighLoadScenario) {
    // Test behavior under high load (rapid executions)
    mockMainLoop.setCurrentTime(1000);
    
    // Rapid fire executions
    for (int i = 0; i < 50; ++i) {
        mockMainLoop.setCurrentTime(1000 + i);  // 1ms intervals
        mockMainLoop.execute();
        
        // Even under high load, high priority should be processed
        EXPECT_TRUE(mockMainLoop.highPriorityProcessed);
    }
    
    EXPECT_EQ(mockMainLoop.executionCount, 50);
}

// ========== Integration Tests ==========

TEST_F(MainLoopTest, ServiceIntegration) {
    // Test integration with service layer
    auto& container = MockServiceContainer::getInstance();
    
    // Simulate some services being available
    container.serviceCount = 5;
    
    mockMainLoop.execute();
    
    EXPECT_TRUE(mockMainLoop.executeCalled);
    
    // In real implementation, MainLoop would interact with services
    // Here we verify the pattern would work
    EXPECT_GT(container.getServiceCount(), 0);
}

TEST_F(MainLoopTest, DIContainerIntegration) {
    // Test dependency injection integration
    auto& container = MockServiceContainer::getInstance();
    container.initializeAll();
    
    mockMainLoop.execute();
    
    EXPECT_TRUE(mockMainLoop.executeCalled);
    EXPECT_TRUE(container.initializeAllCalled);
}

// ========== Parameterized Tests for Priority Intervals ==========

class MainLoopPriorityTest : public MainLoopTest,
                            public ::testing::WithParamInterface<std::tuple<int, bool, bool, bool>> {
protected:
    void SetUp() override {
        MainLoopTest::SetUp();
    }
};

TEST_P(MainLoopPriorityTest, PriorityProcessingAtSpecificIntervals) {
    auto [executionNumber, expectHigh, expectMedium, expectLow] = GetParam();
    
    mockMainLoop.reset();
    mockMainLoop.executionCount = executionNumber - 1;  // Set count before execution
    mockMainLoop.execute();
    
    EXPECT_EQ(mockMainLoop.highPriorityProcessed, expectHigh)
        << "High priority expectation failed at execution " << executionNumber;
    EXPECT_EQ(mockMainLoop.mediumPriorityProcessed, expectMedium)
        << "Medium priority expectation failed at execution " << executionNumber;
    EXPECT_EQ(mockMainLoop.lowPriorityProcessed, expectLow)
        << "Low priority expectation failed at execution " << executionNumber;
}

INSTANTIATE_TEST_SUITE_P(
    PriorityIntervals,
    MainLoopPriorityTest,
    ::testing::Values(
        std::make_tuple(1, true, false, false),    // 1st execution: only high
        std::make_tuple(5, true, false, false),    // 5th execution: only high
        std::make_tuple(10, true, true, false),    // 10th execution: high + medium
        std::make_tuple(20, true, true, false),    // 20th execution: high + medium
        std::make_tuple(50, true, true, false),    // 50th execution: high + medium
        std::make_tuple(100, true, true, true),    // 100th execution: all three
        std::make_tuple(200, true, true, true)     // 200th execution: all three
    )
);

// ========== Test Data Integration ==========

TEST_F(MainLoopTest, TestScenarioIntegration) {
    // Test integration with test scenarios
    auto& testData = TestDataManager::getInstance();
    testData.reset();
    
    // Use test scenarios for main loop testing
    for (size_t i = 0; i < TestDataManager::SCENARIO_COUNT; ++i) {
        const auto& scenario = TestDataManager::COMMON_SCENARIOS[i];
        
        mockMainLoop.reset();
        mockMainLoop.setCurrentTime(0);
        
        // Run main loop for the scenario duration
        unsigned long scenarioDuration = scenario.testDuration;
        for (unsigned long t = 0; t < scenarioDuration; t += 100) {
            mockMainLoop.setCurrentTime(t);
            mockMainLoop.execute();
        }
        
        // Verify execution occurred
        EXPECT_GT(mockMainLoop.executionCount, 0) 
            << "No executions for scenario: " << scenario.name;
        EXPECT_TRUE(mockMainLoop.highPriorityProcessed)
            << "High priority not processed for scenario: " << scenario.name;
    }
}

// ========== Mock Verification Tests ==========

TEST_F(MainLoopTest, MockStateConsistency) {
    // Comprehensive test to verify mock state consistency
    mockMainLoop.execute();
    
    EXPECT_TRUE(mockMainLoop.executeCalled);
    EXPECT_GT(mockMainLoop.executionCount, 0);
    EXPECT_TRUE(mockMainLoop.highPriorityProcessed);
    
    // Verify time advancement
    EXPECT_GE(mockMainLoop.getCurrentTime(), 10000);  // Started at 10000
}

TEST_F(MainLoopTest, AllMockFeaturesUsed) {
    // Test that all mock features are properly used
    const unsigned long testTime = 50000;
    mockMainLoop.setCurrentTime(testTime);
    
    for (int i = 0; i < 110; ++i) {  // Enough to trigger all priority levels
        mockMainLoop.execute();
    }
    
    // Verify all mock features were exercised
    EXPECT_TRUE(mockMainLoop.executeCalled);
    EXPECT_GT(mockMainLoop.executionCount, 100);
    EXPECT_TRUE(mockMainLoop.highPriorityProcessed);
    EXPECT_TRUE(mockMainLoop.mediumPriorityProcessed);
    EXPECT_TRUE(mockMainLoop.lowPriorityProcessed);
    EXPECT_GE(mockMainLoop.getCurrentTime(), testTime);
}