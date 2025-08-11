#include "SystemState.h"
#include "ServiceContainer.h"

SystemState& SystemState::getInstance() {
    static SystemState instance;
    return instance;
}

SystemState::SystemState() 
    : server(80)
    , timeSync{0, 0, 0, 0, false, 1.0}
    , timeManager(&rtc, &timeSync, nullptr)
    , networkManager(&ntpUdp)
    , systemMonitor(&gpsClient, &gpsConnected, &ppsReceived)
    , ntpServer(&ntpUdp, &timeManager, nullptr)
    , loggingService(&ntpUdp)
    , gpsClient(Serial)
{
    // Initialize member variables
    lastPps = 0;
    ppsReceived = false;
    gpsConnected = false;
    webServerStarted = false;
    lastGnssLedUpdate = 0;
    gnssLedState = false;
    gnssBlinkInterval = 0;
    ledOffTime = 0;
    lastLowPriorityUpdate = 0;
    lastMediumPriorityUpdate = 0;
    
    // Initialize hardware status
    hardwareStatus.gpsReady = false;
    hardwareStatus.networkReady = false;
    hardwareStatus.displayReady = false;
    hardwareStatus.rtcReady = false;
    hardwareStatus.storageReady = false;
    hardwareStatus.lastGpsUpdate = 0;
    hardwareStatus.lastNetworkCheck = 0;
    hardwareStatus.cpuTemperature = 0.0f;
    hardwareStatus.freeMemory = 0;
    
    // Initialize system statistics
    systemStatistics.systemUptime = millis();
    systemStatistics.ntpRequestsTotal = 0;
    systemStatistics.ntpResponsesTotal = 0;
    systemStatistics.ntpDroppedTotal = 0;
    systemStatistics.gpsFixCount = 0;
    systemStatistics.ppsCount = 0;
    systemStatistics.errorCount = 0;
    systemStatistics.restartCount = 0;
    systemStatistics.averageResponseTime = 0.0f;
    systemStatistics.currentAccuracy = 0.0f;
}

void SystemState::triggerPps() {
    SystemState& instance = getInstance();
    instance.timeManager.onPpsInterrupt();
    instance.lastPps = micros();
    instance.incrementPpsCount();
}

ServiceContainer& SystemState::getServiceContainer() {
    return ServiceContainer::getInstance();
}

// グローバル変数へのアクセス用ヘルパー関数
static IHardwareInterface* createButtonHAL() {
    extern ButtonHAL g_button_hal;
    return &g_button_hal;
}

static IHardwareInterface* createStorageHAL() {
    extern StorageHAL g_storage_hal;
    return &g_storage_hal;
}

bool SystemState::initializeDIContainer() {
    ServiceContainer& container = getServiceContainer();
    
    // ハードウェア登録
    container.registerHardware("ButtonHAL", createButtonHAL);
    container.registerHardware("StorageHAL", createStorageHAL);
    
    // システム状態をハードウェアステータスに反映
    hardwareStatus.displayReady = true; // OLEDは既に初期化済み
    hardwareStatus.rtcReady = true;     // RTCは既に初期化済み
    
    Serial.println("✓ DI Container initialized with HAL components");
    return true;
}