#include "SystemState.h"

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
}

void SystemState::triggerPps() {
    SystemState& instance = getInstance();
    instance.timeManager.onPpsInterrupt();
    instance.lastPps = micros();
}