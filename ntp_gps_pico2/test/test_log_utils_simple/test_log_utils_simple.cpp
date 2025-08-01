#include <unity.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// Mock LoggingService for testing
class MockLoggingService {
public:
    char last_component[32] = {0};
    char last_message[256] = {0};
    int call_count = 0;
    int info_count = 0;
    int error_count = 0;
    
    void logInfo(const char* component, const char* message) {
        if (component) strncpy(last_component, component, sizeof(last_component) - 1);
        if (message) strncpy(last_message, message, sizeof(last_message) - 1);
        call_count++;
        info_count++;
    }
    
    void logError(const char* component, const char* message) {
        if (component) strncpy(last_component, component, sizeof(last_component) - 1);
        if (message) strncpy(last_message, message, sizeof(last_message) - 1);
        call_count++;
        error_count++;
    }
    
    void reset() {
        memset(last_component, 0, sizeof(last_component));
        memset(last_message, 0, sizeof(last_message));
        call_count = 0;
        info_count = 0;
        error_count = 0;
    }
};

// Simple LogUtils implementation for testing
class LogUtils {
public:
    static void logInfo(MockLoggingService* service, const char* component, const char* message) {
        if (service && component && message) {
            service->logInfo(component, message);
        }
    }
    
    static void logError(MockLoggingService* service, const char* component, const char* message) {
        if (service && component && message) {
            service->logError(component, message);
        }
    }
};

MockLoggingService mockLogger;

/**
 * @brief Test basic log functionality
 */
void test_logutils_basic_functionality() {
    mockLogger.reset();
    
    // Test INFO level
    LogUtils::logInfo(&mockLogger, "TEST", "Info message");
    TEST_ASSERT_EQUAL_STRING("TEST", mockLogger.last_component);
    TEST_ASSERT_EQUAL_STRING("Info message", mockLogger.last_message);
    TEST_ASSERT_EQUAL(1, mockLogger.info_count);
    
    // Test ERROR level
    LogUtils::logError(&mockLogger, "ERROR", "Error message");
    TEST_ASSERT_EQUAL_STRING("ERROR", mockLogger.last_component);
    TEST_ASSERT_EQUAL_STRING("Error message", mockLogger.last_message);
    TEST_ASSERT_EQUAL(1, mockLogger.error_count);
    
    TEST_ASSERT_EQUAL(2, mockLogger.call_count);
}

/**
 * @brief Test null pointer handling
 */
void test_logutils_null_handling() {
    mockLogger.reset();
    
    // Test with null service - should not crash
    LogUtils::logInfo(nullptr, "TEST", "Message");
    LogUtils::logError(nullptr, "TEST", "Message");
    
    // Test with null component/message - should not crash
    LogUtils::logInfo(&mockLogger, nullptr, "Message");
    LogUtils::logInfo(&mockLogger, "TEST", nullptr);
    
    // No calls should be recorded for null inputs
    TEST_ASSERT_EQUAL(0, mockLogger.call_count);
}

/**
 * @brief Test multiple calls
 */
void test_logutils_multiple_calls() {
    mockLogger.reset();
    
    // Multiple info calls
    for (int i = 0; i < 5; i++) {
        LogUtils::logInfo(&mockLogger, "MULTI", "Info message");
    }
    
    TEST_ASSERT_EQUAL(5, mockLogger.call_count);
    TEST_ASSERT_EQUAL(5, mockLogger.info_count);
    TEST_ASSERT_EQUAL(0, mockLogger.error_count);
}

void setUp(void) {
    mockLogger.reset();
}

void tearDown(void) {
    // Cleanup
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_logutils_basic_functionality);
    RUN_TEST(test_logutils_null_handling);
    RUN_TEST(test_logutils_multiple_calls);
    
    return UNITY_END();
}