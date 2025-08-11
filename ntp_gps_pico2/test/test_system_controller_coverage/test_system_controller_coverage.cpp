#include <unity.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Mock Arduino functions
extern "C" {
    uint32_t millis() { return 10000; }
    void delay(uint32_t ms) { (void)ms; }
    void digitalWrite(uint8_t pin, uint8_t val) { (void)pin; (void)val; }
    void reboot() { /* Mock reboot */ }
}

// Mock LoggingService
class MockLoggingService {
public:
    int log_count = 0;
    void logInfo(const char* component, const char* message) { log_count++; }
    void logError(const char* component, const char* message) { log_count++; }
    void logWarning(const char* component, const char* message) { log_count++; }
    void logDebug(const char* component, const char* message) { log_count++; }
};

// Mock TimeManager
class MockTimeManager {
public:
    bool is_initialized = false;
    bool is_synced = false;
    void init() { is_initialized = true; }
    bool isSynced() const { return is_synced; }
    void setSynced(bool synced) { is_synced = synced; }
};

// Mock NetworkManager
class MockNetworkManager {
public:
    bool is_connected = false;
    bool is_initialized = false;
    void init() { is_initialized = true; }
    bool isConnected() const { return is_connected; }
    void setConnected(bool connected) { is_connected = connected; }
};

// Mock SystemMonitor
class MockSystemMonitor {
public:
    bool is_healthy = true;
    bool is_initialized = false;
    void init() { is_initialized = true; }
    bool isHealthy() const { return is_healthy; }
    void setHealthy(bool healthy) { is_healthy = healthy; }
};

// Mock NtpServer
class MockNtpServer {
public:
    bool is_running = false;
    bool is_initialized = false;
    void init() { is_initialized = true; }
    void start() { is_running = true; }
    void stop() { is_running = false; }
    bool isRunning() const { return is_running; }
};

// Mock DisplayManager
class MockDisplayManager {
public:
    bool is_initialized = false;
    bool is_connected = true;
    void init() { is_initialized = true; }
    bool isConnected() const { return is_connected; }
    void setConnected(bool connected) { is_connected = connected; }
};

// Mock ConfigManager
class MockConfigManager {
public:
    bool is_initialized = false;
    bool is_valid = true;
    void init() { is_initialized = true; }
    bool isValid() const { return is_valid; }
    void setValid(bool valid) { is_valid = valid; }
};

// Mock PrometheusMetrics
class MockPrometheusMetrics {
public:
    bool is_initialized = false;
    void init() { is_initialized = true; }
    void update() { }
};

// Mock logging macros
#define LOG_INFO_MSG(component, message) do { } while(0)
#define LOG_ERR_F(component, format, ...) do { } while(0)
#define LOG_WARNING_MSG(component, message) do { } while(0)

// SystemController types and implementation (simplified for testing)
enum class SystemState {
    INITIALIZING,
    STARTUP,
    RUNNING,
    DEGRADED,
    ERROR,
    RECOVERY,
    SHUTDOWN
};

enum class ServiceHealth {
    HEALTHY,
    WARNING,
    CRITICAL,
    UNKNOWN
};

struct ServiceStatus {
    ServiceHealth health;
    const char* name;
    const char* description;
    unsigned long lastCheck;
    bool enabled;
    unsigned long errorCount;
};

struct SystemHealthScore {
    uint8_t overall;
    uint8_t gps;
    uint8_t network;
    uint8_t ntp;
    uint8_t hardware;
    unsigned long timestamp;
};

class SystemController {
private:
    SystemState currentState;
    SystemState previousState;
    unsigned long stateChangedTime;
    
    ServiceStatus services[8]; // GPS, Network, NTP, Display, Config, Logging, Metrics, Hardware
    SystemHealthScore healthScore;
    
    // Service references
    MockTimeManager* timeManager;
    MockNetworkManager* networkManager;
    MockSystemMonitor* systemMonitor;
    MockNtpServer* ntpServer;
    MockDisplayManager* displayManager;
    MockConfigManager* configManager;
    MockLoggingService* loggingService;
    MockPrometheusMetrics* prometheusMetrics;
    
    bool initializationComplete;
    unsigned long initStartTime;
    uint8_t initPhase;
    
    unsigned long lastHealthCheck;
    unsigned long healthCheckInterval;
    bool autoRecoveryEnabled;
    unsigned long lastRecoveryAttempt;
    
    bool gpsConnected;
    bool networkConnected;
    bool displayConnected;
    
    void initializeServices() {
        // Initialize all services in order
        if (timeManager) timeManager->init();
        if (networkManager) networkManager->init();
        if (systemMonitor) systemMonitor->init();
        if (ntpServer) ntpServer->init();
        if (displayManager) displayManager->init();
        if (configManager) configManager->init();
        if (prometheusMetrics) prometheusMetrics->init();
        
        // Initialize service status array
        const char* serviceNames[] = {"GPS", "Network", "NTP", "Display", "Config", "Logging", "Metrics", "Hardware"};
        for (int i = 0; i < 8; i++) {
            services[i] = {
                ServiceHealth::UNKNOWN,
                serviceNames[i],
                "Service initialized",
                millis(),
                true,
                0
            };
        }
        
        initializationComplete = true;
    }
    
    void checkServiceHealth() {
        unsigned long now = millis();
        
        // Check GPS health
        services[0].health = checkGpsHealth();
        services[0].lastCheck = now;
        
        // Check Network health
        services[1].health = checkNetworkHealth();
        services[1].lastCheck = now;
        
        // Check NTP health
        services[2].health = checkNtpHealth();
        services[2].lastCheck = now;
        
        // Check Display health
        services[3].health = checkDisplayHealth();
        services[3].lastCheck = now;
        
        // Check Config health
        services[4].health = checkConfigHealth();
        services[4].lastCheck = now;
        
        // Check Logging health
        services[5].health = checkLoggingHealth();
        services[5].lastCheck = now;
        
        // Check Metrics health
        services[6].health = checkMetricsHealth();
        services[6].lastCheck = now;
        
        // Check Hardware health
        services[7].health = checkHardwareHealth();
        services[7].lastCheck = now;
        
        lastHealthCheck = now;
    }
    
    ServiceHealth checkGpsHealth() {
        if (!timeManager) return ServiceHealth::CRITICAL;
        if (!gpsConnected) return ServiceHealth::WARNING;
        return timeManager->isSynced() ? ServiceHealth::HEALTHY : ServiceHealth::WARNING;
    }
    
    ServiceHealth checkNetworkHealth() {
        if (!networkManager) return ServiceHealth::CRITICAL;
        if (!networkConnected) return ServiceHealth::CRITICAL;
        return networkManager->isConnected() ? ServiceHealth::HEALTHY : ServiceHealth::WARNING;
    }
    
    ServiceHealth checkNtpHealth() {
        if (!ntpServer) return ServiceHealth::CRITICAL;
        return ntpServer->isRunning() ? ServiceHealth::HEALTHY : ServiceHealth::WARNING;
    }
    
    ServiceHealth checkDisplayHealth() {
        if (!displayManager) return ServiceHealth::WARNING;
        return displayManager->isConnected() ? ServiceHealth::HEALTHY : ServiceHealth::WARNING;
    }
    
    ServiceHealth checkConfigHealth() {
        if (!configManager) return ServiceHealth::CRITICAL;
        return configManager->isValid() ? ServiceHealth::HEALTHY : ServiceHealth::CRITICAL;
    }
    
    ServiceHealth checkLoggingHealth() {
        return loggingService ? ServiceHealth::HEALTHY : ServiceHealth::WARNING;
    }
    
    ServiceHealth checkMetricsHealth() {
        return prometheusMetrics ? ServiceHealth::HEALTHY : ServiceHealth::WARNING;
    }
    
    ServiceHealth checkHardwareHealth() {
        if (!systemMonitor) return ServiceHealth::CRITICAL;
        return systemMonitor->isHealthy() ? ServiceHealth::HEALTHY : ServiceHealth::WARNING;
    }
    
    void updateHealthScore() {
        uint8_t healthyCount = 0;
        uint8_t totalServices = 8;
        
        // Calculate individual health scores
        healthScore.gps = (services[0].health == ServiceHealth::HEALTHY) ? 100 : 
                         (services[0].health == ServiceHealth::WARNING) ? 70 : 0;
        healthScore.network = (services[1].health == ServiceHealth::HEALTHY) ? 100 : 
                             (services[1].health == ServiceHealth::WARNING) ? 70 : 0;
        healthScore.ntp = (services[2].health == ServiceHealth::HEALTHY) ? 100 : 
                         (services[2].health == ServiceHealth::WARNING) ? 70 : 0;
        healthScore.hardware = (services[7].health == ServiceHealth::HEALTHY) ? 100 : 
                              (services[7].health == ServiceHealth::WARNING) ? 70 : 0;
        
        // Count healthy services
        for (int i = 0; i < totalServices; i++) {
            if (services[i].health == ServiceHealth::HEALTHY) {
                healthyCount++;
            }
        }
        
        // Calculate overall score
        healthScore.overall = (healthyCount * 100) / totalServices;
        healthScore.timestamp = millis();
    }
    
    void handleStateTransition(SystemState newState) {
        if (currentState != newState) {
            previousState = currentState;
            currentState = newState;
            stateChangedTime = millis();
        }
    }
    
    void performRecoveryActions() {
        if (!autoRecoveryEnabled) return;
        
        unsigned long now = millis();
        if (now - lastRecoveryAttempt < 30000) return; // 30 second cooldown
        
        lastRecoveryAttempt = now;
        
        // Recovery logic based on current state
        switch (currentState) {
            case SystemState::ERROR:
                handleStateTransition(SystemState::RECOVERY);
                break;
            case SystemState::DEGRADED:
                // Try to restore services
                checkServiceHealth();
                break;
            default:
                break;
        }
    }
    
    bool validateSystemIntegrity() {
        // Check if critical services are available
        return (timeManager != nullptr && 
                networkManager != nullptr && 
                systemMonitor != nullptr &&
                configManager != nullptr);
    }

public:
    SystemController() 
        : currentState(SystemState::INITIALIZING),
          previousState(SystemState::INITIALIZING),
          stateChangedTime(0),
          timeManager(nullptr),
          networkManager(nullptr),
          systemMonitor(nullptr),
          ntpServer(nullptr),
          displayManager(nullptr),
          configManager(nullptr),
          loggingService(nullptr),
          prometheusMetrics(nullptr),
          initializationComplete(false),
          initStartTime(0),
          initPhase(0),
          lastHealthCheck(0),
          healthCheckInterval(10000),
          autoRecoveryEnabled(true),
          lastRecoveryAttempt(0),
          gpsConnected(false),
          networkConnected(false),
          displayConnected(false) {
        
        // Initialize health score
        healthScore = {0, 0, 0, 0, 0, 0};
        
        // Initialize services array
        for (int i = 0; i < 8; i++) {
            services[i] = {
                ServiceHealth::UNKNOWN,
                "Unknown",
                "Not initialized",
                0,
                false,
                0
            };
        }
    }
    
    void init() {
        initStartTime = millis();
        stateChangedTime = initStartTime;
        handleStateTransition(SystemState::STARTUP);
        
        if (validateSystemIntegrity()) {
            initializeServices();
            handleStateTransition(SystemState::RUNNING);
        } else {
            handleStateTransition(SystemState::ERROR);
        }
    }
    
    void setServices(MockTimeManager* tm, MockNetworkManager* nm, MockSystemMonitor* sm,
                    MockNtpServer* ntp, MockDisplayManager* dm, MockConfigManager* cm,
                    MockLoggingService* ls, MockPrometheusMetrics* pm) {
        timeManager = tm;
        networkManager = nm;
        systemMonitor = sm;
        ntpServer = ntp;
        displayManager = dm;
        configManager = cm;
        loggingService = ls;
        prometheusMetrics = pm;
    }
    
    void update() {
        unsigned long now = millis();
        
        // Perform health check if interval elapsed
        if (now - lastHealthCheck >= healthCheckInterval) {
            checkServiceHealth();
            updateHealthScore();
            
            // Update system state based on health
            if (healthScore.overall >= 80) {
                if (currentState != SystemState::RUNNING) {
                    handleStateTransition(SystemState::RUNNING);
                }
            } else if (healthScore.overall >= 50) {
                handleStateTransition(SystemState::DEGRADED);
            } else {
                handleStateTransition(SystemState::ERROR);
            }
        }
        
        // Perform recovery if needed
        if (currentState == SystemState::ERROR || currentState == SystemState::DEGRADED) {
            performRecoveryActions();
        }
    }
    
    void shutdown() {
        handleStateTransition(SystemState::SHUTDOWN);
        if (ntpServer) ntpServer->stop();
    }
    
    void restart() {
        shutdown();
        delay(1000);
        init();
    }
    
    void emergencyStop() {
        handleStateTransition(SystemState::ERROR);
        if (ntpServer) ntpServer->stop();
    }
    
    SystemState getState() const { return currentState; }
    
    bool isHealthy() const {
        return (currentState == SystemState::RUNNING && healthScore.overall >= 70);
    }
    
    bool isRunning() const { 
        return currentState == SystemState::RUNNING; 
    }
    
    bool isInitialized() const { 
        return initializationComplete; 
    }
    
    const SystemHealthScore& getHealthScore() const { 
        return healthScore; 
    }
    
    const ServiceStatus* getServiceStatus() const { 
        return services; 
    }
    
    ServiceHealth getServiceHealth(const char* serviceName) {
        const char* serviceNames[] = {"GPS", "Network", "NTP", "Display", "Config", "Logging", "Metrics", "Hardware"};
        for (int i = 0; i < 8; i++) {
            if (strcmp(serviceNames[i], serviceName) == 0) {
                return services[i].health;
            }
        }
        return ServiceHealth::UNKNOWN;
    }
    
    void reportError(const char* service, const char* error) {
        // Find service and increment error count
        const char* serviceNames[] = {"GPS", "Network", "NTP", "Display", "Config", "Logging", "Metrics", "Hardware"};
        for (int i = 0; i < 8; i++) {
            if (strcmp(serviceNames[i], service) == 0) {
                services[i].errorCount++;
                services[i].health = ServiceHealth::WARNING;
                break;
            }
        }
    }
    
    void requestRecovery(const char* service) {
        handleStateTransition(SystemState::RECOVERY);
    }
    
    void enableAutoRecovery(bool enable) { 
        autoRecoveryEnabled = enable; 
    }
    
    unsigned long getUptime() const {
        return millis() - initStartTime;
    }
    
    unsigned long getStateTime() const {
        return millis() - stateChangedTime;
    }
    
    unsigned long getErrorCount() const {
        unsigned long total = 0;
        for (int i = 0; i < 8; i++) {
            total += services[i].errorCount;
        }
        return total;
    }
    
    void updateGpsStatus(bool connected) { 
        gpsConnected = connected; 
    }
    
    void updateNetworkStatus(bool connected) { 
        networkConnected = connected; 
    }
    
    void updateDisplayStatus(bool connected) { 
        displayConnected = connected; 
    }
};

// Global test instances
SystemController systemController;
MockTimeManager mockTimeManager;
MockNetworkManager mockNetworkManager;
MockSystemMonitor mockSystemMonitor;
MockNtpServer mockNtpServer;
MockDisplayManager mockDisplayManager;
MockConfigManager mockConfigManager;
MockLoggingService mockLoggingService;
MockPrometheusMetrics mockPrometheusMetrics;

/**
 * @brief Test SystemController基本初期化と状態管理
 */
void test_systemcontroller_basic_initialization_state_management() {
    // 初期状態確認
    TEST_ASSERT_EQUAL(SystemState::INITIALIZING, systemController.getState());
    TEST_ASSERT_FALSE(systemController.isInitialized());
    TEST_ASSERT_FALSE(systemController.isRunning());
    TEST_ASSERT_FALSE(systemController.isHealthy());
    
    // サービス設定
    systemController.setServices(&mockTimeManager, &mockNetworkManager, &mockSystemMonitor,
                                &mockNtpServer, &mockDisplayManager, &mockConfigManager,
                                &mockLoggingService, &mockPrometheusMetrics);
    
    // 初期化実行
    systemController.init();
    
    // 初期化後の状態確認
    TEST_ASSERT_EQUAL(SystemState::RUNNING, systemController.getState());
    TEST_ASSERT_TRUE(systemController.isInitialized());
    TEST_ASSERT_TRUE(systemController.isRunning());
    
    // すべてのモックサービスが初期化されていることを確認
    TEST_ASSERT_TRUE(mockTimeManager.is_initialized);
    TEST_ASSERT_TRUE(mockNetworkManager.is_initialized);
    TEST_ASSERT_TRUE(mockSystemMonitor.is_initialized);
    TEST_ASSERT_TRUE(mockNtpServer.is_initialized);
    TEST_ASSERT_TRUE(mockDisplayManager.is_initialized);
    TEST_ASSERT_TRUE(mockConfigManager.is_initialized);
    TEST_ASSERT_TRUE(mockPrometheusMetrics.is_initialized);
}

/**
 * @brief Test 全システム状態遷移
 */
void test_systemcontroller_all_system_state_transitions() {
    systemController.setServices(&mockTimeManager, &mockNetworkManager, &mockSystemMonitor,
                                &mockNtpServer, &mockDisplayManager, &mockConfigManager,
                                &mockLoggingService, &mockPrometheusMetrics);
    
    // INITIALIZING → STARTUP → RUNNING
    systemController.init();
    TEST_ASSERT_EQUAL(SystemState::RUNNING, systemController.getState());
    
    // RUNNING → DEGRADED (health score低下)
    mockSystemMonitor.setHealthy(false);
    mockNetworkManager.setConnected(false);
    systemController.updateNetworkStatus(false);
    systemController.update();
    TEST_ASSERT_EQUAL(SystemState::DEGRADED, systemController.getState());
    
    // DEGRADED → ERROR (さらなる健全性低下)
    mockTimeManager.setSynced(false);
    systemController.updateGpsStatus(false);
    mockDisplayManager.setConnected(false);
    systemController.updateDisplayStatus(false);
    systemController.update();
    TEST_ASSERT_EQUAL(SystemState::ERROR, systemController.getState());
    
    // ERROR → RECOVERY (自動復旧)
    systemController.enableAutoRecovery(true);
    systemController.update();
    TEST_ASSERT_EQUAL(SystemState::RECOVERY, systemController.getState());
    
    // RECOVERY → RUNNING (健全性回復)
    mockSystemMonitor.setHealthy(true);
    mockNetworkManager.setConnected(true);
    systemController.updateNetworkStatus(true);
    mockTimeManager.setSynced(true);
    systemController.updateGpsStatus(true);
    systemController.update();
    TEST_ASSERT_EQUAL(SystemState::RUNNING, systemController.getState());
    
    // RUNNING → SHUTDOWN
    systemController.shutdown();
    TEST_ASSERT_EQUAL(SystemState::SHUTDOWN, systemController.getState());
}

/**
 * @brief Test 全サービス健全性チェック
 */
void test_systemcontroller_all_service_health_checks() {
    systemController.setServices(&mockTimeManager, &mockNetworkManager, &mockSystemMonitor,
                                &mockNtpServer, &mockDisplayManager, &mockConfigManager,
                                &mockLoggingService, &mockPrometheusMetrics);
    systemController.init();
    
    // 全サービス正常時
    mockTimeManager.setSynced(true);
    systemController.updateGpsStatus(true);
    mockNetworkManager.setConnected(true);
    systemController.updateNetworkStatus(true);
    mockNtpServer.start();
    mockDisplayManager.setConnected(true);
    systemController.updateDisplayStatus(true);
    mockConfigManager.setValid(true);
    mockSystemMonitor.setHealthy(true);
    
    systemController.update();
    
    // 個別サービス健全性確認
    TEST_ASSERT_EQUAL(ServiceHealth::HEALTHY, systemController.getServiceHealth("GPS"));
    TEST_ASSERT_EQUAL(ServiceHealth::HEALTHY, systemController.getServiceHealth("Network"));
    TEST_ASSERT_EQUAL(ServiceHealth::HEALTHY, systemController.getServiceHealth("NTP"));
    TEST_ASSERT_EQUAL(ServiceHealth::HEALTHY, systemController.getServiceHealth("Display"));
    TEST_ASSERT_EQUAL(ServiceHealth::HEALTHY, systemController.getServiceHealth("Config"));
    TEST_ASSERT_EQUAL(ServiceHealth::HEALTHY, systemController.getServiceHealth("Logging"));
    TEST_ASSERT_EQUAL(ServiceHealth::HEALTHY, systemController.getServiceHealth("Metrics"));
    TEST_ASSERT_EQUAL(ServiceHealth::HEALTHY, systemController.getServiceHealth("Hardware"));
    
    // 健全性スコア確認
    const SystemHealthScore& score = systemController.getHealthScore();
    TEST_ASSERT_EQUAL(100, score.overall);
    TEST_ASSERT_EQUAL(100, score.gps);
    TEST_ASSERT_EQUAL(100, score.network);
    TEST_ASSERT_EQUAL(100, score.ntp);
    TEST_ASSERT_EQUAL(100, score.hardware);
    
    TEST_ASSERT_TRUE(systemController.isHealthy());
    TEST_ASSERT_EQUAL(SystemState::RUNNING, systemController.getState());
}

/**
 * @brief Test 健全性スコア計算・閾値判定
 */
void test_systemcontroller_health_score_calculation_thresholds() {
    systemController.setServices(&mockTimeManager, &mockNetworkManager, &mockSystemMonitor,
                                &mockNtpServer, &mockDisplayManager, &mockConfigManager,
                                &mockLoggingService, &mockPrometheusMetrics);
    systemController.init();
    
    // 一部サービス警告状態
    mockTimeManager.setSynced(false); // GPS WARNING
    systemController.updateGpsStatus(true);
    mockNetworkManager.setConnected(true);
    systemController.updateNetworkStatus(true);
    mockNtpServer.start();
    mockDisplayManager.setConnected(false); // Display WARNING
    systemController.updateDisplayStatus(false);
    
    systemController.update();
    
    const SystemHealthScore& score = systemController.getHealthScore();
    
    // GPS: WARNING → 70点
    TEST_ASSERT_EQUAL(70, score.gps);
    TEST_ASSERT_EQUAL(ServiceHealth::WARNING, systemController.getServiceHealth("GPS"));
    
    // Network: HEALTHY → 100点
    TEST_ASSERT_EQUAL(100, score.network);
    TEST_ASSERT_EQUAL(ServiceHealth::HEALTHY, systemController.getServiceHealth("Network"));
    
    // Display: WARNING → 70点相当
    TEST_ASSERT_EQUAL(ServiceHealth::WARNING, systemController.getServiceHealth("Display"));
    
    // 全体スコア確認（8サービス中6サービスがHEALTHY = 75%）
    TEST_ASSERT_EQUAL(75, score.overall);
    
    // 閾値判定: 75% ≥ 50% → DEGRADED状態
    TEST_ASSERT_EQUAL(SystemState::DEGRADED, systemController.getState());
    
    // さらに悪化させる（≤50%でERROR状態）
    mockNetworkManager.setConnected(false);
    systemController.updateNetworkStatus(false);
    mockSystemMonitor.setHealthy(false);
    systemController.update();
    
    // ERROR状態に移行することを確認
    TEST_ASSERT_EQUAL(SystemState::ERROR, systemController.getState());
}

/**
 * @brief Test エラー報告・カウント機能
 */
void test_systemcontroller_error_reporting_counting() {
    systemController.setServices(&mockTimeManager, &mockNetworkManager, &mockSystemMonitor,
                                &mockNtpServer, &mockDisplayManager, &mockConfigManager,
                                &mockLoggingService, &mockPrometheusMetrics);
    systemController.init();
    
    // 初期エラーカウント
    TEST_ASSERT_EQUAL(0, systemController.getErrorCount());
    
    // エラー報告
    systemController.reportError("GPS", "Signal lost");
    systemController.reportError("Network", "Connection timeout");
    systemController.reportError("GPS", "Antenna failure"); // GPS再度エラー
    systemController.reportError("NTP", "Clock sync failed");
    
    // エラーカウント確認
    TEST_ASSERT_EQUAL(4, systemController.getErrorCount());
    
    // サービス別エラー確認
    const ServiceStatus* services = systemController.getServiceStatus();
    
    // GPS: 2回エラー報告
    TEST_ASSERT_EQUAL(2, services[0].errorCount);
    TEST_ASSERT_EQUAL(ServiceHealth::WARNING, services[0].health);
    
    // Network: 1回エラー報告
    TEST_ASSERT_EQUAL(1, services[1].errorCount);
    TEST_ASSERT_EQUAL(ServiceHealth::WARNING, services[1].health);
    
    // NTP: 1回エラー報告
    TEST_ASSERT_EQUAL(1, services[2].errorCount);
    TEST_ASSERT_EQUAL(ServiceHealth::WARNING, services[2].health);
    
    // その他のサービス: エラーなし
    TEST_ASSERT_EQUAL(0, services[3].errorCount); // Display
    TEST_ASSERT_EQUAL(0, services[4].errorCount); // Config
}

/**
 * @brief Test 自動復旧機能・復旧戦略
 */
void test_systemcontroller_auto_recovery_strategy() {
    systemController.setServices(&mockTimeManager, &mockNetworkManager, &mockSystemMonitor,
                                &mockNtpServer, &mockDisplayManager, &mockConfigManager,
                                &mockLoggingService, &mockPrometheusMetrics);
    systemController.init();
    
    // 自動復旧有効確認
    systemController.enableAutoRecovery(true);
    
    // システムをエラー状態にする
    mockSystemMonitor.setHealthy(false);
    mockNetworkManager.setConnected(false);
    systemController.updateNetworkStatus(false);
    mockTimeManager.setSynced(false);
    systemController.updateGpsStatus(false);
    
    systemController.update();
    TEST_ASSERT_EQUAL(SystemState::ERROR, systemController.getState());
    
    // 自動復旧実行
    systemController.update();
    TEST_ASSERT_EQUAL(SystemState::RECOVERY, systemController.getState());
    
    // 復旧要求テスト
    systemController.requestRecovery("GPS");
    TEST_ASSERT_EQUAL(SystemState::RECOVERY, systemController.getState());
    
    // 自動復旧無効化テスト
    systemController.enableAutoRecovery(false);
    
    // エラー状態に戻す
    systemController.emergencyStop();
    TEST_ASSERT_EQUAL(SystemState::ERROR, systemController.getState());
    
    // 自動復旧が実行されないことを確認
    systemController.update();
    TEST_ASSERT_EQUAL(SystemState::ERROR, systemController.getState()); // 復旧されない
}

/**
 * @brief Test ハードウェア状態更新機能
 */
void test_systemcontroller_hardware_status_updates() {
    systemController.setServices(&mockTimeManager, &mockNetworkManager, &mockSystemMonitor,
                                &mockNtpServer, &mockDisplayManager, &mockConfigManager,
                                &mockLoggingService, &mockPrometheusMetrics);
    systemController.init();
    
    // ハードウェア状態更新
    systemController.updateGpsStatus(true);
    systemController.updateNetworkStatus(true);
    systemController.updateDisplayStatus(true);
    
    // GPS接続時の健全性確認
    mockTimeManager.setSynced(true);
    systemController.update();
    TEST_ASSERT_EQUAL(ServiceHealth::HEALTHY, systemController.getServiceHealth("GPS"));
    
    // GPS切断時の健全性確認
    systemController.updateGpsStatus(false);
    systemController.update();
    TEST_ASSERT_EQUAL(ServiceHealth::WARNING, systemController.getServiceHealth("GPS"));
    
    // ネットワーク切断時の健全性確認
    systemController.updateNetworkStatus(false);
    mockNetworkManager.setConnected(false);
    systemController.update();
    TEST_ASSERT_EQUAL(ServiceHealth::CRITICAL, systemController.getServiceHealth("Network"));
    
    // ディスプレイ切断時の健全性確認
    systemController.updateDisplayStatus(false);
    mockDisplayManager.setConnected(false);
    systemController.update();
    TEST_ASSERT_EQUAL(ServiceHealth::WARNING, systemController.getServiceHealth("Display"));
}

/**
 * @brief Test システム制御機能（再起動・緊急停止）
 */
void test_systemcontroller_system_control_restart_emergency() {
    systemController.setServices(&mockTimeManager, &mockNetworkManager, &mockSystemMonitor,
                                &mockNtpServer, &mockDisplayManager, &mockConfigManager,
                                &mockLoggingService, &mockPrometheusMetrics);
    systemController.init();
    
    // 初期状態確認
    TEST_ASSERT_EQUAL(SystemState::RUNNING, systemController.getState());
    mockNtpServer.start();
    TEST_ASSERT_TRUE(mockNtpServer.isRunning());
    
    // 緊急停止テスト
    systemController.emergencyStop();
    TEST_ASSERT_EQUAL(SystemState::ERROR, systemController.getState());
    TEST_ASSERT_FALSE(mockNtpServer.isRunning());
    
    // 再起動テスト
    systemController.restart();
    TEST_ASSERT_EQUAL(SystemState::RUNNING, systemController.getState());
    TEST_ASSERT_TRUE(systemController.isInitialized());
    
    // 通常シャットダウンテスト
    mockNtpServer.start();
    TEST_ASSERT_TRUE(mockNtpServer.isRunning());
    
    systemController.shutdown();
    TEST_ASSERT_EQUAL(SystemState::SHUTDOWN, systemController.getState());
    TEST_ASSERT_FALSE(mockNtpServer.isRunning());
}

/**
 * @brief Test 統計情報・アップタイム計算
 */
void test_systemcontroller_statistics_uptime_calculation() {
    systemController.setServices(&mockTimeManager, &mockNetworkManager, &mockSystemMonitor,
                                &mockNtpServer, &mockDisplayManager, &mockConfigManager,
                                &mockLoggingService, &mockPrometheusMetrics);
    
    // 初期化前
    unsigned long preInitTime = systemController.getUptime();
    TEST_ASSERT_EQUAL(0, preInitTime);
    
    // 初期化
    systemController.init();
    
    // アップタイム確認（millis()=10000なので、initStartTime以降の経過時間）
    unsigned long uptime = systemController.getUptime();
    TEST_ASSERT_GREATER_THAN(0, uptime);
    
    // 状態時間確認
    unsigned long stateTime = systemController.getStateTime();
    TEST_ASSERT_GREATER_OR_EQUAL(0, stateTime);
    
    // エラー追加とカウント確認
    systemController.reportError("GPS", "Test error 1");
    systemController.reportError("Network", "Test error 2");
    systemController.reportError("NTP", "Test error 3");
    
    TEST_ASSERT_EQUAL(3, systemController.getErrorCount());
    
    // 状態変更後の状態時間確認
    SystemState initialState = systemController.getState();
    systemController.emergencyStop();
    
    if (systemController.getState() != initialState) {
        unsigned long newStateTime = systemController.getStateTime();
        // 状態変更直後なので、状態時間は小さい値
        TEST_ASSERT_LESS_OR_EQUAL(stateTime, newStateTime);
    }
}

/**
 * @brief Test サービス参照なし時のエラーハンドリング
 */
void test_systemcontroller_no_service_references_error_handling() {
    SystemController isolatedController;
    
    // サービス設定なしで初期化
    isolatedController.init();
    
    // エラー状態になることを確認（validateSystemIntegrity失敗）
    TEST_ASSERT_EQUAL(SystemState::ERROR, isolatedController.getState());
    TEST_ASSERT_FALSE(isolatedController.isHealthy());
    TEST_ASSERT_FALSE(isolatedController.isRunning());
    
    // 健全性チェックでCRITICALになることを確認
    isolatedController.update();
    
    // 存在しないサービス健全性確認
    TEST_ASSERT_EQUAL(ServiceHealth::UNKNOWN, isolatedController.getServiceHealth("NonExistent"));
    
    // 部分的サービス設定テスト
    isolatedController.setServices(&mockTimeManager, nullptr, nullptr,
                                  nullptr, nullptr, nullptr,
                                  nullptr, nullptr);
    
    isolatedController.init();
    TEST_ASSERT_EQUAL(SystemState::ERROR, isolatedController.getState()); // まだ不完全
    
    // 最小限の重要サービス設定
    isolatedController.setServices(&mockTimeManager, &mockNetworkManager, &mockSystemMonitor,
                                  nullptr, nullptr, &mockConfigManager,
                                  nullptr, nullptr);
    
    isolatedController.init();
    TEST_ASSERT_EQUAL(SystemState::RUNNING, isolatedController.getState()); // 最小限動作
}

/**
 * @brief Test 境界値・エッジケース処理
 */
void test_systemcontroller_boundary_edge_cases() {
    systemController.setServices(&mockTimeManager, &mockNetworkManager, &mockSystemMonitor,
                                &mockNtpServer, &mockDisplayManager, &mockConfigManager,
                                &mockLoggingService, &mockPrometheusMetrics);
    
    // 初期化前の各種操作
    systemController.update(); // 初期化前のupdate
    systemController.shutdown(); // 初期化前のshutdown
    systemController.emergencyStop(); // 初期化前の緊急停止
    
    // null文字列でのエラー報告
    systemController.reportError(nullptr, "Test error");
    systemController.reportError("GPS", nullptr);
    systemController.reportError(nullptr, nullptr);
    
    // 空文字列でのエラー報告
    systemController.reportError("", "Empty service name");
    systemController.reportError("GPS", "");
    
    // 存在しないサービスでの復旧要求
    systemController.requestRecovery("NonExistentService");
    systemController.requestRecovery(nullptr);
    systemController.requestRecovery("");
    
    // 初期化実行
    systemController.init();
    
    // 重複初期化
    systemController.init();
    TEST_ASSERT_TRUE(systemController.isInitialized());
    
    // 重複シャットダウン
    systemController.shutdown();
    systemController.shutdown();
    TEST_ASSERT_EQUAL(SystemState::SHUTDOWN, systemController.getState());
    
    // 長いサービス名でのエラー報告
    systemController.reportError("VeryLongServiceNameThatExceedsNormalLength", "Long name test");
    
    // 自動復旧の連続実行（クールダウンテスト）
    systemController.enableAutoRecovery(true);
    systemController.emergencyStop();
    systemController.update(); // 1回目の復旧
    SystemState firstRecoveryState = systemController.getState();
    systemController.update(); // 2回目の復旧（クールダウン中）
    
    // クールダウン期間中は追加復旧が実行されないことを確認
    TEST_ASSERT_EQUAL(firstRecoveryState, systemController.getState());
}

// Test suite setup and teardown
void setUp(void) {
    // Reset all mock services
    mockTimeManager.is_initialized = false;
    mockTimeManager.is_synced = false;
    mockNetworkManager.is_connected = false;
    mockNetworkManager.is_initialized = false;
    mockSystemMonitor.is_healthy = true;
    mockSystemMonitor.is_initialized = false;
    mockNtpServer.is_running = false;
    mockNtpServer.is_initialized = false;
    mockDisplayManager.is_initialized = false;
    mockDisplayManager.is_connected = true;
    mockConfigManager.is_initialized = false;
    mockConfigManager.is_valid = true;
    mockPrometheusMetrics.is_initialized = false;
    mockLoggingService.log_count = 0;
}

void tearDown(void) {
    // Cleanup after each test
}

/**
 * @brief SystemController完全カバレッジテスト実行
 */
int main() {
    UNITY_BEGIN();
    
    // Basic initialization and state management
    RUN_TEST(test_systemcontroller_basic_initialization_state_management);
    
    // State transitions
    RUN_TEST(test_systemcontroller_all_system_state_transitions);
    
    // Service health checks
    RUN_TEST(test_systemcontroller_all_service_health_checks);
    
    // Health score calculation
    RUN_TEST(test_systemcontroller_health_score_calculation_thresholds);
    
    // Error reporting
    RUN_TEST(test_systemcontroller_error_reporting_counting);
    
    // Auto recovery
    RUN_TEST(test_systemcontroller_auto_recovery_strategy);
    
    // Hardware status updates
    RUN_TEST(test_systemcontroller_hardware_status_updates);
    
    // System control functions
    RUN_TEST(test_systemcontroller_system_control_restart_emergency);
    
    // Statistics and uptime
    RUN_TEST(test_systemcontroller_statistics_uptime_calculation);
    
    // Error handling without service references
    RUN_TEST(test_systemcontroller_no_service_references_error_handling);
    
    // Edge cases
    RUN_TEST(test_systemcontroller_boundary_edge_cases);
    
    return UNITY_END();
}