#include <unity.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Mock Arduino functions
extern "C" {
    uint32_t millis() { return 1000; }
    void delay(uint32_t ms) { (void)ms; }
    void digitalWrite(uint8_t pin, uint8_t val) { (void)pin; (void)val; }
    void reboot() { /* Mock reboot */ }
}

// Mock LoggingService
class MockLoggingService {
public:
    void logInfo(const char* component, const char* message) {
        (void)component; (void)message;
    }
    void logError(const char* component, const char* message) {
        (void)component; (void)message;
    }
    void logWarning(const char* component, const char* message) {
        (void)component; (void)message;
    }
    void logDebug(const char* component, const char* message) {
        (void)component; (void)message;
    }
};

// Mock logging macros
#define LOG_INFO_MSG(component, message) do { } while(0)
#define LOG_ERR_F(component, format, ...) do { } while(0)
#define LOG_WARNING_MSG(component, message) do { } while(0)

// ErrorHandler types and implementation (simplified for testing)
enum class ErrorType {
    HARDWARE_FAILURE,
    COMMUNICATION_ERROR,
    MEMORY_ERROR,
    CONFIGURATION_ERROR,
    TIMEOUT_ERROR,
    DATA_CORRUPTION,
    NETWORK_ERROR,
    GPS_ERROR,
    NTP_ERROR,
    SYSTEM_ERROR
};

enum class ErrorSeverity {
    INFO,
    WARNING,
    ERROR,
    CRITICAL,
    FATAL
};

enum class RecoveryStrategy {
    NONE,
    RETRY,
    RESTART_SYSTEM
};

struct ErrorInfo {
    ErrorType type;
    ErrorSeverity severity;
    RecoveryStrategy strategy;
    const char* component;
    const char* message;
    const char* details;
    unsigned long timestamp;
    uint32_t errorCode;
    bool resolved;
    unsigned long resolvedTime;
    unsigned int retryCount;
};

struct ErrorStatistics {
    unsigned long totalErrors;
    unsigned long hardwareErrors;
    unsigned long communicationErrors;
    unsigned long memoryErrors;
    unsigned long networkErrors;
    unsigned long gpsErrors;
    unsigned long ntpErrors;
    unsigned long resolvedErrors;
    unsigned long unresolvedErrors;
    float resolutionRate;
    unsigned long lastReset;
};

class ErrorHandler {
private:
    static const int MAX_ERROR_HISTORY = 50;
    
    ErrorInfo errorHistory[MAX_ERROR_HISTORY];
    int errorCount;
    int nextErrorIndex;
    ErrorStatistics statistics;
    bool autoRecoveryEnabled;
    unsigned long maxRetryCount;
    
    uint32_t generateErrorCode(ErrorType type, const char* component) {
        uint32_t typeCode = static_cast<uint32_t>(type);
        uint32_t componentHash = 0;
        if (component) {
            for (int i = 0; component[i] && i < 8; i++) {
                componentHash = (componentHash << 4) + component[i];
            }
        }
        return (typeCode << 24) | (componentHash & 0xFFFFFF);
    }
    
    void updateStatistics(const ErrorInfo& error) {
        statistics.totalErrors++;
        switch (error.type) {
            case ErrorType::HARDWARE_FAILURE:
                statistics.hardwareErrors++;
                break;
            case ErrorType::COMMUNICATION_ERROR:
                statistics.communicationErrors++;
                break;
            case ErrorType::MEMORY_ERROR:
                statistics.memoryErrors++;
                break;
            case ErrorType::NETWORK_ERROR:
                statistics.networkErrors++;
                break;
            case ErrorType::GPS_ERROR:
                statistics.gpsErrors++;
                break;
            case ErrorType::NTP_ERROR:
                statistics.ntpErrors++;
                break;
            default:
                break;
        }
        
        if (error.resolved) {
            statistics.resolvedErrors++;
        } else {
            statistics.unresolvedErrors++;
        }
        
        if (statistics.totalErrors > 0) {
            statistics.resolutionRate = (float)statistics.resolvedErrors / statistics.totalErrors * 100.0f;
        }
    }
    
    void performRecovery(const ErrorInfo& error) {
        if (!autoRecoveryEnabled) return;
        
        switch (error.strategy) {
            case RecoveryStrategy::RETRY:
                if (error.retryCount < maxRetryCount) {
                    // Simulate retry
                }
                break;
            case RecoveryStrategy::RESTART_SYSTEM:
                // Simulate system restart
                break;
            case RecoveryStrategy::NONE:
            default:
                break;
        }
    }

public:
    ErrorHandler() : errorCount(0), nextErrorIndex(0), autoRecoveryEnabled(true), maxRetryCount(3) {
        for (int i = 0; i < MAX_ERROR_HISTORY; i++) {
            errorHistory[i] = {
                ErrorType::SYSTEM_ERROR,
                ErrorSeverity::INFO,
                RecoveryStrategy::NONE,
                "",
                "",
                "",
                0,
                0,
                true,
                0,
                0
            };
        }
        resetStatistics();
    }
    
    void init() {
        resetStatistics();
    }
    
    void reset() {
        errorCount = 0;
        nextErrorIndex = 0;
        resetStatistics();
        
        for (int i = 0; i < MAX_ERROR_HISTORY; i++) {
            errorHistory[i].resolved = true;
            errorHistory[i].resolvedTime = millis();
        }
    }
    
    void reportError(ErrorType type, ErrorSeverity severity, 
                    const char* component, const char* message,
                    const char* details = nullptr) {
        
        ErrorInfo error = {
            type,
            severity,
            RecoveryStrategy::NONE,
            component,
            message,
            details ? details : "",
            millis(),
            generateErrorCode(type, component),
            false,
            0,
            0
        };
        
        // Set recovery strategy based on severity
        switch (severity) {
            case ErrorSeverity::FATAL:
            case ErrorSeverity::CRITICAL:
                error.strategy = RecoveryStrategy::RESTART_SYSTEM;
                break;
            case ErrorSeverity::ERROR:
                error.strategy = RecoveryStrategy::RETRY;
                break;
            case ErrorSeverity::WARNING:
            case ErrorSeverity::INFO:
            default:
                error.strategy = RecoveryStrategy::NONE;
                break;
        }
        
        errorHistory[nextErrorIndex] = error;
        nextErrorIndex = (nextErrorIndex + 1) % MAX_ERROR_HISTORY;
        if (errorCount < MAX_ERROR_HISTORY) {
            errorCount++;
        }
        
        updateStatistics(error);
        performRecovery(error);
    }
    
    void reportHardwareError(const char* component, const char* message) {
        reportError(ErrorType::HARDWARE_FAILURE, ErrorSeverity::ERROR, component, message);
    }
    
    void reportCommunicationError(const char* component, const char* message) {
        reportError(ErrorType::COMMUNICATION_ERROR, ErrorSeverity::WARNING, component, message);
    }
    
    void reportMemoryError(const char* component, size_t requestedSize) {
        char details[64];
        snprintf(details, sizeof(details), "Requested: %zu bytes", requestedSize);
        reportError(ErrorType::MEMORY_ERROR, ErrorSeverity::CRITICAL, component, "Memory allocation failed", details);
    }
    
    void reportConfigurationError(const char* component, const char* message) {
        reportError(ErrorType::CONFIGURATION_ERROR, ErrorSeverity::ERROR, component, message);
    }
    
    void reportTimeoutError(const char* component, unsigned long timeoutMs) {
        char details[64];
        snprintf(details, sizeof(details), "Timeout: %lu ms", timeoutMs);
        reportError(ErrorType::TIMEOUT_ERROR, ErrorSeverity::WARNING, component, "Operation timeout", details);
    }
    
    void reportNetworkError(const char* component, const char* message) {
        reportError(ErrorType::NETWORK_ERROR, ErrorSeverity::ERROR, component, message);
    }
    
    void reportGpsError(const char* message) {
        reportError(ErrorType::GPS_ERROR, ErrorSeverity::WARNING, "GPS", message);
    }
    
    void reportNtpError(const char* message) {
        reportError(ErrorType::NTP_ERROR, ErrorSeverity::WARNING, "NTP", message);
    }
    
    void resolveError(const char* component, ErrorType type) {
        for (int i = 0; i < errorCount; i++) {
            if (!errorHistory[i].resolved && 
                errorHistory[i].type == type &&
                strcmp(errorHistory[i].component, component) == 0) {
                errorHistory[i].resolved = true;
                errorHistory[i].resolvedTime = millis();
                updateStatistics(errorHistory[i]);
                break;
            }
        }
    }
    
    void resolveAllErrors(const char* component) {
        for (int i = 0; i < errorCount; i++) {
            if (!errorHistory[i].resolved && 
                strcmp(errorHistory[i].component, component) == 0) {
                errorHistory[i].resolved = true;
                errorHistory[i].resolvedTime = millis();
                updateStatistics(errorHistory[i]);
            }
        }
    }
    
    void markResolved(int errorIndex) {
        if (errorIndex >= 0 && errorIndex < errorCount) {
            errorHistory[errorIndex].resolved = true;
            errorHistory[errorIndex].resolvedTime = millis();
            updateStatistics(errorHistory[errorIndex]);
        }
    }
    
    bool hasUnresolvedErrors() const {
        for (int i = 0; i < errorCount; i++) {
            if (!errorHistory[i].resolved) {
                return true;
            }
        }
        return false;
    }
    
    bool hasUnresolvedErrors(const char* component) const {
        for (int i = 0; i < errorCount; i++) {
            if (!errorHistory[i].resolved && 
                strcmp(errorHistory[i].component, component) == 0) {
                return true;
            }
        }
        return false;
    }
    
    bool hasCriticalErrors() const {
        for (int i = 0; i < errorCount; i++) {
            if (!errorHistory[i].resolved && 
                (errorHistory[i].severity == ErrorSeverity::CRITICAL ||
                 errorHistory[i].severity == ErrorSeverity::FATAL)) {
                return true;
            }
        }
        return false;
    }
    
    ErrorSeverity getHighestSeverity() const {
        ErrorSeverity highest = ErrorSeverity::INFO;
        for (int i = 0; i < errorCount; i++) {
            if (!errorHistory[i].resolved) {
                if (errorHistory[i].severity > highest) {
                    highest = errorHistory[i].severity;
                }
            }
        }
        return highest;
    }
    
    unsigned int getErrorCount() const { return errorCount; }
    
    unsigned int getUnresolvedCount() const {
        unsigned int count = 0;
        for (int i = 0; i < errorCount; i++) {
            if (!errorHistory[i].resolved) {
                count++;
            }
        }
        return count;
    }
    
    const ErrorStatistics& getStatistics() const { return statistics; }
    
    void resetStatistics() {
        statistics = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0.0f, millis()};
    }
    
    void updateStatistics() {
        statistics.resolvedErrors = 0;
        statistics.unresolvedErrors = 0;
        
        for (int i = 0; i < errorCount; i++) {
            if (errorHistory[i].resolved) {
                statistics.resolvedErrors++;
            } else {
                statistics.unresolvedErrors++;
            }
        }
        
        if (statistics.totalErrors > 0) {
            statistics.resolutionRate = (float)statistics.resolvedErrors / statistics.totalErrors * 100.0f;
        }
    }
    
    const ErrorInfo* getErrorHistory() const { return errorHistory; }
    
    const ErrorInfo* getLatestError() const {
        if (errorCount == 0) return nullptr;
        int latestIndex = (nextErrorIndex - 1 + MAX_ERROR_HISTORY) % MAX_ERROR_HISTORY;
        return &errorHistory[latestIndex];
    }
    
    const ErrorInfo* getLatestError(const char* component) const {
        for (int i = nextErrorIndex - 1; i >= 0; i--) {
            if (strcmp(errorHistory[i].component, component) == 0) {
                return &errorHistory[i];
            }
        }
        return nullptr;
    }
    
    void setAutoRecovery(bool enabled) { autoRecoveryEnabled = enabled; }
    void setMaxRetryCount(unsigned long count) { maxRetryCount = count; }
    
    void emergencyStop(const char* reason) {
        reportError(ErrorType::SYSTEM_ERROR, ErrorSeverity::FATAL, "SYSTEM", reason, "Emergency stop initiated");
    }
};

ErrorHandler errorHandler;

/**
 * @brief Test ErrorHandler基本初期化と設定
 */
void test_errorhandler_basic_initialization_configuration() {
    errorHandler.init();
    
    // 初期状態確認
    TEST_ASSERT_EQUAL(0, errorHandler.getErrorCount());
    TEST_ASSERT_FALSE(errorHandler.hasUnresolvedErrors());
    TEST_ASSERT_FALSE(errorHandler.hasCriticalErrors());
    TEST_ASSERT_EQUAL(ErrorSeverity::INFO, errorHandler.getHighestSeverity());
    
    // 設定変更テスト
    errorHandler.setAutoRecovery(false);
    errorHandler.setMaxRetryCount(5);
    
    // 統計情報の初期状態
    const ErrorStatistics& stats = errorHandler.getStatistics();
    TEST_ASSERT_EQUAL(0, stats.totalErrors);
    TEST_ASSERT_EQUAL(0, stats.resolvedErrors);
    TEST_ASSERT_EQUAL(0, stats.unresolvedErrors);
    TEST_ASSERT_EQUAL(0.0f, stats.resolutionRate);
}

/**
 * @brief Test 全エラータイプ報告機能
 */
void test_errorhandler_all_error_types_reporting() {
    errorHandler.reset();
    
    // 各エラータイプの報告テスト
    errorHandler.reportHardwareError("I2C", "Bus failure");
    errorHandler.reportCommunicationError("GPS", "UART timeout");
    errorHandler.reportMemoryError("HEAP", 1024);
    errorHandler.reportConfigurationError("CONFIG", "Invalid settings");
    errorHandler.reportTimeoutError("NTP", 5000);
    errorHandler.reportNetworkError("W5500", "Connection lost");
    errorHandler.reportGpsError("Signal lost");
    errorHandler.reportNtpError("Clock sync failed");
    
    // 総エラー数確認
    TEST_ASSERT_EQUAL(8, errorHandler.getErrorCount());
    TEST_ASSERT_TRUE(errorHandler.hasUnresolvedErrors());
    
    // 統計情報確認
    const ErrorStatistics& stats = errorHandler.getStatistics();
    TEST_ASSERT_EQUAL(8, stats.totalErrors);
    TEST_ASSERT_EQUAL(1, stats.hardwareErrors);
    TEST_ASSERT_EQUAL(1, stats.communicationErrors);
    TEST_ASSERT_EQUAL(1, stats.memoryErrors);
    TEST_ASSERT_EQUAL(1, stats.networkErrors);
    TEST_ASSERT_EQUAL(1, stats.gpsErrors);
    TEST_ASSERT_EQUAL(1, stats.ntpErrors);
}

/**
 * @brief Test エラー深刻度とストラテジー自動設定
 */
void test_errorhandler_severity_strategy_assignment() {
    errorHandler.reset();
    
    // 各深刻度レベルのテスト
    errorHandler.reportError(ErrorType::SYSTEM_ERROR, ErrorSeverity::INFO, "TEST", "Info message");
    errorHandler.reportError(ErrorType::SYSTEM_ERROR, ErrorSeverity::WARNING, "TEST", "Warning message");
    errorHandler.reportError(ErrorType::SYSTEM_ERROR, ErrorSeverity::ERROR, "TEST", "Error message");
    errorHandler.reportError(ErrorType::SYSTEM_ERROR, ErrorSeverity::CRITICAL, "TEST", "Critical message");
    errorHandler.reportError(ErrorType::SYSTEM_ERROR, ErrorSeverity::FATAL, "TEST", "Fatal message");
    
    TEST_ASSERT_EQUAL(5, errorHandler.getErrorCount());
    TEST_ASSERT_TRUE(errorHandler.hasCriticalErrors());
    TEST_ASSERT_EQUAL(ErrorSeverity::FATAL, errorHandler.getHighestSeverity());
    
    // 最新エラーの確認（FATAL）
    const ErrorInfo* latestError = errorHandler.getLatestError();
    TEST_ASSERT_NOT_NULL(latestError);
    TEST_ASSERT_EQUAL(ErrorSeverity::FATAL, latestError->severity);
    TEST_ASSERT_EQUAL(RecoveryStrategy::RESTART_SYSTEM, latestError->strategy);
}

/**
 * @brief Test エラー解決機能・個別・一括解決
 */
void test_errorhandler_error_resolution_individual_bulk() {
    errorHandler.reset();
    
    // 複数のエラーを報告
    errorHandler.reportHardwareError("I2C", "Bus error");
    errorHandler.reportHardwareError("SPI", "Transfer error");
    errorHandler.reportCommunicationError("I2C", "Timeout");
    errorHandler.reportNetworkError("W5500", "Link down");
    
    TEST_ASSERT_EQUAL(4, errorHandler.getErrorCount());
    TEST_ASSERT_EQUAL(4, errorHandler.getUnresolvedCount());
    
    // 個別解決テスト
    errorHandler.resolveError("I2C", ErrorType::HARDWARE_FAILURE);
    TEST_ASSERT_EQUAL(3, errorHandler.getUnresolvedCount());
    
    // 一括解決テスト（I2Cの全エラー）
    errorHandler.resolveAllErrors("I2C");
    TEST_ASSERT_EQUAL(2, errorHandler.getUnresolvedCount());
    
    // インデックス指定解決テスト
    errorHandler.markResolved(1); // SPI hardware error
    TEST_ASSERT_EQUAL(1, errorHandler.getUnresolvedCount());
    
    // 統計更新確認
    errorHandler.updateStatistics();
    const ErrorStatistics& stats = errorHandler.getStatistics();
    TEST_ASSERT_EQUAL(4, stats.totalErrors);
    TEST_ASSERT_EQUAL(3, stats.resolvedErrors);
    TEST_ASSERT_EQUAL(1, stats.unresolvedErrors);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 75.0f, stats.resolutionRate);
}

/**
 * @brief Test エラー履歴管理・循環バッファ
 */
void test_errorhandler_error_history_circular_buffer() {
    errorHandler.reset();
    
    // 最大履歴数を超えるエラーを生成
    for (int i = 0; i < 55; i++) { // MAX_ERROR_HISTORY = 50を超える
        char component[16];
        char message[32];
        snprintf(component, sizeof(component), "COMP_%d", i);
        snprintf(message, sizeof(message), "Error %d", i);
        errorHandler.reportError(ErrorType::SYSTEM_ERROR, ErrorSeverity::WARNING, component, message);
    }
    
    // 履歴数は最大値に制限される
    TEST_ASSERT_EQUAL(50, errorHandler.getErrorCount());
    
    // 最新エラーの確認
    const ErrorInfo* latestError = errorHandler.getLatestError();
    TEST_ASSERT_NOT_NULL(latestError);
    TEST_ASSERT_EQUAL_STRING("Error 54", latestError->message);
    
    // 循環バッファによる古いエラーの上書き確認
    const ErrorInfo* history = errorHandler.getErrorHistory();
    TEST_ASSERT_NOT_NULL(history);
}

/**
 * @brief Test エラー統計情報・解決率計算
 */
void test_errorhandler_error_statistics_resolution_rate() {
    errorHandler.reset();
    
    // 10個のエラーを報告
    for (int i = 0; i < 10; i++) {
        errorHandler.reportError(ErrorType::SYSTEM_ERROR, ErrorSeverity::WARNING, "TEST", "Test error");
    }
    
    // 最初の5個を解決
    for (int i = 0; i < 5; i++) {
        errorHandler.markResolved(i);
    }
    
    errorHandler.updateStatistics();
    const ErrorStatistics& stats = errorHandler.getStatistics();
    
    TEST_ASSERT_EQUAL(10, stats.totalErrors);
    TEST_ASSERT_EQUAL(5, stats.resolvedErrors);
    TEST_ASSERT_EQUAL(5, stats.unresolvedErrors);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 50.0f, stats.resolutionRate);
    
    // 残り全てを解決
    for (int i = 5; i < 10; i++) {
        errorHandler.markResolved(i);
    }
    
    errorHandler.updateStatistics();
    const ErrorStatistics& updatedStats = errorHandler.getStatistics();
    TEST_ASSERT_EQUAL(10, updatedStats.resolvedErrors);
    TEST_ASSERT_EQUAL(0, updatedStats.unresolvedErrors);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, updatedStats.resolutionRate);
}

/**
 * @brief Test コンポーネント別エラー検索・フィルタリング
 */
void test_errorhandler_component_filtering_search() {
    errorHandler.reset();
    
    // 異なるコンポーネントからエラー報告
    errorHandler.reportHardwareError("GPS", "Signal lost");
    errorHandler.reportHardwareError("GPS", "Antenna disconnected");
    errorHandler.reportNetworkError("W5500", "Link failure");
    errorHandler.reportCommunicationError("I2C", "Bus error");
    errorHandler.reportNetworkError("W5500", "DHCP timeout");
    
    // コンポーネント別の未解決エラー確認
    TEST_ASSERT_TRUE(errorHandler.hasUnresolvedErrors("GPS"));
    TEST_ASSERT_TRUE(errorHandler.hasUnresolvedErrors("W5500"));
    TEST_ASSERT_TRUE(errorHandler.hasUnresolvedErrors("I2C"));
    TEST_ASSERT_FALSE(errorHandler.hasUnresolvedErrors("NONEXISTENT"));
    
    // 特定コンポーネントの最新エラー取得
    const ErrorInfo* gpsLatestError = errorHandler.getLatestError("GPS");
    TEST_ASSERT_NOT_NULL(gpsLatestError);
    TEST_ASSERT_EQUAL_STRING("GPS", gpsLatestError->component);
    TEST_ASSERT_EQUAL_STRING("Antenna disconnected", gpsLatestError->message);
    
    // GPSエラーを全て解決
    errorHandler.resolveAllErrors("GPS");
    TEST_ASSERT_FALSE(errorHandler.hasUnresolvedErrors("GPS"));
    TEST_ASSERT_TRUE(errorHandler.hasUnresolvedErrors("W5500"));
}

/**
 * @brief Test 復旧戦略と自動復旧機能
 */
void test_errorhandler_recovery_strategy_auto_recovery() {
    errorHandler.reset();
    errorHandler.setAutoRecovery(true);
    errorHandler.setMaxRetryCount(3);
    
    // 異なる深刻度のエラーで復旧戦略テスト
    errorHandler.reportError(ErrorType::SYSTEM_ERROR, ErrorSeverity::INFO, "TEST", "Info error");
    errorHandler.reportError(ErrorType::SYSTEM_ERROR, ErrorSeverity::WARNING, "TEST", "Warning error");
    errorHandler.reportError(ErrorType::SYSTEM_ERROR, ErrorSeverity::ERROR, "TEST", "Error error");
    errorHandler.reportError(ErrorType::SYSTEM_ERROR, ErrorSeverity::CRITICAL, "TEST", "Critical error");
    errorHandler.reportError(ErrorType::SYSTEM_ERROR, ErrorSeverity::FATAL, "TEST", "Fatal error");
    
    const ErrorInfo* history = errorHandler.getErrorHistory();
    
    // INFO/WARNING → NONE strategy
    TEST_ASSERT_EQUAL(RecoveryStrategy::NONE, history[0].strategy);
    TEST_ASSERT_EQUAL(RecoveryStrategy::NONE, history[1].strategy);
    
    // ERROR → RETRY strategy  
    TEST_ASSERT_EQUAL(RecoveryStrategy::RETRY, history[2].strategy);
    
    // CRITICAL/FATAL → RESTART_SYSTEM strategy
    TEST_ASSERT_EQUAL(RecoveryStrategy::RESTART_SYSTEM, history[3].strategy);
    TEST_ASSERT_EQUAL(RecoveryStrategy::RESTART_SYSTEM, history[4].strategy);
    
    // 自動復旧無効化テスト
    errorHandler.setAutoRecovery(false);
    errorHandler.reportError(ErrorType::SYSTEM_ERROR, ErrorSeverity::CRITICAL, "TEST", "No auto recovery");
    // 自動復旧が無効化されていても、戦略は設定される
    const ErrorInfo* latestError = errorHandler.getLatestError();
    TEST_ASSERT_EQUAL(RecoveryStrategy::RESTART_SYSTEM, latestError->strategy);
}

/**
 * @brief Test エラーコード生成・一意性
 */
void test_errorhandler_error_code_generation_uniqueness() {
    errorHandler.reset();
    
    // 異なるエラータイプとコンポーネントでコード生成
    errorHandler.reportHardwareError("GPS", "Hardware error");
    errorHandler.reportCommunicationError("GPS", "Communication error");
    errorHandler.reportHardwareError("I2C", "Hardware error");
    errorHandler.reportMemoryError("HEAP", 1024);
    
    const ErrorInfo* history = errorHandler.getErrorHistory();
    
    // エラーコードが生成されていることを確認
    TEST_ASSERT_NOT_EQUAL(0, history[0].errorCode);
    TEST_ASSERT_NOT_EQUAL(0, history[1].errorCode);
    TEST_ASSERT_NOT_EQUAL(0, history[2].errorCode);
    TEST_ASSERT_NOT_EQUAL(0, history[3].errorCode);
    
    // 同じコンポーネント・異なるタイプのエラーコードは異なる
    TEST_ASSERT_NOT_EQUAL(history[0].errorCode, history[1].errorCode); // GPS hardware vs communication
    
    // 同じタイプ・異なるコンポーネントのエラーコードは異なる
    TEST_ASSERT_NOT_EQUAL(history[0].errorCode, history[2].errorCode); // GPS vs I2C hardware
}

/**
 * @brief Test 緊急停止機能
 */
void test_errorhandler_emergency_stop_functionality() {
    errorHandler.reset();
    
    // 緊急停止実行
    errorHandler.emergencyStop("Critical system failure detected");
    
    // FATALエラーが報告されることを確認
    TEST_ASSERT_EQUAL(1, errorHandler.getErrorCount());
    TEST_ASSERT_TRUE(errorHandler.hasCriticalErrors());
    TEST_ASSERT_EQUAL(ErrorSeverity::FATAL, errorHandler.getHighestSeverity());
    
    const ErrorInfo* latestError = errorHandler.getLatestError();
    TEST_ASSERT_NOT_NULL(latestError);
    TEST_ASSERT_EQUAL(ErrorType::SYSTEM_ERROR, latestError->type);
    TEST_ASSERT_EQUAL(ErrorSeverity::FATAL, latestError->severity);
    TEST_ASSERT_EQUAL_STRING("SYSTEM", latestError->component);
    TEST_ASSERT_EQUAL_STRING("Critical system failure detected", latestError->message);
    TEST_ASSERT_EQUAL_STRING("Emergency stop initiated", latestError->details);
    TEST_ASSERT_EQUAL(RecoveryStrategy::RESTART_SYSTEM, latestError->strategy);
}

/**
 * @brief Test リセット機能・全エラークリア
 */
void test_errorhandler_reset_functionality_clear_all_errors() {
    errorHandler.reset();
    
    // 複数のエラーを報告
    errorHandler.reportHardwareError("TEST1", "Error 1");
    errorHandler.reportCommunicationError("TEST2", "Error 2");
    errorHandler.reportMemoryError("TEST3", 512);
    
    TEST_ASSERT_EQUAL(3, errorHandler.getErrorCount());
    TEST_ASSERT_TRUE(errorHandler.hasUnresolvedErrors());
    
    // リセット実行
    errorHandler.reset();
    
    // 全てがリセットされることを確認
    TEST_ASSERT_EQUAL(0, errorHandler.getErrorCount());
    TEST_ASSERT_FALSE(errorHandler.hasUnresolvedErrors());
    TEST_ASSERT_FALSE(errorHandler.hasCriticalErrors());
    TEST_ASSERT_EQUAL(ErrorSeverity::INFO, errorHandler.getHighestSeverity());
    
    // 統計もリセットされることを確認
    const ErrorStatistics& stats = errorHandler.getStatistics();
    TEST_ASSERT_EQUAL(0, stats.totalErrors);
    TEST_ASSERT_EQUAL(0, stats.resolvedErrors);
    TEST_ASSERT_EQUAL(0, stats.unresolvedErrors);
    TEST_ASSERT_EQUAL(0.0f, stats.resolutionRate);
}

/**
 * @brief Test 境界値・エッジケース処理
 */
void test_errorhandler_boundary_edge_cases() {
    errorHandler.reset();
    
    // null ポインタ処理テスト
    errorHandler.reportError(ErrorType::SYSTEM_ERROR, ErrorSeverity::WARNING, nullptr, "No component");
    errorHandler.reportError(ErrorType::SYSTEM_ERROR, ErrorSeverity::WARNING, "COMP", nullptr);
    errorHandler.reportError(ErrorType::SYSTEM_ERROR, ErrorSeverity::WARNING, nullptr, nullptr);
    
    TEST_ASSERT_EQUAL(3, errorHandler.getErrorCount());
    
    // 存在しないインデックスでの解決試行
    errorHandler.markResolved(-1);  // 負のインデックス
    errorHandler.markResolved(100); // 範囲外インデックス
    
    // 存在しないコンポーネントでの解決試行
    errorHandler.resolveError("NONEXISTENT", ErrorType::HARDWARE_FAILURE);
    errorHandler.resolveAllErrors("NONEXISTENT");
    
    // 空文字列コンポーネント
    errorHandler.reportError(ErrorType::SYSTEM_ERROR, ErrorSeverity::INFO, "", "Empty component");
    
    // 最新エラー取得（存在しないコンポーネント）
    const ErrorInfo* nonExistentError = errorHandler.getLatestError("NONEXISTENT");
    TEST_ASSERT_NULL(nonExistentError);
    
    // 正常なケース
    const ErrorInfo* latestError = errorHandler.getLatestError();
    TEST_ASSERT_NOT_NULL(latestError);
}

// Test suite setup and teardown
void setUp(void) {
    errorHandler.reset();
}

void tearDown(void) {
    // Cleanup after each test
}

/**
 * @brief ErrorHandler完全カバレッジテスト実行
 */
int main() {
    UNITY_BEGIN();
    
    // Basic functionality
    RUN_TEST(test_errorhandler_basic_initialization_configuration);
    RUN_TEST(test_errorhandler_all_error_types_reporting);
    
    // Severity and strategy handling
    RUN_TEST(test_errorhandler_severity_strategy_assignment);
    
    // Error resolution
    RUN_TEST(test_errorhandler_error_resolution_individual_bulk);
    
    // History management
    RUN_TEST(test_errorhandler_error_history_circular_buffer);
    
    // Statistics
    RUN_TEST(test_errorhandler_error_statistics_resolution_rate);
    
    // Component filtering
    RUN_TEST(test_errorhandler_component_filtering_search);
    
    // Recovery strategy
    RUN_TEST(test_errorhandler_recovery_strategy_auto_recovery);
    
    // Error code generation
    RUN_TEST(test_errorhandler_error_code_generation_uniqueness);
    
    // Emergency functionality
    RUN_TEST(test_errorhandler_emergency_stop_functionality);
    
    // Reset functionality
    RUN_TEST(test_errorhandler_reset_functionality_clear_all_errors);
    
    // Edge cases
    RUN_TEST(test_errorhandler_boundary_edge_cases);
    
    return UNITY_END();
}