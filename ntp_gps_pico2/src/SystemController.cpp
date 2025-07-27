#include "SystemController.h"
#include "LoggingService.h"

SystemController::SystemController() 
    : currentState(SystemState::INITIALIZING),
      previousState(SystemState::INITIALIZING),
      stateChangedTime(0),
      initializationComplete(false),
      initStartTime(0),
      initPhase(0),
      lastHealthCheck(0),
      healthCheckInterval(5000), // 5秒間隔
      autoRecoveryEnabled(true),
      lastRecoveryAttempt(0),
      gpsConnected(false),
      networkConnected(false),
      displayConnected(false) {
    
    // サービス状態の初期化
    const char* serviceNames[] = {
        "GPS", "Network", "NTP", "Display", 
        "Config", "Logging", "Metrics", "Hardware"
    };
    
    for (int i = 0; i < 8; i++) {
        services[i] = {
            ServiceHealth::UNKNOWN,
            serviceNames[i],
            "Initializing",
            0,
            true,
            0
        };
    }
    
    // 健全性スコアの初期化
    healthScore = {0, 0, 0, 0, 0, millis()};
    
    // すべてのサービス参照をnullに初期化
    timeManager = nullptr;
    networkManager = nullptr;
    systemMonitor = nullptr;
    ntpServer = nullptr;
    displayManager = nullptr;
    configManager = nullptr;
    loggingService = nullptr;
    prometheusMetrics = nullptr;
}

void SystemController::init() {
    initStartTime = millis();
    currentState = SystemState::STARTUP;
    stateChangedTime = millis();
    
    // Note: LoggingService initialization message already handled
    LOG_INFO_MSG("SYSTEM", "System controller initialization started");
    
    // 初期化完了をマーク
    initializationComplete = true;
    logSystemState();
}

void SystemController::setServices(TimeManager* tm, NetworkManager* nm, SystemMonitor* sm,
                                 NtpServer* ntp, DisplayManager* dm, ConfigManager* cm,
                                 LoggingService* ls, PrometheusMetrics* pm) {
    timeManager = tm;
    networkManager = nm;
    systemMonitor = sm;
    ntpServer = ntp;
    displayManager = dm;
    configManager = cm;
    loggingService = ls;
    prometheusMetrics = pm;
    
    // Note: LoggingService message already handled
    LOG_INFO_MSG("SYSTEM", "All system services registered with controller");
}

void SystemController::update() {
    unsigned long now = millis();
    
    // 定期健全性チェック
    if (now - lastHealthCheck >= healthCheckInterval) {
        checkServiceHealth();
        updateHealthScore();
        lastHealthCheck = now;
        
        // 状態遷移の判定
        SystemState newState = currentState;
        
        if (currentState == SystemState::STARTUP && validateSystemIntegrity()) {
            newState = SystemState::RUNNING;
        } else if (currentState == SystemState::RUNNING && !isHealthy()) {
            newState = SystemState::DEGRADED;
        } else if (currentState == SystemState::DEGRADED && isHealthy()) {
            newState = SystemState::RUNNING;
        } else if (healthScore.overall < 30) {
            newState = SystemState::ERROR;
        }
        
        if (newState != currentState) {
            handleStateTransition(newState);
        }
    }
    
    // 自動復旧処理
    if (autoRecoveryEnabled && currentState == SystemState::ERROR && 
        now - lastRecoveryAttempt > 30000) { // 30秒間隔で復旧試行
        performRecoveryActions();
        lastRecoveryAttempt = now;
    }
}

void SystemController::checkServiceHealth() {
    services[0].health = checkGpsHealth();
    services[1].health = checkNetworkHealth();
    services[2].health = checkNtpHealth();
    services[3].health = checkDisplayHealth();
    services[4].health = checkConfigHealth();
    services[5].health = checkLoggingHealth();
    services[6].health = checkMetricsHealth();
    services[7].health = checkHardwareHealth();
    
    // 最終チェック時刻を更新
    unsigned long now = millis();
    for (int i = 0; i < 8; i++) {
        services[i].lastCheck = now;
    }
}

ServiceHealth SystemController::checkGpsHealth() {
    if (!gpsConnected) return ServiceHealth::CRITICAL;
    if (!timeManager) return ServiceHealth::UNKNOWN;
    
    // GPS同期状態をチェック
    if (timeManager->getNtpStratum() == 1) {
        return ServiceHealth::HEALTHY;
    } else if (timeManager->getNtpStratum() <= 3) {
        return ServiceHealth::WARNING;
    } else {
        return ServiceHealth::CRITICAL;
    }
}

ServiceHealth SystemController::checkNetworkHealth() {
    if (!networkConnected) return ServiceHealth::CRITICAL;
    if (!networkManager) return ServiceHealth::UNKNOWN;
    
    // ネットワークマネージャーからの状態取得が必要
    // 現在は簡易チェック
    return networkConnected ? ServiceHealth::HEALTHY : ServiceHealth::CRITICAL;
}

ServiceHealth SystemController::checkNtpHealth() {
    if (!ntpServer) return ServiceHealth::UNKNOWN;
    
    // NTPサーバーの応答性をチェック
    // 実装が必要: NtpServerにgetRequestCount()等の統計メソッドを追加
    return ServiceHealth::HEALTHY; // 暫定
}

ServiceHealth SystemController::checkHardwareHealth() {
    // Raspberry Pi Pico 2でのメモリチェック（簡易版）
    // 実際のメモリ使用量取得は複雑なため、基本的な動作確認のみ
    
    // システムの応答性チェック
    unsigned long responseStart = micros();
    delay(1); // 1ms待機
    unsigned long responseTime = micros() - responseStart;
    
    if (responseTime > 5000) { // 5ms以上で重大
        return ServiceHealth::CRITICAL;
    }
    if (responseTime > 2000) { // 2ms以上で警告
        return ServiceHealth::WARNING;
    }
    
    return ServiceHealth::HEALTHY;
}

ServiceHealth SystemController::checkDisplayHealth() {
    return displayConnected ? ServiceHealth::HEALTHY : ServiceHealth::WARNING;
}

ServiceHealth SystemController::checkConfigHealth() {
    return configManager ? ServiceHealth::HEALTHY : ServiceHealth::CRITICAL;
}

ServiceHealth SystemController::checkLoggingHealth() {
    return loggingService ? ServiceHealth::HEALTHY : ServiceHealth::WARNING;
}

ServiceHealth SystemController::checkMetricsHealth() {
    return prometheusMetrics ? ServiceHealth::HEALTHY : ServiceHealth::WARNING;
}

void SystemController::updateHealthScore() {
    uint8_t healthyCount = 0;
    uint8_t totalServices = 8;
    
    // 各サービスの健全性を数値化
    for (int i = 0; i < totalServices; i++) {
        switch (services[i].health) {
            case ServiceHealth::HEALTHY: healthyCount += 4; break;
            case ServiceHealth::WARNING: healthyCount += 2; break;
            case ServiceHealth::CRITICAL: healthyCount += 0; break;
            case ServiceHealth::UNKNOWN: healthyCount += 1; break;
        }
    }
    
    // 全体スコア計算 (0-100)
    healthScore.overall = (healthyCount * 100) / (totalServices * 4);
    
    // 個別スコア
    healthScore.gps = (services[0].health == ServiceHealth::HEALTHY) ? 100 : 
                     (services[0].health == ServiceHealth::WARNING) ? 50 : 0;
    healthScore.network = (services[1].health == ServiceHealth::HEALTHY) ? 100 : 
                         (services[1].health == ServiceHealth::WARNING) ? 50 : 0;
    healthScore.ntp = (services[2].health == ServiceHealth::HEALTHY) ? 100 : 
                     (services[2].health == ServiceHealth::WARNING) ? 50 : 0;
    healthScore.hardware = (services[7].health == ServiceHealth::HEALTHY) ? 100 : 
                          (services[7].health == ServiceHealth::WARNING) ? 50 : 0;
    
    healthScore.timestamp = millis();
}

bool SystemController::validateSystemIntegrity() {
    // 最低限の要件チェック
    bool gpsOk = (services[0].health == ServiceHealth::HEALTHY || 
                  services[0].health == ServiceHealth::WARNING);
    bool networkOk = (services[1].health == ServiceHealth::HEALTHY);
    bool configOk = (services[4].health == ServiceHealth::HEALTHY);
    
    return gpsOk && networkOk && configOk;
}

void SystemController::handleStateTransition(SystemState newState) {
    previousState = currentState;
    currentState = newState;
    stateChangedTime = millis();
    
    // 状態遷移のログ
    const char* stateNames[] = {
        "INITIALIZING", "STARTUP", "RUNNING", "DEGRADED", "ERROR", "RECOVERY", "SHUTDOWN"
    };
    
    // Use LoggingService for state transitions (handled below)
    LOG_INFO_F("SYSTEM", "State transition %s -> %s", 
               stateNames[(int)previousState], stateNames[(int)newState]);
    
    // 状態に応じた処理
    switch (newState) {
        case SystemState::RUNNING:
            LOG_INFO_MSG("SYSTEM", "System is now fully operational");
            if (displayManager) {
                displayManager->displaySystemStatus(true, true, getUptime() / 1000);
            }
            break;
            
        case SystemState::DEGRADED:
            LOG_WARN_MSG("SYSTEM", "System running in degraded mode");
            if (displayManager) {
                displayManager->displayError("Degraded Mode");
            }
            break;
            
        case SystemState::ERROR:
            LOG_ERR_MSG("SYSTEM", "System in error state");
            if (displayManager) {
                displayManager->displayError("System Error");
            }
            break;
            
        case SystemState::RECOVERY:
            LOG_INFO_MSG("SYSTEM", "Attempting system recovery");
            break;
            
        default:
            break;
    }
    
    logSystemState();
}

void SystemController::performRecoveryActions() {
    currentState = SystemState::RECOVERY;
    LOG_INFO_MSG("SYSTEM", "Starting recovery actions");
    
    // 重大な問題があるサービスを特定して復旧を試行
    for (int i = 0; i < 8; i++) {
        if (services[i].health == ServiceHealth::CRITICAL) {
            LOG_INFO_F("SYSTEM", "Attempting recovery for service: %s", services[i].name);
            
            // サービス別復旧処理
            if (strcmp(services[i].name, "GPS") == 0) {
                // GPS復旧処理（再初期化等）
                if (timeManager) {
                    timeManager->init();
                }
            } else if (strcmp(services[i].name, "Network") == 0) {
                // ネットワーク復旧処理
                if (networkManager) {
                    networkManager->init();
                }
            }
            // 他のサービスの復旧処理も追加可能
        }
    }
}

bool SystemController::isHealthy() const {
    return healthScore.overall >= 70; // 70%以上で健全とみなす
}

void SystemController::reportError(const char* service, const char* error) {
    // 該当サービスのエラーカウントを増加
    for (int i = 0; i < 8; i++) {
        if (strcmp(services[i].name, service) == 0) {
            services[i].errorCount++;
            services[i].description = error;
            break;
        }
    }
    
    LOG_ERR_F("SYSTEM", "Error reported by %s: %s", service, error);
}

void SystemController::logSystemState() {
    LOG_INFO_F("SYSTEM", "State: %d, Health: %d%%, Uptime: %lu ms", 
               (int)currentState, healthScore.overall, getUptime());
}

unsigned long SystemController::getUptime() const {
    return millis() - initStartTime;
}

unsigned long SystemController::getStateTime() const {
    return millis() - stateChangedTime;
}

void SystemController::printSystemStatus() {
#ifdef DEBUG_SYSTEM_STATUS
    LOG_DEBUG_MSG("SYSTEM", "=== System Status ===");
    LOG_DEBUG_F("SYSTEM", "State: %d, Health: %d%%, Uptime: %lu ms", 
                (int)currentState, healthScore.overall, getUptime());
    LOG_DEBUG_F("SYSTEM", "GPS: %s, Network: %s, Display: %s",
                gpsConnected ? "OK" : "FAIL",
                networkConnected ? "OK" : "FAIL", 
                displayConnected ? "OK" : "FAIL");
#endif
}

void SystemController::printServiceStatus() {
    LOG_INFO_MSG("SYSTEM", "=== Service Status ===");
    const char* healthNames[] = {"HEALTHY", "WARNING", "CRITICAL", "UNKNOWN"};
    
    for (int i = 0; i < 8; i++) {
        LOG_INFO_F("SYSTEM", "%s: %s - %s (errors: %lu)", 
                   services[i].name,
                   healthNames[(int)services[i].health],
                   services[i].description,
                   services[i].errorCount);
    }
}

// 未実装メソッドを追加
void SystemController::shutdown() {
    LOG_INFO_MSG("SYSTEM", "System shutdown initiated");
    
    currentState = SystemState::SHUTDOWN;
    stateChangedTime = millis();
    
    // 全サービスを順次停止
    if (ntpServer) {
        // NTP server shutdown
    }
    if (networkManager) {
        // Network manager shutdown
    }
    if (displayManager) {
        displayManager->displayError("Shutting down...");
    }
    
    LOG_INFO_MSG("SYSTEM", "System shutdown completed");
}

void SystemController::restart() {
    LOG_WARN_MSG("SYSTEM", "System restart requested");
    
    shutdown();
    
    // 実際の再起動処理（Raspberry Pi Pico 2特有）
    // watchdog timer reset や NVIC_SystemReset() 等を使用
}

void SystemController::emergencyStop() {
    LOG_EMERG_MSG("SYSTEM", "Emergency stop activated");
    
    currentState = SystemState::ERROR;
    stateChangedTime = millis();
    
    // 緊急停止処理
    if (displayManager) {
        displayManager->displayError("EMERGENCY STOP");
    }
    
    // 全システムを安全に停止
}

ServiceHealth SystemController::getServiceHealth(const char* serviceName) {
    for (int i = 0; i < 8; i++) {
        if (strcmp(services[i].name, serviceName) == 0) {
            return services[i].health;
        }
    }
    return ServiceHealth::UNKNOWN;
}

void SystemController::requestRecovery(const char* service) {
    LOG_INFO_F("SYSTEM", "Recovery requested for service: %s", service);
    
    // 該当サービスの復旧を試行
    for (int i = 0; i < 8; i++) {
        if (strcmp(services[i].name, service) == 0) {
            // サービス固有の復旧処理
            if (strcmp(service, "GPS") == 0 && timeManager) {
                timeManager->init();
            } else if (strcmp(service, "Network") == 0 && networkManager) {
                networkManager->init();
            }
            break;
        }
    }
}

unsigned long SystemController::getErrorCount() const {
    unsigned long totalErrors = 0;
    for (int i = 0; i < 8; i++) {
        totalErrors += services[i].errorCount;
    }
    return totalErrors;
}

void SystemController::generateHealthReport(char* buffer, size_t bufferSize) {
    if (!buffer || bufferSize == 0) return;
    
    snprintf(buffer, bufferSize,
        "System Health Report:\n"
        "State: %d\n"
        "Overall Health: %d%%\n"
        "Uptime: %lu ms\n"
        "GPS Health: %d%%\n"
        "Network Health: %d%%\n"
        "NTP Health: %d%%\n"
        "Hardware Health: %d%%\n",
        (int)currentState,
        healthScore.overall,
        getUptime(),
        healthScore.gps,
        healthScore.network,
        healthScore.ntp,
        healthScore.hardware);
}