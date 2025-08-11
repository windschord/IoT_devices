#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <LittleFS.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include "RTClib.h"

// Forward declarations for service classes
class ConfigManager;
class TimeManager;
class NetworkManager;
class SystemMonitor;
class NtpServer;
class DisplayManager;
class SystemController;
class ErrorHandler;
class PhysicalReset;
class PowerManager;
class LoggingService;
class PrometheusMetrics;
class GpsClient;
class GpsWebServer;

/**
 * @brief システム初期化を集約管理するクラス
 * 
 * システムの全初期化処理を順序正しく実行し、
 * 依存関係の管理とエラーハンドリングを統一化する。
 */
class SystemInitializer {
public:
    /**
     * @brief 初期化結果を表す構造体
     */
    struct InitializationResult {
        bool success;
        const char* errorMessage;
        int errorCode;
        
        InitializationResult(bool s = true, const char* msg = nullptr, int code = 0)
            : success(s), errorMessage(msg), errorCode(code) {}
    };

    /**
     * @brief システム全体の初期化を実行
     * @return 初期化結果
     */
    static InitializationResult initialize();

private:
    // 初期化段階（順序重要）
    static InitializationResult initializeSerial();
    static InitializationResult initializeLEDs();
    static InitializationResult initializeI2C_OLED();
    static InitializationResult initializeFileSystem();
    static InitializationResult initializeCoreServices();
    static InitializationResult setupServiceDependencies();
    static InitializationResult initializeSystemModules();
    static InitializationResult initializeNTPServer();
    static InitializationResult initializeWebServer();
    static InitializationResult initializeGPSAndRTC();
    static InitializationResult initializePhysicalReset();
    static InitializationResult initializePowerManagement();
    static InitializationResult finalizeSystemController();

    // GPS関連初期化ヘルパー
    static bool setupGps();
    static bool setupRtc();
    static void enhanceIndoorReception();
    static void configurePPSOutput();
    static void enableAllGNSSConstellations();
    static bool enableQZSSL1S();

    // エラーハンドリングヘルパー
    static void logInitializationError(const char* component, const char* message);
    static void logInitializationSuccess(const char* component, const char* message);
};