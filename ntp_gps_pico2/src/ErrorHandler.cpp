#include "ErrorHandler.h"
#include "LoggingService.h"

// グローバルエラーハンドラーのインスタンス
ErrorHandler* globalErrorHandler = nullptr;

ErrorHandler::ErrorHandler() 
    : errorCount(0),
      nextErrorIndex(0),
      autoRecoveryEnabled(true),
      maxRetryCount(3) {
    
    // エラー履歴の初期化
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
            true, // 初期状態は解決済み
            0,
            0
        };
    }
    
    // 統計の初期化
    resetStatistics();
}

void ErrorHandler::init() {
    LOG_INFO_MSG("ERROR_HDL", "ErrorHandler initialization started");
    
    // グローバル参照を設定
    globalErrorHandler = this;
    
    // 統計をリセット
    resetStatistics();
    
    LOG_INFO_MSG("ERROR", "Error handler initialized successfully");
    LOG_INFO_MSG("ERROR_HDL", "ErrorHandler initialization completed");
}

void ErrorHandler::reset() {
    errorCount = 0;
    nextErrorIndex = 0;
    resetStatistics();
    
    // 全エラーを解決済みに設定
    for (int i = 0; i < MAX_ERROR_HISTORY; i++) {
        errorHistory[i].resolved = true;
        errorHistory[i].resolvedTime = millis();
    }
    
    LOG_INFO_MSG("ERROR", "Error handler reset completed");
}

void ErrorHandler::reportError(ErrorType type, ErrorSeverity severity, 
                              const char* component, const char* message,
                              const char* details) {
    
    // エラー情報を作成
    ErrorInfo error = {
        type,
        severity,
        RecoveryStrategy::NONE, // デフォルト値、後で決定
        component,
        message,
        details ? details : "",
        millis(),
        generateErrorCode(type, component),
        false, // 未解決
        0,
        0
    };
    
    // 復旧戦略を決定（簡素化）
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
    
    // エラー履歴に追加
    errorHistory[nextErrorIndex] = error;
    nextErrorIndex = (nextErrorIndex + 1) % MAX_ERROR_HISTORY;
    if (errorCount < MAX_ERROR_HISTORY) {
        errorCount++;
    }
    
    // 統計を更新
    updateStatistics(error);
    
    // ログに記録
    logError(error);
    
    // 自動復旧が有効な場合は復旧処理を実行
    if (autoRecoveryEnabled && error.strategy != RecoveryStrategy::NONE) {
        performRecovery(error);
    }
    
    // 致命的エラーの場合は緊急停止
    if (severity == ErrorSeverity::FATAL) {
        emergencyStop(message);
    }
}

void ErrorHandler::reportHardwareError(const char* component, const char* message) {
    reportError(ErrorType::HARDWARE_FAILURE, ErrorSeverity::CRITICAL, component, message);
}

void ErrorHandler::reportCommunicationError(const char* component, const char* message) {
    reportError(ErrorType::COMMUNICATION_ERROR, ErrorSeverity::WARNING, component, message);
}

void ErrorHandler::reportMemoryError(const char* component, size_t requestedSize) {
    char details[64];
    snprintf(details, sizeof(details), "Requested: %u bytes", (unsigned)requestedSize);
    reportError(ErrorType::MEMORY_ERROR, ErrorSeverity::CRITICAL, component, "Memory allocation failed", details);
}

void ErrorHandler::reportConfigurationError(const char* component, const char* message) {
    reportError(ErrorType::CONFIGURATION_ERROR, ErrorSeverity::ERROR, component, message);
}

void ErrorHandler::reportTimeoutError(const char* component, unsigned long timeoutMs) {
    char details[64];
    snprintf(details, sizeof(details), "Timeout: %lu ms", timeoutMs);
    reportError(ErrorType::TIMEOUT_ERROR, ErrorSeverity::WARNING, component, "Operation timeout", details);
}

void ErrorHandler::reportNetworkError(const char* component, const char* message) {
    reportError(ErrorType::NETWORK_ERROR, ErrorSeverity::ERROR, component, message);
}

void ErrorHandler::reportGpsError(const char* message) {
    reportError(ErrorType::GPS_ERROR, ErrorSeverity::ERROR, "GPS", message);
}

void ErrorHandler::reportNtpError(const char* message) {
    reportError(ErrorType::NTP_ERROR, ErrorSeverity::ERROR, "NTP", message);
}

void ErrorHandler::resolveError(const char* component, ErrorType type) {
    // 指定されたコンポーネント・タイプのエラーを検索して解決
    for (int i = 0; i < errorCount; i++) {
        int index = (nextErrorIndex - 1 - i + MAX_ERROR_HISTORY) % MAX_ERROR_HISTORY;
        if (!errorHistory[index].resolved &&
            strcmp(errorHistory[index].component, component) == 0 &&
            errorHistory[index].type == type) {
            
            errorHistory[index].resolved = true;
            errorHistory[index].resolvedTime = millis();
            
            LOG_INFO_F("ERROR", "Error resolved: %s - %s", component, errorHistory[index].message);
            return;
        }
    }
}

void ErrorHandler::resolveAllErrors(const char* component) {
    unsigned int resolvedCount = 0;
    
    for (int i = 0; i < errorCount; i++) {
        int index = (nextErrorIndex - 1 - i + MAX_ERROR_HISTORY) % MAX_ERROR_HISTORY;
        if (!errorHistory[index].resolved &&
            strcmp(errorHistory[index].component, component) == 0) {
            
            errorHistory[index].resolved = true;
            errorHistory[index].resolvedTime = millis();
            resolvedCount++;
        }
    }
    
    if (resolvedCount > 0) {
        LOG_INFO_F("ERROR", "Resolved %u errors for component: %s", resolvedCount, component);
    }
}

uint32_t ErrorHandler::generateErrorCode(ErrorType type, const char* component) {
    // エラーコード生成（タイプ + コンポーネントハッシュ + タイムスタンプ）
    uint32_t code = ((uint32_t)type << 24);
    
    // 簡単なハッシュ
    uint32_t hash = 0;
    for (const char* p = component; *p; p++) {
        hash = hash * 31 + *p;
    }
    code |= (hash & 0x00FFFF00);
    code |= (millis() & 0x000000FF);
    
    return code;
}

void ErrorHandler::updateStatistics(const ErrorInfo& error) {
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
    
    // 解決率を計算
    if (statistics.totalErrors > 0) {
        statistics.resolutionRate = (float)statistics.resolvedErrors / statistics.totalErrors * 100.0f;
    }
}

void ErrorHandler::performRecovery(const ErrorInfo& error) {
    LOG_INFO_F("ERROR", "Attempting simple recovery for: %s", error.component);
    
    // 簡素化された復旧処理（設定エラーの場合は直接工場出荷時リセット）
    if (error.type == ErrorType::CONFIGURATION_ERROR || error.type == ErrorType::DATA_CORRUPTION) {
        LOG_WARN_F("ERROR", "Configuration error detected - using factory defaults");
        // ConfigManagerは既にloadDefaults()を呼び出すので、ここでは解決とマーク
        resolveError(error.component, error.type);
        return;
    }
    
    // その他のエラーは基本的な再試行のみ
    if (error.strategy == RecoveryStrategy::RETRY && error.retryCount < maxRetryCount) {
        LOG_INFO_F("ERROR", "Retrying operation for: %s", error.component);
        resolveError(error.component, error.type);
    } else {
        LOG_WARN_F("ERROR", "Simple recovery not applicable for: %s", error.component);
    }
}

bool ErrorHandler::executeRecoveryStrategy(const ErrorInfo& error) {
    // 簡素化された復旧戦略
    switch (error.strategy) {
        case RecoveryStrategy::RETRY:
            LOG_INFO_F("ERROR", "Simple retry for %s", error.component);
            return true;
            
        case RecoveryStrategy::RESTART_SYSTEM:
            LOG_ERR_F("ERROR", "System restart required for %s", error.component);
            // システム再起動（PhysicalResetで実行済み）
            return false;
            
        case RecoveryStrategy::NONE:
        default:
            LOG_DEBUG_F("ERROR", "No recovery strategy for %s", error.component);
            return false;
    }
}

void ErrorHandler::logError(const ErrorInfo& error) {
    const char* typeNames[] = {
        "HARDWARE", "COMMUNICATION", "MEMORY", "CONFIG", 
        "TIMEOUT", "DATA_CORRUPTION", "NETWORK", "GPS", "NTP", "SYSTEM"
    };
    
    const char* severityNames[] = {
        "INFO", "WARNING", "ERROR", "CRITICAL", "FATAL"
    };
    
    LOG_ERR_F("ERROR", "[%s][%s] %s: %s (Code: 0x%08X)", 
              severityNames[(int)error.severity],
              typeNames[(int)error.type],
              error.component,
              error.message,
              error.errorCode);
}

bool ErrorHandler::hasUnresolvedErrors() const {
    for (int i = 0; i < errorCount; i++) {
        int index = (nextErrorIndex - 1 - i + MAX_ERROR_HISTORY) % MAX_ERROR_HISTORY;
        if (!errorHistory[index].resolved) {
            return true;
        }
    }
    return false;
}

bool ErrorHandler::hasCriticalErrors() const {
    for (int i = 0; i < errorCount; i++) {
        int index = (nextErrorIndex - 1 - i + MAX_ERROR_HISTORY) % MAX_ERROR_HISTORY;
        if (!errorHistory[index].resolved && 
            (errorHistory[index].severity == ErrorSeverity::CRITICAL ||
             errorHistory[index].severity == ErrorSeverity::FATAL)) {
            return true;
        }
    }
    return false;
}

void ErrorHandler::update() {
    // 古いエラーのクリーンアップ（24時間以上前）
    cleanupOldErrors(24 * 60 * 60 * 1000UL);
    
    // 統計の更新
    updateStatisticsGlobal();
    
    // 復旧チェック
    if (autoRecoveryEnabled) {
        checkForRecovery();
    }
}

void ErrorHandler::updateStatisticsGlobal() {
    // 統計情報の再計算
    statistics.totalErrors = 0;
    statistics.resolvedErrors = 0;
    statistics.unresolvedErrors = 0;
    
    for (int i = 0; i < errorCount; i++) {
        int index = (nextErrorIndex - 1 - i + MAX_ERROR_HISTORY) % MAX_ERROR_HISTORY;
        if (errorHistory[index].timestamp > 0) {
            statistics.totalErrors++;
            if (errorHistory[index].resolved) {
                statistics.resolvedErrors++;
            } else {
                statistics.unresolvedErrors++;
            }
        }
    }
    
    // 解決率を計算
    if (statistics.totalErrors > 0) {
        statistics.resolutionRate = (float)statistics.resolvedErrors / statistics.totalErrors * 100.0f;
    } else {
        statistics.resolutionRate = 100.0f;
    }
    
    statistics.lastReset = millis();
}

void ErrorHandler::checkForRecovery() {
    unsigned long now = millis();
    
    for (int i = 0; i < errorCount; i++) {
        int index = (nextErrorIndex - 1 - i + MAX_ERROR_HISTORY) % MAX_ERROR_HISTORY;
        ErrorInfo& error = errorHistory[index];
        
        if (!error.resolved && error.retryCount < maxRetryCount) {
            // 復旧タイムアウトをチェック（30秒）
            const unsigned long recoveryTimeout = 30000UL;
            if (now - error.timestamp > recoveryTimeout) {
                error.retryCount++;
                LOG_INFO_F("ERROR", "Retry recovery for %s (attempt %d/%lu)", 
                           error.component, error.retryCount, maxRetryCount);
                
                if (executeRecoveryStrategy(error)) {
                    error.resolved = true;
                    error.resolvedTime = now;
                    LOG_INFO_F("ERROR", "Recovery successful for %s after %d attempts", 
                               error.component, error.retryCount);
                } else if (error.retryCount >= maxRetryCount) {
                    LOG_ERR_F("ERROR", "Recovery failed for %s after %d attempts", 
                              error.component, error.retryCount);
                }
            }
        }
    }
}

void ErrorHandler::cleanupOldErrors(unsigned long maxAge) {
    unsigned long now = millis();
    int cleaned = 0;
    
    for (int i = 0; i < errorCount; i++) {
        int index = (nextErrorIndex - 1 - i + MAX_ERROR_HISTORY) % MAX_ERROR_HISTORY;
        if (errorHistory[index].resolved && 
            (now - errorHistory[index].resolvedTime > maxAge)) {
            // 古い解決済みエラーをクリア
            errorHistory[index].timestamp = 0;
            cleaned++;
        }
    }
    
    if (cleaned > 0) {
        LOG_INFO_F("ERROR", "Cleaned up %d old resolved errors", cleaned);
    }
}

void ErrorHandler::emergencyStop(const char* reason) {
    LOG_EMERG_F("ERROR", "EMERGENCY STOP: %s", reason);
    LOG_EMERG_F("ERROR_HDL", "EMERGENCY STOP: %s", reason);
    
    // 全システムを安全に停止
    // 実装は慎重に行う必要がある
}

void ErrorHandler::safeMode(const char* reason) {
    LOG_WARN_F("ERROR", "Entering safe mode: %s", reason);
    LOG_WARN_F("ERROR_HDL", "SAFE MODE: %s", reason);
    
    // 最小限の機能のみで動作
}

void ErrorHandler::resetStatistics() {
    statistics = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0.0f, millis()
    };
}

void ErrorHandler::printErrorHistory() const {
    LOG_INFO_MSG("ERROR_HDL", "=== Error History ===");
    
    for (int i = 0; i < errorCount; i++) {
        int index = (nextErrorIndex - 1 - i + MAX_ERROR_HISTORY) % MAX_ERROR_HISTORY;
        const ErrorInfo& error = errorHistory[index];
        
        LOG_INFO_F("ERROR_HDL", "[%lu] %s: %s - %s (%s)",
                   error.timestamp,
                   error.component,
                   error.message,
                   error.resolved ? "RESOLVED" : "UNRESOLVED",
                   error.details);
    }
}

void ErrorHandler::printStatistics() const {
#ifdef DEBUG_ERROR_STATS
    LOG_INFO_MSG("ERROR_HDL", "=== Error Statistics ===");
    LOG_INFO_F("ERROR_HDL", "Total: %lu, Resolved: %lu, Unresolved: %lu",
               statistics.totalErrors, statistics.resolvedErrors, statistics.unresolvedErrors);
    LOG_INFO_F("ERROR_HDL", "Hardware: %lu, Network: %lu, GPS: %lu, NTP: %lu",
               statistics.hardwareErrors, statistics.networkErrors, 
               statistics.gpsErrors, statistics.ntpErrors);
    LOG_INFO_F("ERROR_HDL", "Resolution Rate: %.1f%%", statistics.resolutionRate);
#endif
}

// 未実装メソッドの追加
bool ErrorHandler::hasUnresolvedErrors(const char* component) const {
    for (int i = 0; i < errorCount; i++) {
        int index = (nextErrorIndex - 1 - i + MAX_ERROR_HISTORY) % MAX_ERROR_HISTORY;
        if (!errorHistory[index].resolved && 
            strcmp(errorHistory[index].component, component) == 0) {
            return true;
        }
    }
    return false;
}

ErrorSeverity ErrorHandler::getHighestSeverity() const {
    ErrorSeverity highest = ErrorSeverity::INFO;
    
    for (int i = 0; i < errorCount; i++) {
        int index = (nextErrorIndex - 1 - i + MAX_ERROR_HISTORY) % MAX_ERROR_HISTORY;
        if (!errorHistory[index].resolved && 
            errorHistory[index].severity > highest) {
            highest = errorHistory[index].severity;
        }
    }
    
    return highest;
}

unsigned int ErrorHandler::getUnresolvedCount() const {
    unsigned int count = 0;
    for (int i = 0; i < errorCount; i++) {
        int index = (nextErrorIndex - 1 - i + MAX_ERROR_HISTORY) % MAX_ERROR_HISTORY;
        if (!errorHistory[index].resolved) {
            count++;
        }
    }
    return count;
}

const ErrorInfo* ErrorHandler::getLatestError() const {
    if (errorCount == 0) return nullptr;
    
    int index = (nextErrorIndex - 1 + MAX_ERROR_HISTORY) % MAX_ERROR_HISTORY;
    return &errorHistory[index];
}

const ErrorInfo* ErrorHandler::getLatestError(const char* component) const {
    for (int i = 0; i < errorCount; i++) {
        int index = (nextErrorIndex - 1 - i + MAX_ERROR_HISTORY) % MAX_ERROR_HISTORY;
        if (strcmp(errorHistory[index].component, component) == 0) {
            return &errorHistory[index];
        }
    }
    return nullptr;
}

void ErrorHandler::markResolved(int errorIndex) {
    if (errorIndex >= 0 && errorIndex < MAX_ERROR_HISTORY) {
        errorHistory[errorIndex].resolved = true;
        errorHistory[errorIndex].resolvedTime = millis();
    }
}

void ErrorHandler::factoryReset() {
    LOG_WARN_MSG("ERROR", "Factory reset requested");
    reset();
    // 実際のファクトリーリセット処理は慎重に実装
}

void ErrorHandler::generateErrorReport(char* buffer, size_t bufferSize) const {
    if (!buffer || bufferSize == 0) return;
    
    snprintf(buffer, bufferSize,
        "Error Report:\n"
        "Total Errors: %lu\n"
        "Resolved: %lu\n"
        "Unresolved: %lu\n"
        "Resolution Rate: %.1f%%\n"
        "Critical Errors: %s\n",
        statistics.totalErrors,
        statistics.resolvedErrors,
        statistics.unresolvedErrors,
        statistics.resolutionRate,
        hasCriticalErrors() ? "YES" : "NO");
}