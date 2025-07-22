#ifndef SYSTEM_CONTROLLER_H
#define SYSTEM_CONTROLLER_H

#include <Arduino.h>
#include "SystemTypes.h"
#include "TimeManager.h"
#include "NetworkManager.h"
#include "SystemMonitor.h"
#include "NtpServer.h"
#include "DisplayManager.h"
#include "ConfigManager.h"
#include "LoggingService.h"
#include "PrometheusMetrics.h"

// システム全体の状態を管理する列挙型
enum class SystemState {
    INITIALIZING,     // システム初期化中
    STARTUP,          // 起動処理中
    RUNNING,          // 正常運転中
    DEGRADED,         // 一部機能が利用不可
    ERROR,            // エラー状態
    RECOVERY,         // 復旧処理中
    SHUTDOWN          // シャットダウン中
};

// 各サービスの健全性状態
enum class ServiceHealth {
    HEALTHY,          // 正常
    WARNING,          // 警告
    CRITICAL,         // 重大
    UNKNOWN           // 不明
};

// サービス状態の構造体
struct ServiceStatus {
    ServiceHealth health;
    const char* name;
    const char* description;
    unsigned long lastCheck;
    bool enabled;
    unsigned long errorCount;
};

// システム健全性スコア
struct SystemHealthScore {
    uint8_t overall;          // 全体スコア (0-100)
    uint8_t gps;             // GPS健全性
    uint8_t network;         // ネットワーク健全性
    uint8_t ntp;             // NTP健全性
    uint8_t hardware;        // ハードウェア健全性
    unsigned long timestamp; // 最終更新時刻
};

class SystemController {
private:
    // システム状態
    SystemState currentState;
    SystemState previousState;
    unsigned long stateChangedTime;
    
    // サービス状態
    ServiceStatus services[8]; // GPS, Network, NTP, Display, Config, Logging, Metrics, Hardware
    SystemHealthScore healthScore;
    
    // 依存サービスへの参照
    TimeManager* timeManager;
    NetworkManager* networkManager;
    SystemMonitor* systemMonitor;
    NtpServer* ntpServer;
    DisplayManager* displayManager;
    ConfigManager* configManager;
    LoggingService* loggingService;
    PrometheusMetrics* prometheusMetrics;
    
    // 初期化順序管理
    bool initializationComplete;
    unsigned long initStartTime;
    uint8_t initPhase;
    
    // エラー管理
    unsigned long lastHealthCheck;
    unsigned long healthCheckInterval;
    bool autoRecoveryEnabled;
    unsigned long lastRecoveryAttempt;
    
    // ハードウェア監視
    bool gpsConnected;
    bool networkConnected;
    bool displayConnected;
    
    // 内部メソッド
    void initializeServices();
    void checkServiceHealth();
    void updateHealthScore();
    void handleStateTransition(SystemState newState);
    void performRecoveryActions();
    bool validateSystemIntegrity();
    void logSystemState();
    
    // 個別サービス健全性チェック
    ServiceHealth checkGpsHealth();
    ServiceHealth checkNetworkHealth();
    ServiceHealth checkNtpHealth();
    ServiceHealth checkHardwareHealth();
    ServiceHealth checkDisplayHealth();
    ServiceHealth checkConfigHealth();
    ServiceHealth checkLoggingHealth();
    ServiceHealth checkMetricsHealth();

public:
    SystemController();
    
    // 初期化と設定
    void init();
    void setServices(TimeManager* tm, NetworkManager* nm, SystemMonitor* sm,
                    NtpServer* ntp, DisplayManager* dm, ConfigManager* cm,
                    LoggingService* ls, PrometheusMetrics* pm);
    
    // システム制御
    void update();
    void shutdown();
    void restart();
    void emergencyStop();
    
    // 状態管理
    SystemState getState() const { return currentState; }
    bool isHealthy() const;
    bool isRunning() const { return currentState == SystemState::RUNNING; }
    bool isInitialized() const { return initializationComplete; }
    
    // 健全性監視
    const SystemHealthScore& getHealthScore() const { return healthScore; }
    const ServiceStatus* getServiceStatus() const { return services; }
    ServiceHealth getServiceHealth(const char* serviceName);
    
    // エラー処理
    void reportError(const char* service, const char* error);
    void requestRecovery(const char* service);
    void enableAutoRecovery(bool enable) { autoRecoveryEnabled = enable; }
    
    // 統計情報
    unsigned long getUptime() const;
    unsigned long getStateTime() const;
    unsigned long getErrorCount() const;
    
    // ハードウェア状態更新
    void updateGpsStatus(bool connected) { gpsConnected = connected; }
    void updateNetworkStatus(bool connected) { networkConnected = connected; }
    void updateDisplayStatus(bool connected) { displayConnected = connected; }
    
    // デバッグ・診断
    void printSystemStatus();
    void printServiceStatus();
    void generateHealthReport(char* buffer, size_t bufferSize);
};

#endif // SYSTEM_CONTROLLER_H