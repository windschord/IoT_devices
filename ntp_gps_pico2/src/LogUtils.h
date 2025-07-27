#ifndef LOG_UTILS_H
#define LOG_UTILS_H

#include <Arduino.h>
#include "LoggingService.h"

/**
 * @brief ログ出力の統一化を行うユーティリティクラス
 * 
 * 各クラスで重複していたloggingServiceのnullチェックと
 * ログ出力処理を統一化し、DRY原則を適用する。
 */
class LogUtils {
public:
    /**
     * @brief ログサービスの安全な呼び出し（INFO レベル）
     * @param loggingService ログサービスインスタンス
     * @param component コンポーネント名
     * @param message メッセージ
     */
    static void logInfo(LoggingService* loggingService, const char* component, const char* message) {
        if (loggingService) {
            loggingService->logInfo(component, message);
        }
    }

    /**
     * @brief ログサービスの安全な呼び出し（ERROR レベル）
     * @param loggingService ログサービスインスタンス
     * @param component コンポーネント名
     * @param message メッセージ
     */
    static void logError(LoggingService* loggingService, const char* component, const char* message) {
        if (loggingService) {
            loggingService->logError(component, message);
        }
    }

    /**
     * @brief ログサービスの安全な呼び出し（WARNING レベル）
     * @param loggingService ログサービスインスタンス
     * @param component コンポーネント名
     * @param message メッセージ
     */
    static void logWarning(LoggingService* loggingService, const char* component, const char* message) {
        if (loggingService) {
            loggingService->logWarning(component, message);
        }
    }

    /**
     * @brief ログサービスの安全な呼び出し（DEBUG レベル）
     * @param loggingService ログサービスインスタンス
     * @param component コンポーネント名
     * @param message メッセージ
     */
    static void logDebug(LoggingService* loggingService, const char* component, const char* message) {
        if (loggingService) {
            loggingService->logDebug(component, message);
        }
    }

    /**
     * @brief フォーマット付きログサービスの安全な呼び出し（INFO レベル）
     * @param loggingService ログサービスインスタンス
     * @param component コンポーネント名
     * @param format フォーマット文字列
     * @param ... 可変引数
     */
    static void logInfoF(LoggingService* loggingService, const char* component, const char* format, ...) {
        if (loggingService) {
            va_list args;
            va_start(args, format);
            char buffer[256];
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);
            loggingService->logInfo(component, buffer);
        }
    }

    /**
     * @brief フォーマット付きログサービスの安全な呼び出し（ERROR レベル）
     * @param loggingService ログサービスインスタンス
     * @param component コンポーネント名
     * @param format フォーマット文字列
     * @param ... 可変引数
     */
    static void logErrorF(LoggingService* loggingService, const char* component, const char* format, ...) {
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

// 便利なマクロ定義（既存のLOG_INFO_MSG等と互換性を保つ）
#define SAFE_LOG_INFO(logger, component, message) LogUtils::logInfo(logger, component, message)
#define SAFE_LOG_ERROR(logger, component, message) LogUtils::logError(logger, component, message)
#define SAFE_LOG_WARNING(logger, component, message) LogUtils::logWarning(logger, component, message)
#define SAFE_LOG_DEBUG(logger, component, message) LogUtils::logDebug(logger, component, message)
#define SAFE_LOG_INFO_F(logger, component, format, ...) LogUtils::logInfoF(logger, component, format, __VA_ARGS__)
#define SAFE_LOG_ERROR_F(logger, component, format, ...) LogUtils::logErrorF(logger, component, format, __VA_ARGS__)

#endif // LOG_UTILS_H