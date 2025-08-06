#include <Arduino.h>
#include <unity.h>

// Mock LoggingService for testing
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
        strncpy(last_component, component, sizeof(last_component) - 1);
        strncpy(last_message, message, sizeof(last_message) - 1);
        call_count++;
        info_count++;
    }
    
    void logError(const char* component, const char* message) {
        strncpy(last_component, component, sizeof(last_component) - 1);
        strncpy(last_message, message, sizeof(last_message) - 1);
        call_count++;
        error_count++;
    }
    
    void logWarning(const char* component, const char* message) {
        strncpy(last_component, component, sizeof(last_component) - 1);
        strncpy(last_message, message, sizeof(last_message) - 1);
        call_count++;
        warning_count++;
    }
    
    void logDebug(const char* component, const char* message) {
        strncpy(last_component, component, sizeof(last_component) - 1);
        strncpy(last_message, message, sizeof(last_message) - 1);
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

// LogUtils implementation (simplified for testing)
class LogUtils {
public:
    static void logInfo(MockLoggingService* loggingService, const char* component, const char* message) {
        if (loggingService) {
            loggingService->logInfo(component, message);
        }
    }

    static void logError(MockLoggingService* loggingService, const char* component, const char* message) {
        if (loggingService) {
            loggingService->logError(component, message);
        }
    }

    static void logWarning(MockLoggingService* loggingService, const char* component, const char* message) {
        if (loggingService) {
            loggingService->logWarning(component, message);
        }
    }

    static void logDebug(MockLoggingService* loggingService, const char* component, const char* message) {
        if (loggingService) {
            loggingService->logDebug(component, message);
        }
    }

    static void logInfoF(MockLoggingService* loggingService, const char* component, const char* format, ...) {
        if (loggingService) {
            va_list args;
            va_start(args, format);
            char buffer[256];
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);
            loggingService->logInfo(component, buffer);
        }
    }

    static void logErrorF(MockLoggingService* loggingService, const char* component, const char* format, ...) {
        if (loggingService) {
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
 * @brief Test 全ログレベル・フォーマット出力
 */
void test_logutils_all_log_levels() {
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
    
    // Test WARNING level
    LogUtils::logWarning(&mockLogger, "WARN_COMP", "Warning message");
    TEST_ASSERT_EQUAL_STRING("WARN_COMP", mockLogger.last_component);
    TEST_ASSERT_EQUAL_STRING("Warning message", mockLogger.last_message);
    TEST_ASSERT_EQUAL(1, mockLogger.warning_count);
    
    // Test DEBUG level
    LogUtils::logDebug(&mockLogger, "DEBUG_COMP", "Debug message");
    TEST_ASSERT_EQUAL_STRING("DEBUG_COMP", mockLogger.last_component);
    TEST_ASSERT_EQUAL_STRING("Debug message", mockLogger.last_message);
    TEST_ASSERT_EQUAL(1, mockLogger.debug_count);
    
    // Total call count should be 4
    TEST_ASSERT_EQUAL(4, mockLogger.call_count);
}

/**
 * @brief Test フォーマット付きログ出力
 */
void test_logutils_formatted_output() {
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
 * @brief Test null LoggingService処理
 */
void test_logutils_null_service() {
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
 * @brief Test 高頻度ログ出力
 */
void test_logutils_high_frequency() {
    mockLogger.reset();
    
    // High frequency logging test
    for (int i = 0; i < 100; i++) {
        if (i % 4 == 0) {
            LogUtils::logInfo(&mockLogger, "PERF", "High freq info");
        } else if (i % 4 == 1) {
            LogUtils::logError(&mockLogger, "PERF", "High freq error");
        } else if (i % 4 == 2) {
            LogUtils::logWarning(&mockLogger, "PERF", "High freq warning");
        } else {
            LogUtils::logDebug(&mockLogger, "PERF", "High freq debug");
        }
    }
    
    // Should handle high frequency logging
    TEST_ASSERT_EQUAL(100, mockLogger.call_count);
    TEST_ASSERT_EQUAL(25, mockLogger.info_count);
    TEST_ASSERT_EQUAL(25, mockLogger.error_count);
    TEST_ASSERT_EQUAL(25, mockLogger.warning_count);
    TEST_ASSERT_EQUAL(25, mockLogger.debug_count);
}

/**
 * @brief Test バッファオーバーフロー処理
 */
void test_logutils_buffer_overflow() {
    mockLogger.reset();
    
    // Test with very long message (potential buffer overflow)
    char long_message[512];
    memset(long_message, 'A', sizeof(long_message) - 1);
    long_message[sizeof(long_message) - 1] = '\0';
    
    LogUtils::logInfo(&mockLogger, "OVERFLOW_TEST", long_message);
    
    // Should handle long messages gracefully
    TEST_ASSERT_EQUAL_STRING("OVERFLOW_TEST", mockLogger.last_component);
    TEST_ASSERT_EQUAL(1, mockLogger.info_count);
    
    // Test with null component
    LogUtils::logInfo(&mockLogger, nullptr, "Message with null component");
    TEST_ASSERT_EQUAL(2, mockLogger.info_count); // Should still work
    
    // Test with null message
    LogUtils::logInfo(&mockLogger, "COMPONENT", nullptr);
    TEST_ASSERT_EQUAL(3, mockLogger.info_count); // Should still work
}

void setUp(void) {
    mockLogger.reset();
}

void tearDown(void) {
    // Cleanup
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_logutils_all_log_levels);
    RUN_TEST(test_logutils_formatted_output);
    RUN_TEST(test_logutils_null_service);
    RUN_TEST(test_logutils_high_frequency);
    RUN_TEST(test_logutils_buffer_overflow);
    
    return UNITY_END();
}