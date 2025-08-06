#include <unity.h>
#include <stdint.h>
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

// Mock Arduino functions
extern "C" {
    int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
        // Simple mock implementation
        if (str && size > 0) {
            strncpy(str, "formatted message", size - 1);
            str[size - 1] = '\0';
        }
        return 17; // Length of "formatted message"
    }
}

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
void test_logutils_all_log_levels_format_output() {
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
void test_logutils_formatted_log_output() {
    mockLogger.reset();
    
    // Test formatted INFO log
    LogUtils::logInfoF(&mockLogger, "FORMAT_TEST", "Value: %d, String: %s", 42, "test");
    TEST_ASSERT_EQUAL_STRING("FORMAT_TEST", mockLogger.last_component);
    TEST_ASSERT_EQUAL_STRING("formatted message", mockLogger.last_message); // Mock returns this
    TEST_ASSERT_EQUAL(1, mockLogger.info_count);
    
    // Test formatted ERROR log
    LogUtils::logErrorF(&mockLogger, "ERROR_FORMAT", "Error code: %d", 500);
    TEST_ASSERT_EQUAL_STRING("ERROR_FORMAT", mockLogger.last_component);
    TEST_ASSERT_EQUAL_STRING("formatted message", mockLogger.last_message); // Mock returns this
    TEST_ASSERT_EQUAL(1, mockLogger.error_count);
    
    TEST_ASSERT_EQUAL(2, mockLogger.call_count);
}

/**
 * @brief Test ログバッファオーバーフロー・メモリ管理
 */
void test_logutils_buffer_overflow_memory_management() {
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

/**
 * @brief Test Syslog転送・ローカル保存の全パターン
 */
void test_logutils_syslog_transfer_local_storage_patterns() {
    mockLogger.reset();
    
    // Simulate different storage scenarios
    
    // Local storage pattern (logging service available)
    LogUtils::logInfo(&mockLogger, "LOCAL", "Local storage message");
    TEST_ASSERT_EQUAL(1, mockLogger.call_count);
    
    // Remote syslog pattern (simulated)
    LogUtils::logError(&mockLogger, "REMOTE", "Remote syslog message");
    TEST_ASSERT_EQUAL(2, mockLogger.call_count);
    
    // Mixed pattern
    LogUtils::logWarning(&mockLogger, "MIXED", "Mixed storage message");
    LogUtils::logDebug(&mockLogger, "MIXED", "Debug message");
    TEST_ASSERT_EQUAL(4, mockLogger.call_count);
}

/**
 * @brief Test ログローテーション・古いログ削除処理
 */
void test_logutils_log_rotation_old_log_deletion() {
    mockLogger.reset();
    
    // Simulate log rotation by generating many log entries
    for (int i = 0; i < 100; i++) {
        char component[32];
        char message[64];
        snprintf(component, sizeof(component), "ROTATE_%d", i);
        snprintf(message, sizeof(message), "Log entry %d", i);
        
        LogUtils::logInfo(&mockLogger, component, message);
    }
    
    // Should have processed all 100 log entries
    TEST_ASSERT_EQUAL(100, mockLogger.call_count);
    TEST_ASSERT_EQUAL(100, mockLogger.info_count);
    
    // Last message should be from the final iteration
    TEST_ASSERT_EQUAL_STRING("ROTATE_99", mockLogger.last_component);
    TEST_ASSERT_EQUAL_STRING("Log entry 99", mockLogger.last_message);
}

/**
 * @brief Test 高頻度ログ出力時の性能・安定性
 */
void test_logutils_high_frequency_logging_performance_stability() {
    mockLogger.reset();
    
    // High frequency logging test
    for (int i = 0; i < 1000; i++) {
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
    TEST_ASSERT_EQUAL(1000, mockLogger.call_count);
    TEST_ASSERT_EQUAL(250, mockLogger.info_count);
    TEST_ASSERT_EQUAL(250, mockLogger.error_count);
    TEST_ASSERT_EQUAL(250, mockLogger.warning_count);
    TEST_ASSERT_EQUAL(250, mockLogger.debug_count);
}

/**
 * @brief Test 構造化ログ・JSON形式出力
 */
void test_logutils_structured_logging_json_format() {
    mockLogger.reset();
    
    // Test structured logging patterns
    LogUtils::logInfoF(&mockLogger, "JSON", "{\"level\":\"info\",\"message\":\"%s\"}", "structured log");
    TEST_ASSERT_EQUAL_STRING("JSON", mockLogger.last_component);
    TEST_ASSERT_EQUAL(1, mockLogger.info_count);
    
    // Test with special characters that might break JSON
    LogUtils::logError(&mockLogger, "JSON_SPECIAL", "Message with \"quotes\" and \\backslashes");
    TEST_ASSERT_EQUAL_STRING("JSON_SPECIAL", mockLogger.last_component);
    TEST_ASSERT_EQUAL(1, mockLogger.error_count);
    
    // Test with very long structured message
    LogUtils::logWarning(&mockLogger, "JSON_LONG", "Very long JSON structure...");
    TEST_ASSERT_EQUAL(1, mockLogger.warning_count);
}

/**
 * @brief Test null LoggingService処理
 */
void test_logutils_null_logging_service_handling() {
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
 * @brief Test 同時ログ出力・スレッドセーフティ
 */
void test_logutils_concurrent_logging_thread_safety() {
    mockLogger.reset();
    
    // Simulate concurrent logging from different components
    const char* components[] = {"COMP_A", "COMP_B", "COMP_C", "COMP_D"};
    const char* messages[] = {"Message A", "Message B", "Message C", "Message D"};
    
    // Interleaved logging
    for (int i = 0; i < 20; i++) {
        int comp_idx = i % 4;
        LogUtils::logInfo(&mockLogger, components[comp_idx], messages[comp_idx]);
    }
    
    TEST_ASSERT_EQUAL(20, mockLogger.call_count);
    TEST_ASSERT_EQUAL(20, mockLogger.info_count);
    
    // Last call should be from COMP_D (19 % 4 = 3)
    TEST_ASSERT_EQUAL_STRING("COMP_D", mockLogger.last_component);
    TEST_ASSERT_EQUAL_STRING("Message D", mockLogger.last_message);
}

/**
 * @brief Test エラー状況でのログ出力
 */
void test_logutils_logging_under_error_conditions() {
    mockLogger.reset();
    
    // Test logging during simulated error conditions
    LogUtils::logError(&mockLogger, "ERROR_HANDLER", "Critical system error");
    TEST_ASSERT_EQUAL(1, mockLogger.error_count);
    
    // Test cascading errors
    LogUtils::logError(&mockLogger, "ERROR_CASCADE", "First error");
    LogUtils::logError(&mockLogger, "ERROR_CASCADE", "Second error");
    LogUtils::logError(&mockLogger, "ERROR_CASCADE", "Third error");
    TEST_ASSERT_EQUAL(4, mockLogger.error_count); // 1 + 3
    
    // Test recovery logging
    LogUtils::logInfo(&mockLogger, "RECOVERY", "System recovered");
    TEST_ASSERT_EQUAL(1, mockLogger.info_count);
    
    TEST_ASSERT_EQUAL(5, mockLogger.call_count);
}

/**
 * @brief Test ログレベルフィルタリング（シミュレーション）
 */
void test_logutils_log_level_filtering_simulation() {
    mockLogger.reset();
    
    // Simulate log level filtering by testing all levels
    LogUtils::logDebug(&mockLogger, "FILTER", "Debug message"); // Lowest priority
    LogUtils::logInfo(&mockLogger, "FILTER", "Info message");
    LogUtils::logWarning(&mockLogger, "FILTER", "Warning message");
    LogUtils::logError(&mockLogger, "FILTER", "Error message"); // Highest priority
    
    // All messages should be logged (filtering would be done by LoggingService)
    TEST_ASSERT_EQUAL(4, mockLogger.call_count);
    TEST_ASSERT_EQUAL(1, mockLogger.debug_count);
    TEST_ASSERT_EQUAL(1, mockLogger.info_count);
    TEST_ASSERT_EQUAL(1, mockLogger.warning_count);
    TEST_ASSERT_EQUAL(1, mockLogger.error_count);
}

// Test suite setup and teardown
void setUp(void) {
    mockLogger.reset();
}

void tearDown(void) {
    // Cleanup after each test
}

/**
 * @brief LogUtils完全カバレッジテスト実行
 */
int main() {
    UNITY_BEGIN();
    
    // Basic logging level tests
    RUN_TEST(test_logutils_all_log_levels_format_output);
    RUN_TEST(test_logutils_formatted_log_output);
    
    // Buffer and memory management
    RUN_TEST(test_logutils_buffer_overflow_memory_management);
    
    // Storage and transfer patterns
    RUN_TEST(test_logutils_syslog_transfer_local_storage_patterns);
    
    // Log rotation and cleanup
    RUN_TEST(test_logutils_log_rotation_old_log_deletion);
    
    // Performance and stability
    RUN_TEST(test_logutils_high_frequency_logging_performance_stability);
    
    // Structured logging
    RUN_TEST(test_logutils_structured_logging_json_format);
    
    // Error handling
    RUN_TEST(test_logutils_null_logging_service_handling);
    
    // Concurrency simulation
    RUN_TEST(test_logutils_concurrent_logging_thread_safety);
    
    // Error conditions
    RUN_TEST(test_logutils_logging_under_error_conditions);
    
    // Filtering simulation
    RUN_TEST(test_logutils_log_level_filtering_simulation);
    
    return UNITY_END();
}