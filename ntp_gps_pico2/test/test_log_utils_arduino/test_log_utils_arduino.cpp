#include <unity.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// Mock LoggingService for testing (Simple Test Design Pattern)
class MockLoggingService {
public:
    char last_component[32] = {0};
    char last_message[256] = {0};
    int call_count = 0;
    int info_count = 0;
    int error_count = 0;
    int warning_count = 0;
    int debug_count = 0;
    
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
    
    void logWarning(const char* component, const char* message) {
        if (component) strncpy(last_component, component, sizeof(last_component) - 1);
        if (message) strncpy(last_message, message, sizeof(last_message) - 1);
        call_count++;
        warning_count++;
    }
    
    void logDebug(const char* component, const char* message) {
        if (component) strncpy(last_component, component, sizeof(last_component) - 1);
        if (message) strncpy(last_message, message, sizeof(last_message) - 1);
        call_count++;
        debug_count++;
    }
    
    void reset() {
        memset(last_component, 0, sizeof(last_component));
        memset(last_message, 0, sizeof(last_message));
        call_count = 0;
        info_count = 0;
        error_count = 0;
        warning_count = 0;
        debug_count = 0;
    }
};

// LogUtils implementation for testing (Simple Test Design Pattern)
class LogUtils {
public:
    static void logInfo(MockLoggingService* loggingService, const char* component, const char* message) {
        if (loggingService && component && message) {
            loggingService->logInfo(component, message);
        }
    }

    static void logError(MockLoggingService* loggingService, const char* component, const char* message) {
        if (loggingService && component && message) {
            loggingService->logError(component, message);
        }
    }

    static void logWarning(MockLoggingService* loggingService, const char* component, const char* message) {
        if (loggingService && component && message) {
            loggingService->logWarning(component, message);
        }
    }

    static void logDebug(MockLoggingService* loggingService, const char* component, const char* message) {
        if (loggingService && component && message) {
            loggingService->logDebug(component, message);
        }
    }

    static void logInfoF(MockLoggingService* loggingService, const char* component, const char* format, ...) {
        if (loggingService && component && format) {
            va_list args;
            va_start(args, format);
            char buffer[256];
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);
            loggingService->logInfo(component, buffer);
        }
    }

    static void logErrorF(MockLoggingService* loggingService, const char* component, const char* format, ...) {
        if (loggingService && component && format) {
            va_list args;
            va_start(args, format);
            char buffer[256];
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);
            loggingService->logError(component, buffer);
        }
    }
};

MockLoggingService mockLogger;

/**
 * @brief Test basic log levels
 */
void test_logutils_basic_log_levels() {
    mockLogger.reset();
    
    // Test INFO level
    LogUtils::logInfo(&mockLogger, "TEST", "Info message");
    TEST_ASSERT_EQUAL_STRING("TEST", mockLogger.last_component);
    TEST_ASSERT_EQUAL_STRING("Info message", mockLogger.last_message);
    TEST_ASSERT_EQUAL(1, mockLogger.info_count);
    
    // Test ERROR level
    LogUtils::logError(&mockLogger, "ERROR_COMP", "Error message");
    TEST_ASSERT_EQUAL_STRING("ERROR_COMP", mockLogger.last_component);
    TEST_ASSERT_EQUAL_STRING("Error message", mockLogger.last_message);
    TEST_ASSERT_EQUAL(1, mockLogger.error_count);
    
    TEST_ASSERT_EQUAL(2, mockLogger.call_count);
}

/**
 * @brief Test formatted logging
 */
void test_logutils_formatted_logging() {
    mockLogger.reset();
    
    // Test formatted INFO log
    LogUtils::logInfoF(&mockLogger, "FORMAT_TEST", "Value: %d", 42);
    TEST_ASSERT_EQUAL_STRING("FORMAT_TEST", mockLogger.last_component);
    TEST_ASSERT_EQUAL(1, mockLogger.info_count);
    
    // Test formatted ERROR log
    LogUtils::logErrorF(&mockLogger, "ERROR_FORMAT", "Error code: %d", 500);
    TEST_ASSERT_EQUAL_STRING("ERROR_FORMAT", mockLogger.last_component);
    TEST_ASSERT_EQUAL(1, mockLogger.error_count);
    
    TEST_ASSERT_EQUAL(2, mockLogger.call_count);
}

/**
 * @brief Test null service handling
 */
void test_logutils_null_service_handling() {
    mockLogger.reset();
    
    // Test with null logging service - should not crash
    LogUtils::logInfo(nullptr, "NULL_TEST", "This should not crash");
    LogUtils::logError(nullptr, "NULL_TEST", "This should not crash");
    LogUtils::logWarning(nullptr, "NULL_TEST", "This should not crash");
    LogUtils::logDebug(nullptr, "NULL_TEST", "This should not crash");
    
    // Test formatted logging with null service
    LogUtils::logInfoF(nullptr, "NULL_FORMAT", "Value: %d", 42);
    LogUtils::logErrorF(nullptr, "NULL_FORMAT", "Error: %s", "test error");
    
    // Mock logger should still be at reset state (0 calls)
    TEST_ASSERT_EQUAL(0, mockLogger.call_count);
}

/**
 * @brief Test null parameter handling
 */
void test_logutils_null_parameter_handling() {
    mockLogger.reset();
    
    // Test with null component
    LogUtils::logInfo(&mockLogger, nullptr, "Message with null component");
    TEST_ASSERT_EQUAL(0, mockLogger.info_count); // Should not log
    
    // Test with null message
    LogUtils::logInfo(&mockLogger, "COMPONENT", nullptr);
    TEST_ASSERT_EQUAL(0, mockLogger.info_count); // Should not log
    
    // Test with null format
    LogUtils::logInfoF(&mockLogger, "FORMAT_TEST", nullptr);
    TEST_ASSERT_EQUAL(0, mockLogger.info_count); // Should not log
    
    TEST_ASSERT_EQUAL(0, mockLogger.call_count);
}

/**
 * @brief Test multiple log levels
 */
void test_logutils_multiple_log_levels() {
    mockLogger.reset();
    
    LogUtils::logDebug(&mockLogger, "DEBUG", "Debug message");
    LogUtils::logInfo(&mockLogger, "INFO", "Info message");
    LogUtils::logWarning(&mockLogger, "WARNING", "Warning message");
    LogUtils::logError(&mockLogger, "ERROR", "Error message");
    
    TEST_ASSERT_EQUAL(1, mockLogger.debug_count);
    TEST_ASSERT_EQUAL(1, mockLogger.info_count);
    TEST_ASSERT_EQUAL(1, mockLogger.warning_count);
    TEST_ASSERT_EQUAL(1, mockLogger.error_count);
    TEST_ASSERT_EQUAL(4, mockLogger.call_count);
}

void setUp(void) {
    mockLogger.reset();
}

void tearDown(void) {
    // Cleanup
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_logutils_basic_log_levels);
    RUN_TEST(test_logutils_formatted_logging);
    RUN_TEST(test_logutils_null_service_handling);
    RUN_TEST(test_logutils_null_parameter_handling);
    RUN_TEST(test_logutils_multiple_log_levels);
    
    return UNITY_END();
}