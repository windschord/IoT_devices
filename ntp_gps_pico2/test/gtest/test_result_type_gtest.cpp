#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include necessary mocks
#include "../mocks/system_mocks.h"

// Mock the Arduino environment
#include "../arduino_mock.h"
#include "../test_common.h"

// Include the system under test
#include "../../src/system/Result.h"
#include "../../src/system/ErrorCategories.h"

using ::testing::_;

/**
 * @file test_result_type_gtest.cpp
 * @brief GoogleTest tests for Result<T, E> type
 * 
 * Tests the Result<T, E> type introduced for type-safe error handling
 * without exceptions, suitable for embedded systems.
 */

class ResultTypeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset any global state if needed
    }

    void TearDown() override {
        // Cleanup if needed
    }
};

// ========== Basic Result Creation Tests ==========

TEST_F(ResultTypeTest, CreateSuccessResult) {
    // Test creating a successful Result
    auto result = Result<int, ErrorType>::ok(42);
    
    EXPECT_TRUE(result.isOk());
    EXPECT_FALSE(result.isError());
    EXPECT_EQ(result.value(), 42);
}

TEST_F(ResultTypeTest, CreateErrorResult) {
    // Test creating an error Result
    auto result = Result<int, ErrorType>::error(ErrorType::SYSTEM_ERROR);
    
    EXPECT_FALSE(result.isOk());
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), ErrorType::SYSTEM_ERROR);
}

TEST_F(ResultTypeTest, CreateStringResult) {
    // Test creating Result with string value
    const char* testString = "Hello, World!";
    auto result = Result<const char*, ErrorType>::ok(testString);
    
    EXPECT_TRUE(result.isOk());
    EXPECT_STREQ(result.value(), testString);
}

// ========== Copy and Move Semantics Tests ==========

TEST_F(ResultTypeTest, CopyConstructor) {
    // Test copy constructor
    auto original = Result<int, ErrorType>::ok(123);
    auto copied = original;
    
    EXPECT_TRUE(copied.isOk());
    EXPECT_EQ(copied.value(), 123);
    
    // Original should still be valid
    EXPECT_TRUE(original.isOk());
    EXPECT_EQ(original.value(), 123);
}

TEST_F(ResultTypeTest, MoveConstructor) {
    // Test move constructor
    auto original = Result<int, ErrorType>::ok(456);
    auto moved = std::move(original);
    
    EXPECT_TRUE(moved.isOk());
    EXPECT_EQ(moved.value(), 456);
}

TEST_F(ResultTypeTest, AssignmentOperator) {
    // Test assignment operator
    auto result1 = Result<int, ErrorType>::ok(100);
    auto result2 = Result<int, ErrorType>::error(ErrorType::NETWORK_ERROR);
    
    result2 = result1;
    
    EXPECT_TRUE(result2.isOk());
    EXPECT_EQ(result2.value(), 100);
}

// ========== Value Access Tests ==========

TEST_F(ResultTypeTest, ValueOrDefault) {
    // Test valueOr method with success case
    auto success = Result<int, ErrorType>::ok(42);
    EXPECT_EQ(success.valueOr(99), 42);
    
    // Test valueOr method with error case
    auto error = Result<int, ErrorType>::error(ErrorType::GPS_ERROR);
    EXPECT_EQ(error.valueOr(99), 99);
}

// ========== Monadic Operations Tests ==========

TEST_F(ResultTypeTest, MapOperation) {
    // Test map operation on success value
    auto result = Result<int, ErrorType>::ok(10);
    
    auto mapped = result.map([](int x) { return x * 2; });
    
    EXPECT_TRUE(mapped.isOk());
    EXPECT_EQ(mapped.value(), 20);
}

TEST_F(ResultTypeTest, MapOperationOnError) {
    // Test map operation on error value (should propagate error)
    auto result = Result<int, ErrorType>::error(ErrorType::HARDWARE_ERROR);
    
    auto mapped = result.map([](int x) { return x * 2; });
    
    EXPECT_TRUE(mapped.isError());
    EXPECT_EQ(mapped.error(), ErrorType::HARDWARE_ERROR);
}

TEST_F(ResultTypeTest, MapErrorOperation) {
    // Test mapError operation
    auto result = Result<int, ErrorType>::error(ErrorType::GPS_ERROR);
    
    auto mappedError = result.mapError([](ErrorType e) { return ErrorType::SYSTEM_ERROR; });
    
    EXPECT_TRUE(mappedError.isError());
    EXPECT_EQ(mappedError.error(), ErrorType::SYSTEM_ERROR);
}

TEST_F(ResultTypeTest, MapErrorOnSuccess) {
    // Test mapError operation on success value (should preserve success)
    auto result = Result<int, ErrorType>::ok(42);
    
    auto mappedError = result.mapError([](ErrorType e) { return ErrorType::SYSTEM_ERROR; });
    
    EXPECT_TRUE(mappedError.isOk());
    EXPECT_EQ(mappedError.value(), 42);
}

// ========== Chain Operations Tests ==========

TEST_F(ResultTypeTest, AndThenOperation) {
    // Test andThen operation (monadic bind)
    auto result = Result<int, ErrorType>::ok(5);
    
    auto chained = result.andThen([](int x) -> Result<int, ErrorType> {
        if (x > 0) {
            return Result<int, ErrorType>::ok(x * x);
        } else {
            return Result<int, ErrorType>::error(ErrorType::SYSTEM_ERROR);
        }
    });
    
    EXPECT_TRUE(chained.isOk());
    EXPECT_EQ(chained.value(), 25);
}

TEST_F(ResultTypeTest, AndThenOperationWithError) {
    // Test andThen operation starting with error
    auto result = Result<int, ErrorType>::error(ErrorType::NETWORK_ERROR);
    
    auto chained = result.andThen([](int x) -> Result<int, ErrorType> {
        return Result<int, ErrorType>::ok(x * x);
    });
    
    EXPECT_TRUE(chained.isError());
    EXPECT_EQ(chained.error(), ErrorType::NETWORK_ERROR);
}

TEST_F(ResultTypeTest, OrElseOperation) {
    // Test orElse operation with error (should return fallback)
    auto result = Result<int, ErrorType>::error(ErrorType::GPS_ERROR);
    auto fallback = Result<int, ErrorType>::ok(999);
    
    auto final = result.orElse(fallback);
    
    EXPECT_TRUE(final.isOk());
    EXPECT_EQ(final.value(), 999);
}

TEST_F(ResultTypeTest, OrElseOperationWithSuccess) {
    // Test orElse operation with success (should return original)
    auto result = Result<int, ErrorType>::ok(123);
    auto fallback = Result<int, ErrorType>::ok(999);
    
    auto final = result.orElse(fallback);
    
    EXPECT_TRUE(final.isOk());
    EXPECT_EQ(final.value(), 123);
}

// ========== Match Operation Tests ==========

TEST_F(ResultTypeTest, MatchOperation) {
    // Test match operation with success value
    auto success = Result<int, ErrorType>::ok(42);
    
    int matchResult = success.match(
        [](int value) { return value * 2; },    // Success handler
        [](ErrorType error) { return -1; }      // Error handler
    );
    
    EXPECT_EQ(matchResult, 84);
}

TEST_F(ResultTypeTest, MatchOperationWithError) {
    // Test match operation with error value
    auto error = Result<int, ErrorType>::error(ErrorType::SYSTEM_ERROR);
    
    int matchResult = error.match(
        [](int value) { return value * 2; },    // Success handler
        [](ErrorType error) { return -1; }      // Error handler
    );
    
    EXPECT_EQ(matchResult, -1);
}

// ========== Specialized void Result Tests ==========

TEST_F(ResultTypeTest, VoidResultSuccess) {
    // Test void Result for operations that don't return values
    auto result = Result<void, ErrorType>::ok();
    
    EXPECT_TRUE(result.isOk());
    EXPECT_FALSE(result.isError());
}

TEST_F(ResultTypeTest, VoidResultError) {
    // Test void Result with error
    auto result = Result<void, ErrorType>::error(ErrorType::HARDWARE_ERROR);
    
    EXPECT_FALSE(result.isOk());
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), ErrorType::HARDWARE_ERROR);
}

TEST_F(ResultTypeTest, VoidResultAndThen) {
    // Test andThen with void Result
    auto result = Result<void, ErrorType>::ok();
    bool chainExecuted = false;
    
    auto chained = result.andThen([&chainExecuted]() -> Result<void, ErrorType> {
        chainExecuted = true;
        return Result<void, ErrorType>::ok();
    });
    
    EXPECT_TRUE(chained.isOk());
    EXPECT_TRUE(chainExecuted);
}

// ========== System Result Types Tests ==========

TEST_F(ResultTypeTest, SystemResultType) {
    // Test using SystemResult typedef
    SystemResult success = SystemResult::ok();
    SystemResult failure = SystemResult::error(ErrorType::NETWORK_ERROR);
    
    EXPECT_TRUE(success.isOk());
    EXPECT_TRUE(failure.isError());
    EXPECT_EQ(failure.error(), ErrorType::NETWORK_ERROR);
}

TEST_F(ResultTypeTest, InitResultType) {
    // Test using InitResult typedef
    InitResult success = InitResult::ok(true);
    InitResult failure = InitResult::error(ErrorType::HARDWARE_ERROR);
    
    EXPECT_TRUE(success.isOk());
    EXPECT_EQ(success.value(), true);
    
    EXPECT_TRUE(failure.isError());
    EXPECT_EQ(failure.error(), ErrorType::HARDWARE_ERROR);
}

TEST_F(ResultTypeTest, StringResultType) {
    // Test using StringResult typedef
    const char* testStr = "Test String";
    StringResult success = StringResult::ok(testStr);
    StringResult failure = StringResult::error(ErrorType::SYSTEM_ERROR);
    
    EXPECT_TRUE(success.isOk());
    EXPECT_STREQ(success.value(), testStr);
    
    EXPECT_TRUE(failure.isError());
    EXPECT_EQ(failure.error(), ErrorType::SYSTEM_ERROR);
}

// ========== Utility Functions Tests ==========

TEST_F(ResultTypeTest, OkIfUtility) {
    // Test okIf utility function
    auto success = okIf<int>(true, 42, ErrorType::SYSTEM_ERROR);
    auto failure = okIf<int>(false, 42, ErrorType::SYSTEM_ERROR);
    
    EXPECT_TRUE(success.isOk());
    EXPECT_EQ(success.value(), 42);
    
    EXPECT_TRUE(failure.isError());
    EXPECT_EQ(failure.error(), ErrorType::SYSTEM_ERROR);
}

TEST_F(ResultTypeTest, OkIfVoidUtility) {
    // Test okIf utility function for void results
    auto success = okIf(true, ErrorType::SYSTEM_ERROR);
    auto failure = okIf(false, ErrorType::SYSTEM_ERROR);
    
    EXPECT_TRUE(success.isOk());
    EXPECT_TRUE(failure.isError());
    EXPECT_EQ(failure.error(), ErrorType::SYSTEM_ERROR);
}

// ========== Error Type Integration Tests ==========

TEST_F(ResultTypeTest, AllErrorTypes) {
    // Test Result with all ErrorType enum values
    struct ErrorTestCase {
        ErrorType errorType;
        const char* description;
    };
    
    ErrorTestCase testCases[] = {
        {ErrorType::NONE, "No error"},
        {ErrorType::SYSTEM_ERROR, "System error"},
        {ErrorType::HARDWARE_ERROR, "Hardware error"},
        {ErrorType::NETWORK_ERROR, "Network error"},
        {ErrorType::GPS_ERROR, "GPS error"},
        {ErrorType::CONFIG_ERROR, "Configuration error"},
        {ErrorType::STORAGE_ERROR, "Storage error"}
    };
    
    for (const auto& testCase : testCases) {
        auto result = Result<int, ErrorType>::error(testCase.errorType);
        
        EXPECT_TRUE(result.isError()) << "Failed for: " << testCase.description;
        EXPECT_EQ(result.error(), testCase.errorType) << "Failed for: " << testCase.description;
    }
}

// ========== Real-World Usage Pattern Tests ==========

TEST_F(ResultTypeTest, InitializationPattern) {
    // Test typical initialization pattern
    auto initializeComponent = [](bool shouldSucceed) -> InitResult {
        if (shouldSucceed) {
            return InitResult::ok(true);
        } else {
            return InitResult::error(ErrorType::HARDWARE_ERROR);
        }
    };
    
    auto success = initializeComponent(true);
    EXPECT_TRUE(success.isOk());
    EXPECT_TRUE(success.value());
    
    auto failure = initializeComponent(false);
    EXPECT_TRUE(failure.isError());
    EXPECT_EQ(failure.error(), ErrorType::HARDWARE_ERROR);
}

TEST_F(ResultTypeTest, ConfigurationPattern) {
    // Test configuration reading pattern
    auto readConfig = [](const char* key) -> StringResult {
        if (strcmp(key, "valid_key") == 0) {
            return StringResult::ok("config_value");
        } else {
            return StringResult::error(ErrorType::CONFIG_ERROR);
        }
    };
    
    auto validConfig = readConfig("valid_key");
    EXPECT_TRUE(validConfig.isOk());
    EXPECT_STREQ(validConfig.value(), "config_value");
    
    auto invalidConfig = readConfig("invalid_key");
    EXPECT_TRUE(invalidConfig.isError());
    EXPECT_EQ(invalidConfig.error(), ErrorType::CONFIG_ERROR);
}

TEST_F(ResultTypeTest, NetworkOperationPattern) {
    // Test network operation pattern
    auto networkRequest = [](bool networkAvailable) -> Result<int, ErrorType> {
        if (networkAvailable) {
            return Result<int, ErrorType>::ok(200);  // HTTP OK
        } else {
            return Result<int, ErrorType>::error(ErrorType::NETWORK_ERROR);
        }
    };
    
    auto success = networkRequest(true);
    EXPECT_TRUE(success.isOk());
    EXPECT_EQ(success.value(), 200);
    
    auto failure = networkRequest(false);
    EXPECT_TRUE(failure.isError());
    EXPECT_EQ(failure.error(), ErrorType::NETWORK_ERROR);
}

// ========== Chaining Multiple Operations Tests ==========

TEST_F(ResultTypeTest, ChainingMultipleOperations) {
    // Test chaining multiple Result-returning operations
    auto step1 = [](int input) -> Result<int, ErrorType> {
        if (input > 0) {
            return Result<int, ErrorType>::ok(input * 2);
        }
        return Result<int, ErrorType>::error(ErrorType::SYSTEM_ERROR);
    };
    
    auto step2 = [](int input) -> Result<int, ErrorType> {
        if (input < 100) {
            return Result<int, ErrorType>::ok(input + 10);
        }
        return Result<int, ErrorType>::error(ErrorType::CONFIG_ERROR);
    };
    
    auto step3 = [](int input) -> Result<const char*, ErrorType> {
        if (input < 50) {
            return Result<const char*, ErrorType>::ok("success");
        }
        return Result<const char*, ErrorType>::error(ErrorType::NETWORK_ERROR);
    };
    
    // Test successful chain
    auto result = Result<int, ErrorType>::ok(5)
        .andThen(step1)
        .andThen(step2)
        .andThen(step3);
    
    EXPECT_TRUE(result.isOk());
    EXPECT_STREQ(result.value(), "success");
    
    // Test chain with early failure
    auto failedResult = Result<int, ErrorType>::ok(-1)  // This will cause step1 to fail
        .andThen(step1)
        .andThen(step2)
        .andThen(step3);
    
    EXPECT_TRUE(failedResult.isError());
    EXPECT_EQ(failedResult.error(), ErrorType::SYSTEM_ERROR);
}

// ========== Mock Integration Tests ==========

TEST_F(ResultTypeTest, MockResultCreation) {
    // Test creating mock results using helper
    auto successResult = MockTestHelper::createMockResult(true);
    EXPECT_TRUE(successResult.isOk());
    EXPECT_TRUE(successResult.value());
    
    auto errorResult = MockTestHelper::createMockResult(false, ErrorType::GPS_ERROR);
    EXPECT_TRUE(errorResult.isError());
    EXPECT_EQ(errorResult.error(), ErrorType::GPS_ERROR);
    
    auto systemSuccess = MockTestHelper::createMockSystemResult(true);
    EXPECT_TRUE(systemSuccess.isOk());
    
    auto systemError = MockTestHelper::createMockSystemResult(false, ErrorType::HARDWARE_ERROR);
    EXPECT_TRUE(systemError.isError());
    EXPECT_EQ(systemError.error(), ErrorType::HARDWARE_ERROR);
}

// ========== Performance Tests ==========

TEST_F(ResultTypeTest, PerformanceWithManyResults) {
    // Test performance with many Result operations
    const int numOperations = 1000;
    int successCount = 0;
    int errorCount = 0;
    
    for (int i = 0; i < numOperations; ++i) {
        auto result = Result<int, ErrorType>::ok(i);
        
        auto processed = result.map([](int x) { return x * 2; });
        
        if (processed.isOk()) {
            successCount++;
        } else {
            errorCount++;
        }
    }
    
    EXPECT_EQ(successCount, numOperations);
    EXPECT_EQ(errorCount, 0);
}

// ========== Edge Cases Tests ==========

TEST_F(ResultTypeTest, EdgeCases) {
    // Test edge cases
    
    // Zero values
    auto zeroResult = Result<int, ErrorType>::ok(0);
    EXPECT_TRUE(zeroResult.isOk());
    EXPECT_EQ(zeroResult.value(), 0);
    
    // Negative values
    auto negativeResult = Result<int, ErrorType>::ok(-42);
    EXPECT_TRUE(negativeResult.isOk());
    EXPECT_EQ(negativeResult.value(), -42);
    
    // Empty string
    auto emptyStringResult = Result<const char*, ErrorType>::ok("");
    EXPECT_TRUE(emptyStringResult.isOk());
    EXPECT_STREQ(emptyStringResult.value(), "");
}

// ========== Memory Safety Tests ==========

TEST_F(ResultTypeTest, MemorySafetyBasic) {
    // Test basic memory safety (no crashes during construction/destruction)
    {
        auto result = Result<int, ErrorType>::ok(42);
        EXPECT_TRUE(result.isOk());
    } // result goes out of scope here
    
    {
        auto result = Result<int, ErrorType>::error(ErrorType::SYSTEM_ERROR);
        EXPECT_TRUE(result.isError());
    } // result goes out of scope here
    
    // If we reach here without crashing, memory management is working
    SUCCEED();
}