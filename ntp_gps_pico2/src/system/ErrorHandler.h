#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <Arduino.h>
#include "SystemTypes.h"

// Forward declaration to avoid circular dependency
template<typename T, typename E> class Result;

// エラーの種類
enum class ErrorType {
    HARDWARE_FAILURE,      // ハードウェア故障
    COMMUNICATION_ERROR,   // 通信エラー
    MEMORY_ERROR,         // メモリ不足
    CONFIGURATION_ERROR,  // 設定エラー
    TIMEOUT_ERROR,        // タイムアウト
    DATA_CORRUPTION,      // データ破損
    NETWORK_ERROR,        // ネットワークエラー
    GPS_ERROR,            // GPS関連エラー
    NTP_ERROR,            // NTP関連エラー
    SYSTEM_ERROR          // システム全般エラー
};

// エラーの深刻度
enum class ErrorSeverity {
    INFO,                 // 情報
    WARNING,              // 警告
    ERROR,                // エラー
    CRITICAL,             // 重大
    FATAL                 // 致命的
};

// 復旧戦略（簡素化）
enum class RecoveryStrategy {
    NONE,                 // 復旧なし
    RETRY,                // 再試行
    RESTART_SYSTEM        // システム再起動
};

// エラー情報の構造体
struct ErrorInfo {
    ErrorType type;
    ErrorSeverity severity;
    RecoveryStrategy strategy;
    const char* component;     // エラー発生コンポーネント
    const char* message;       // エラーメッセージ
    const char* details;       // 詳細情報
    unsigned long timestamp;   // 発生時刻
    uint32_t errorCode;       // エラーコード
    bool resolved;            // 解決済みフラグ
    unsigned long resolvedTime; // 解決時刻
    unsigned int retryCount;   // 再試行回数
};

// エラー統計
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
    float resolutionRate;     // 解決率 (%)
    unsigned long lastReset;  // 最後のリセット時刻
};

class ErrorHandler {
private:
    static const int MAX_ERROR_HISTORY = 50;  // 最大エラー履歴数
    
    ErrorInfo errorHistory[MAX_ERROR_HISTORY];
    int errorCount;
    int nextErrorIndex;
    
    ErrorStatistics statistics;
    
    // 簡素化された自動復旧設定
    bool autoRecoveryEnabled;
    unsigned long maxRetryCount;
    
    // 内部メソッド（簡素化）
    uint32_t generateErrorCode(ErrorType type, const char* component);
    void updateStatistics(const ErrorInfo& error);
    void performRecovery(const ErrorInfo& error);
    bool executeRecoveryStrategy(const ErrorInfo& error);
    void logError(const ErrorInfo& error);

public:
    ErrorHandler();
    
    // 初期化
    void init();
    void reset();
    
    // エラー報告
    void reportError(ErrorType type, ErrorSeverity severity, 
                    const char* component, const char* message,
                    const char* details = nullptr);
    void reportHardwareError(const char* component, const char* message);
    void reportCommunicationError(const char* component, const char* message);
    void reportMemoryError(const char* component, size_t requestedSize);
    void reportConfigurationError(const char* component, const char* message);
    void reportTimeoutError(const char* component, unsigned long timeoutMs);
    void reportNetworkError(const char* component, const char* message);
    void reportGpsError(const char* message);
    void reportNtpError(const char* message);
    
    // エラー解決
    void resolveError(const char* component, ErrorType type);
    void resolveAllErrors(const char* component);
    void markResolved(int errorIndex);
    
    // 復旧処理（簡素化）
    void setAutoRecovery(bool enabled) { autoRecoveryEnabled = enabled; }
    void setMaxRetryCount(unsigned long count) { maxRetryCount = count; }
    
    // 状態確認
    bool hasUnresolvedErrors() const;
    bool hasUnresolvedErrors(const char* component) const;
    bool hasCriticalErrors() const;
    ErrorSeverity getHighestSeverity() const;
    unsigned int getErrorCount() const { return errorCount; }
    unsigned int getUnresolvedCount() const;
    
    // 統計情報
    const ErrorStatistics& getStatistics() const { return statistics; }
    void updateStatistics();
    void updateStatisticsGlobal();
    void resetStatistics();
    
    // エラー履歴
    const ErrorInfo* getErrorHistory() const { return errorHistory; }
    const ErrorInfo* getLatestError() const;
    const ErrorInfo* getLatestError(const char* component) const;
    void getErrorsByComponent(const char* component, ErrorInfo* buffer, int maxCount) const;
    void getErrorsBySeverity(ErrorSeverity severity, ErrorInfo* buffer, int maxCount) const;
    
    // 定期処理
    void update();
    void checkForRecovery();
    void cleanupOldErrors(unsigned long maxAge);
    
    // デバッグ・診断
    void printErrorHistory() const;
    void printStatistics() const;
    void generateErrorReport(char* buffer, size_t bufferSize) const;
    
    // 緊急処理
    void emergencyStop(const char* reason);
    void safeMode(const char* reason);
    void factoryReset();
    
    // Result型との統合（宣言のみ、実装はcppファイルへ）
    template<typename T>
    Result<T, ErrorType> wrapResult(T value, bool success, ErrorType errorType, const char* component, const char* message = nullptr);
    
    Result<void, ErrorType> wrapVoidResult(bool success, ErrorType errorType, const char* component, const char* message = nullptr);
    
    // エラーコンテキスト管理
    struct ErrorContext {
        const char* operation;
        const char* component;
        ErrorType fallbackType;
        
        ErrorContext(const char* op, const char* comp, ErrorType type = ErrorType::SYSTEM_ERROR) 
            : operation(op), component(comp), fallbackType(type) {}
    };
    
    template<typename T>
    Result<T, ErrorType> tryOperation(const ErrorContext& context, T (*operation)());
    
    Result<void, ErrorType> tryVoidOperation(const ErrorContext& context, void (*operation)());
};

// グローバルエラーハンドラーへの参照
extern ErrorHandler* globalErrorHandler;

// 便利なマクロ
#define REPORT_ERROR(type, component, message) \
    if (globalErrorHandler) globalErrorHandler->reportError(type, ErrorSeverity::ERROR, component, message)

#define REPORT_CRITICAL(type, component, message) \
    if (globalErrorHandler) globalErrorHandler->reportError(type, ErrorSeverity::CRITICAL, component, message)

#define REPORT_WARNING(type, component, message) \
    if (globalErrorHandler) globalErrorHandler->reportError(type, ErrorSeverity::WARNING, component, message)

#define REPORT_HW_ERROR(component, message) \
    if (globalErrorHandler) globalErrorHandler->reportHardwareError(component, message)

#define REPORT_COMM_ERROR(component, message) \
    if (globalErrorHandler) globalErrorHandler->reportCommunicationError(component, message)

#define REPORT_MEMORY_ERROR(component, size) \
    if (globalErrorHandler) globalErrorHandler->reportMemoryError(component, size)

#define REPORT_TIMEOUT_ERROR(component, timeout) \
    if (globalErrorHandler) globalErrorHandler->reportTimeoutError(component, timeout)

#define RESOLVE_ERROR(component, type) \
    if (globalErrorHandler) globalErrorHandler->resolveError(component, type)

// Result型統合マクロ（型推論を修正）
#define WRAP_RESULT(value, success, errorType, component, message) \
    (success ? Result<decltype(value), ErrorType>::ok(value) : \
     (globalErrorHandler ? (globalErrorHandler->reportError(errorType, ErrorSeverity::ERROR, component, message), \
                           Result<decltype(value), ErrorType>::error(errorType)) : \
                          Result<decltype(value), ErrorType>::error(errorType)))

#define WRAP_VOID_RESULT(success, errorType, component, message) \
    (success ? Result<void, ErrorType>::ok() : \
     (globalErrorHandler ? (globalErrorHandler->reportError(errorType, ErrorSeverity::ERROR, component, message), \
                           Result<void, ErrorType>::error(errorType)) : \
                          Result<void, ErrorType>::error(errorType)))

#define TRY_WITH_ERROR_HANDLER(result, component) \
    do { \
        auto __temp_result = (result); \
        if (__temp_result.isError() && globalErrorHandler) { \
            globalErrorHandler->reportError(__temp_result.error(), ErrorSeverity::ERROR, component, #result); \
        } \
        if (__temp_result.isError()) { \
            return typeof(__temp_result)::error(__temp_result.error()); \
        } \
    } while(0)

#define TRY_VALUE_WITH_ERROR_HANDLER(result, component) \
    ({ \
        auto __temp_result = (result); \
        if (__temp_result.isError() && globalErrorHandler) { \
            globalErrorHandler->reportError(__temp_result.error(), ErrorSeverity::ERROR, component, #result); \
        } \
        if (__temp_result.isError()) { \
            return typeof(__temp_result)::error(__temp_result.error()); \
        } \
        __temp_result.value(); \
    })

// 操作実行マクロ
#define TRY_OPERATION(operation, component, errorType) \
    (globalErrorHandler ? globalErrorHandler->tryOperation( \
        ErrorHandler::ErrorContext(#operation, component, errorType), operation) : \
     Result<typeof(operation()), ErrorType>::ok(operation()))

#define TRY_VOID_OPERATION(operation, component, errorType) \
    (globalErrorHandler ? globalErrorHandler->tryVoidOperation( \
        ErrorHandler::ErrorContext(#operation, component, errorType), operation) : \
     (operation(), Result<void, ErrorType>::ok()))

#endif // ERROR_HANDLER_H