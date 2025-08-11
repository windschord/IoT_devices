#pragma once

#include <Arduino.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include "RTClib.h"

// Include necessary headers for concrete types
#include "../config/ConfigManager.h"
#include "../gps/TimeManager.h"
#include "../network/NetworkManager.h"
#include "../system/SystemMonitor.h"
#include "../network/NtpServer.h"
#include "../display/DisplayManager.h"
#include "../system/SystemController.h"
#include "../system/ErrorHandler.h"
#include "../display/PhysicalReset.h"
#include "../system/PowerManager.h"
#include "../config/LoggingService.h"
#include "../system/PrometheusMetrics.h"
#include "../gps/GpsClient.h"
#include "../network/webserver.h"
#include "../system/SystemTypes.h"

// Forward declaration to avoid circular dependency
class ServiceContainer;

/**
 * @brief システム状態を管理するシングルトンクラス
 * 
 * 全てのグローバル変数とサービスインスタンスを統合管理し、
 * システム全体の状態へのアクセスを統一的に提供する。
 * スレッドセーフティを確保し、依存関係を明確化する。
 */
class SystemState {
public:
    /**
     * @brief シングルトンインスタンスを取得
     * @return SystemState唯一のインスタンス
     */
    static SystemState& getInstance();
    
    // コピー・代入禁止（シングルトンパターン）
    SystemState(const SystemState&) = delete;
    SystemState& operator=(const SystemState&) = delete;

    // ========== ハードウェアインスタンスアクセス ==========
    SFE_UBLOX_GNSS& getGNSS() { return myGNSS; }
    EthernetServer& getEthernetServer() { return server; }
    EthernetUDP& getNtpUdp() { return ntpUdp; }
    RTC_DS3231& getRTC() { return rtc; }

    // ========== サービスインスタンスアクセス ==========
    ConfigManager& getConfigManager() { return configManager; }
    TimeManager& getTimeManager() { return timeManager; }
    NetworkManager& getNetworkManager() { return networkManager; }
    SystemMonitor& getSystemMonitor() { return systemMonitor; }
    NtpServer& getNtpServer() { return ntpServer; }
    DisplayManager& getDisplayManager() { return displayManager; }
    SystemController& getSystemController() { return systemController; }
    ErrorHandler& getErrorHandler() { return errorHandler; }
    PhysicalReset& getPhysicalReset() { return physicalReset; }
    PowerManager& getPowerManager() { return powerManager; }
    LoggingService& getLoggingService() { return loggingService; }
    PrometheusMetrics& getPrometheusMetrics() { return prometheusMetrics; }
    GpsClient& getGpsClient() { return gpsClient; }
    GpsWebServer& getWebServer() { return webServer; }

    // ========== システム状態管理 ==========
    // PPS状態
    volatile unsigned long getLastPps() const { return lastPps; }
    void setLastPps(unsigned long value) { lastPps = value; }
    
    volatile bool isPpsReceived() const { return ppsReceived; }
    void setPpsReceived(bool value) { ppsReceived = value; }

    // GPS状態
    bool isGpsConnected() const { return gpsConnected; }
    void setGpsConnected(bool value) { gpsConnected = value; }

    // Webサーバー状態
    bool isWebServerStarted() const { return webServerStarted; }
    void setWebServerStarted(bool value) { webServerStarted = value; }

    // LED状態
    unsigned long getLastGnssLedUpdate() const { return lastGnssLedUpdate; }
    void setLastGnssLedUpdate(unsigned long value) { lastGnssLedUpdate = value; }
    
    bool getGnssLedState() const { return gnssLedState; }
    void setGnssLedState(bool value) { gnssLedState = value; }
    
    unsigned long getGnssBlinkInterval() const { return gnssBlinkInterval; }
    void setGnssBlinkInterval(unsigned long value) { gnssBlinkInterval = value; }

    // LED管理状態
    unsigned long getLedOffTime() const { return ledOffTime; }
    void setLedOffTime(unsigned long value) { ledOffTime = value; }

    // タイミング制御状態
    unsigned long getLastLowPriorityUpdate() const { return lastLowPriorityUpdate; }
    void setLastLowPriorityUpdate(unsigned long value) { lastLowPriorityUpdate = value; }
    
    unsigned long getLastMediumPriorityUpdate() const { return lastMediumPriorityUpdate; }
    void setLastMediumPriorityUpdate(unsigned long value) { lastMediumPriorityUpdate = value; }

    // TimeSync構造体アクセス
    TimeSync& getTimeSync() { return timeSync; }

    // ========== ハードウェア状態管理 ==========
    struct HardwareStatus {
        bool gpsReady = false;
        bool networkReady = false;
        bool displayReady = false;
        bool rtcReady = false;
        bool storageReady = false;
        unsigned long lastGpsUpdate = 0;
        unsigned long lastNetworkCheck = 0;
        float cpuTemperature = 0.0f;
        uint32_t freeMemory = 0;
    };
    
    HardwareStatus& getHardwareStatus() { return hardwareStatus; }
    const HardwareStatus& getHardwareStatus() const { return hardwareStatus; }
    
    // ========== システム統計情報 ==========
    struct SystemStatistics {
        unsigned long systemUptime = 0;
        unsigned long ntpRequestsTotal = 0;
        unsigned long ntpResponsesTotal = 0;
        unsigned long ntpDroppedTotal = 0;
        unsigned long gpsFixCount = 0;
        unsigned long ppsCount = 0;
        unsigned long errorCount = 0;
        unsigned long restartCount = 0;
        float averageResponseTime = 0.0f;
        float currentAccuracy = 0.0f;
    };
    
    SystemStatistics& getSystemStatistics() { return systemStatistics; }
    const SystemStatistics& getSystemStatistics() const { return systemStatistics; }
    
    // 統計更新メソッド
    void incrementNtpRequests() { systemStatistics.ntpRequestsTotal++; }
    void incrementNtpResponses() { systemStatistics.ntpResponsesTotal++; }
    void incrementNtpDropped() { systemStatistics.ntpDroppedTotal++; }
    void incrementGpsFixCount() { systemStatistics.gpsFixCount++; }
    void incrementPpsCount() { systemStatistics.ppsCount++; }
    void incrementErrorCount() { systemStatistics.errorCount++; }
    void updateResponseTime(float responseTime) {
        // 簡単な移動平均
        systemStatistics.averageResponseTime = 
            (systemStatistics.averageResponseTime * 0.9f) + (responseTime * 0.1f);
    }
    void updateAccuracy(float accuracy) { systemStatistics.currentAccuracy = accuracy; }
    
    // ========== スレッドセーフアクセス ==========
    void lockState() { /* 組み込み環境のため、割り込み制御を使用 */ noInterrupts(); }
    void unlockState() { interrupts(); }
    
    // ========== DI コンテナアクセス ==========
    /**
     * @brief DIコンテナ取得
     * @return ServiceContainerインスタンス
     */
    ServiceContainer& getServiceContainer();
    
    /**
     * @brief DIコンテナ初期化
     * ハードウェアとサービスをDIコンテナに登録
     * @return true 成功, false 失敗
     */
    bool initializeDIContainer();

    // ========== PPS割り込みコールバック ==========
    /**
     * @brief PPS割り込み処理
     * 静的メンバー関数として定義し、attachInterrupt()で使用可能
     */
    static void triggerPps();

private:
    SystemState(); // プライベートコンストラクタ
    ~SystemState() = default;

    // ========== ハードウェアインスタンス ==========
    SFE_UBLOX_GNSS myGNSS;
    EthernetServer server;
    EthernetUDP ntpUdp;
    RTC_DS3231 rtc;

    // ========== サービスインスタンス ==========
    ConfigManager configManager;
    TimeSync timeSync;
    TimeManager timeManager;
    NetworkManager networkManager;
    SystemMonitor systemMonitor;
    NtpServer ntpServer;
    DisplayManager displayManager;
    SystemController systemController;
    ErrorHandler errorHandler;
    PhysicalReset physicalReset;
    PowerManager powerManager;
    LoggingService loggingService;
    PrometheusMetrics prometheusMetrics;
    GpsClient gpsClient;
    GpsWebServer webServer;

    // ========== システム状態変数 ==========
    // PPS状態
    volatile unsigned long lastPps = 0;
    volatile bool ppsReceived = false;

    // GPS状態
    bool gpsConnected = false;
    bool webServerStarted = false;

    // LED制御状態
    unsigned long lastGnssLedUpdate = 0;
    bool gnssLedState = false;
    unsigned long gnssBlinkInterval = 0;
    unsigned long ledOffTime = 0;

    // タイミング制御
    unsigned long lastLowPriorityUpdate = 0;
    unsigned long lastMediumPriorityUpdate = 0;
    
    // ハードウェア状態
    HardwareStatus hardwareStatus;
    
    // システム統計
    SystemStatistics systemStatistics;
};