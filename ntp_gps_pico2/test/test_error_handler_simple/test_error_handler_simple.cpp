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
    int error_count = 0;
    int warning_count = 0;
    
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
    
    void reset() {
        memset(last_component, 0, sizeof(last_component));
        memset(last_message, 0, sizeof(last_message));
        call_count = 0;
        error_count = 0;
        warning_count = 0;
    }
};

// Simple ErrorHandler implementation for testing
enum ErrorType {
    ERROR_NONE = 0,
    ERROR_GPS = 1,
    ERROR_NETWORK = 2,
    ERROR_I2C = 3,
    ERROR_CONFIG = 4
};

enum ErrorSeverity {
    SEVERITY_LOW = 1,
    SEVERITY_MEDIUM = 2,
    SEVERITY_HIGH = 3,
    SEVERITY_CRITICAL = 4
};

class ErrorHandler {
public:
    static void reportError(MockLoggingService* logger, ErrorType error_type, ErrorSeverity severity, const char* message) {
        if (!logger || !message) return;
        
        const char* type_str = getErrorTypeName(error_type);
        const char* severity_str = getSeverityName(severity);
        
        char formatted_msg[256];
        snprintf(formatted_msg, sizeof(formatted_msg), "[%s:%s] %s", 
                type_str, severity_str, message);
        
        if (severity >= SEVERITY_HIGH) {
            logger->logError("ERROR_HANDLER", formatted_msg);
        } else {
            logger->logWarning("ERROR_HANDLER", formatted_msg);
        }
    }
    
    static const char* getErrorTypeName(ErrorType error_type) {
        switch (error_type) {
            case ERROR_GPS: return "GPS";
            case ERROR_NETWORK: return "NETWORK";
            case ERROR_I2C: return "I2C";
            case ERROR_CONFIG: return "CONFIG";
            default: return "UNKNOWN";
        }
    }
    
    static const char* getSeverityName(ErrorSeverity severity) {
        switch (severity) {
            case SEVERITY_LOW: return "LOW";
            case SEVERITY_MEDIUM: return "MEDIUM";
            case SEVERITY_HIGH: return "HIGH";
            case SEVERITY_CRITICAL: return "CRITICAL";
            default: return "UNKNOWN";
        }
    }
};

MockLoggingService mockLogger;

/**
 * @brief Test basic error reporting functionality
 */
void test_error_handler_basic_functionality() {
    mockLogger.reset();
    
    // Test high severity error (should use logError)
    ErrorHandler::reportError(&mockLogger, ERROR_GPS, SEVERITY_HIGH, "GPS signal lost");
    TEST_ASSERT_EQUAL_STRING("ERROR_HANDLER", mockLogger.last_component);
    TEST_ASSERT_TRUE(strstr(mockLogger.last_message, "GPS:HIGH") != NULL);
    TEST_ASSERT_TRUE(strstr(mockLogger.last_message, "GPS signal lost") != NULL);
    TEST_ASSERT_EQUAL(1, mockLogger.error_count);
    TEST_ASSERT_EQUAL(0, mockLogger.warning_count);
    
    // Test low severity error (should use logWarning)
    ErrorHandler::reportError(&mockLogger, ERROR_NETWORK, SEVERITY_LOW, "Connection slow");
    TEST_ASSERT_EQUAL_STRING("ERROR_HANDLER", mockLogger.last_component);
    TEST_ASSERT_TRUE(strstr(mockLogger.last_message, "NETWORK:LOW") != NULL);
    TEST_ASSERT_TRUE(strstr(mockLogger.last_message, "Connection slow") != NULL);
    TEST_ASSERT_EQUAL(1, mockLogger.error_count);
    TEST_ASSERT_EQUAL(1, mockLogger.warning_count);
    
    TEST_ASSERT_EQUAL(2, mockLogger.call_count);
}

/**
 * @brief Test error type classification
 */
void test_error_handler_error_type_classification() {
    mockLogger.reset();
    
    // Test different error types
    ErrorHandler::reportError(&mockLogger, ERROR_I2C, SEVERITY_CRITICAL, "I2C bus failure");
    TEST_ASSERT_TRUE(strstr(mockLogger.last_message, "I2C:CRITICAL") != NULL);
    
    ErrorHandler::reportError(&mockLogger, ERROR_CONFIG, SEVERITY_MEDIUM, "Config validation failed");
    TEST_ASSERT_TRUE(strstr(mockLogger.last_message, "CONFIG:MEDIUM") != NULL);
    
    TEST_ASSERT_EQUAL(2, mockLogger.call_count);
}

/**
 * @brief Test null pointer handling
 */
void test_error_handler_null_handling() {
    mockLogger.reset();
    
    // Test with null logger - should not crash
    ErrorHandler::reportError(nullptr, ERROR_GPS, SEVERITY_HIGH, "Test message");
    
    // Test with null message - should not crash
    ErrorHandler::reportError(&mockLogger, ERROR_GPS, SEVERITY_HIGH, nullptr);
    
    // No calls should be recorded for null inputs
    TEST_ASSERT_EQUAL(0, mockLogger.call_count);
}

/**
 * @brief Test severity threshold behavior
 */
void test_error_handler_severity_threshold() {
    mockLogger.reset();
    
    // Test all severity levels
    ErrorHandler::reportError(&mockLogger, ERROR_GPS, SEVERITY_LOW, "Low severity");
    TEST_ASSERT_EQUAL(0, mockLogger.error_count);
    TEST_ASSERT_EQUAL(1, mockLogger.warning_count);
    
    ErrorHandler::reportError(&mockLogger, ERROR_GPS, SEVERITY_MEDIUM, "Medium severity");
    TEST_ASSERT_EQUAL(0, mockLogger.error_count);
    TEST_ASSERT_EQUAL(2, mockLogger.warning_count);
    
    ErrorHandler::reportError(&mockLogger, ERROR_GPS, SEVERITY_HIGH, "High severity");
    TEST_ASSERT_EQUAL(1, mockLogger.error_count);
    TEST_ASSERT_EQUAL(2, mockLogger.warning_count);
    
    ErrorHandler::reportError(&mockLogger, ERROR_GPS, SEVERITY_CRITICAL, "Critical severity");
    TEST_ASSERT_EQUAL(2, mockLogger.error_count);
    TEST_ASSERT_EQUAL(2, mockLogger.warning_count);
}

void setUp(void) {
    mockLogger.reset();
}

void tearDown(void) {
    // Cleanup
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_error_handler_basic_functionality);
    RUN_TEST(test_error_handler_error_type_classification);
    RUN_TEST(test_error_handler_null_handling);
    RUN_TEST(test_error_handler_severity_threshold);
    
    return UNITY_END();
}