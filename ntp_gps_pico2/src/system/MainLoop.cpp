#include "MainLoop.h"
#include "SystemState.h"
#include "../gps/Gps_model.h"
#include "../network/NtpTypes.h"
#include "../hal/HardwareConfig.h"

void MainLoop::execute() {
    unsigned long currentTime = millis();
    SystemState& state = SystemState::getInstance();
    
    // ====== HIGH PRIORITY (every loop) ======
    executeHighPriorityTasks();
    
    // ====== MEDIUM PRIORITY (100ms interval) ======
    if (currentTime - state.getLastMediumPriorityUpdate() >= MEDIUM_PRIORITY_INTERVAL) {
        executeMediumPriorityTasks();
        state.setLastMediumPriorityUpdate(currentTime);
    }
    
    // ====== LOW PRIORITY (1000ms interval) ======
    if (currentTime - state.getLastLowPriorityUpdate() >= LOW_PRIORITY_INTERVAL) {
        executeLowPriorityTasks();
        state.setLastLowPriorityUpdate(currentTime);
    }
}

void MainLoop::executeHighPriorityTasks() {
    SystemState& state = SystemState::getInstance();
    
    // Critical system monitoring
    state.getErrorHandler().update();
    state.getPhysicalReset().update();
    state.getPowerManager().update();
    
    // GPS data processing
    processGpsData();
    
    // LED management
    manageLeds();
    
    // Network and communication
    state.getNetworkManager().manageUdpSockets();
    state.getNtpServer().processRequests();
    
    // Logging and web services
    state.getLoggingService().processLogs();
    state.getWebServer().handleClient(
        Serial, 
        state.getEthernetServer(), 
        state.getGpsClient().getUbxNavSatData_t(), 
        state.getGpsClient().getGpsSummaryData()
    );
}

void MainLoop::executeMediumPriorityTasks() {
    SystemState& state = SystemState::getInstance();
    
    // Display and system control updates
    state.getDisplayManager().update();
    state.getSystemController().update();
    
    // GPS signal monitoring
    state.getSystemMonitor().monitorGpsSignal();
    
    // Display content processing
    processDisplayContent();
}

void MainLoop::executeLowPriorityTasks() {
    SystemState& state = SystemState::getInstance();
    
    // Update hardware status
    state.getSystemController().updateGpsStatus(state.isGpsConnected());
    state.getSystemController().updateNetworkStatus(state.getNetworkManager().isConnected());
    
    // Network monitoring and recovery
    processNetworkRecovery();
    
    // Update metrics
    updateMetrics();
    
    // Invalidate GPS cache for fresh data
    state.getWebServer().invalidateGpsCache();
    
    // Debug output (reduced frequency)
    debugNetworkStatus();
}

void MainLoop::processGpsData() {
    SystemState& state = SystemState::getInstance();
    
    if (state.isGpsConnected()) {
        state.getGNSS().checkUblox();
        state.getGNSS().checkCallbacks();
        
        // PPS signal processing
        GpsSummaryData gpsData = state.getGpsClient().getGpsSummaryData();
        state.getTimeManager().processPpsSync(gpsData);
        
        // Update GNSS LED based on GPS fix quality
        if (gpsData.fixType >= 3) {
            state.setGnssBlinkInterval(0); // ON (constant): 3D fix or better
            digitalWrite(LED_GNSS_FIX_PIN, HIGH);
        } else if (gpsData.fixType >= 2) {
            state.setGnssBlinkInterval(500); // FAST BLINK: 2D fix
        } else {
            state.setGnssBlinkInterval(2000); // SLOW BLINK: GPS connected but no fix
        }
    } else {
        // GPS not connected
        state.setGnssBlinkInterval(0);
        digitalWrite(LED_GNSS_FIX_PIN, LOW);
    }
    
    handleGnssBlinking();
}

void MainLoop::manageLeds() {
    SystemState& state = SystemState::getInstance();
    
    // PPS LED management (non-blocking)
    unsigned long ledOffTime = state.getLedOffTime();
    if (ledOffTime == 0 && digitalRead(LED_PPS_PIN)) {
        state.setLedOffTime(millis() + 50); // Turn off LED after 50ms
    }
    if (ledOffTime > 0 && millis() > ledOffTime) {
        analogWrite(LED_PPS_PIN, 0);
        state.setLedOffTime(0);
    }
}

void MainLoop::processNetworkRecovery() {
    SystemState& state = SystemState::getInstance();
    
    state.getNetworkManager().monitorConnection();
    state.getNetworkManager().attemptReconnection();
    state.getNetworkManager().performHealthCheck();
    
    // Auto-recovery if needed
    if (state.getNetworkManager().isAutoRecoveryNeeded()) {
        if (state.getNetworkManager().performHardwareReset()) {
            state.getNetworkManager().attemptReconnection();
            state.getNetworkManager().resetAutoRecoveryCounters();
        } else {
            state.getNetworkManager().handleConnectionFailure();
        }
    }
    
    // Reset counters on successful connection
    if (state.getNetworkManager().isConnected()) {
        static bool wasDisconnected = true;
        if (wasDisconnected) {
            state.getNetworkManager().resetAutoRecoveryCounters();
            wasDisconnected = false;
        }
    } else {
        static bool wasConnected = false;
        if (!wasConnected) {
            wasConnected = true;
        }
    }
}

void MainLoop::updateMetrics() {
    SystemState& state = SystemState::getInstance();
    
    GpsSummaryData gpsData = state.getGpsClient().getGpsSummaryData();
    const NtpStatistics& ntpStats = state.getNtpServer().getStatistics();
    const GpsMonitor& gpsMonitor = state.getSystemMonitor().getGpsMonitor();
    unsigned long ppsCount = state.getTimeManager().getPpsCount();
    
    state.getPrometheusMetrics().update(&ntpStats, &gpsData, &gpsMonitor, ppsCount);
}

void MainLoop::processDisplayContent() {
    SystemState& state = SystemState::getInstance();
    
    if (state.getDisplayManager().shouldDisplay()) {
        GpsSummaryData gpsData = state.getGpsClient().getGpsSummaryData();
        
        switch (state.getDisplayManager().getCurrentMode()) {
            case DISPLAY_GPS_TIME:
            case DISPLAY_GPS_SATS:
                state.getDisplayManager().displayInfo(gpsData);
                break;
                
            case DISPLAY_NTP_STATS:
                state.getDisplayManager().displayNtpStats(state.getNtpServer().getStatistics());
                break;
                
            case DISPLAY_SYSTEM_STATUS:
                state.getDisplayManager().displaySystemStatus(
                    state.isGpsConnected(), 
                    state.getNetworkManager().isConnected(), 
                    millis() / 1000
                );
                break;
                
            case DISPLAY_ERROR:
                // Error display is handled automatically by DisplayManager
                break;
                
            default:
                state.getDisplayManager().displayInfo(gpsData);
                break;
        }
    }
}

void MainLoop::debugNetworkStatus() {
    static unsigned long lastNetworkDebug = 0;
    if (millis() - lastNetworkDebug > NETWORK_DEBUG_INTERVAL) {
        lastNetworkDebug = millis();
        
        #ifdef DEBUG_NETWORK
        SystemState& state = SystemState::getInstance();
        Serial.print("Network Status - Connected: ");
        Serial.print(state.getNetworkManager().isConnected() ? "YES" : "NO");
        if (state.getNetworkManager().isConnected()) {
            Serial.print(", IP: ");
            Serial.print(Ethernet.localIP());
        }
        Serial.print(", Hardware: ");
        switch(Ethernet.hardwareStatus()) {
            case EthernetNoHardware: Serial.print("NO_HW"); break;
            case EthernetW5100: Serial.print("W5100"); break;
            case EthernetW5200: Serial.print("W5200"); break;
            case EthernetW5500: Serial.print("W5500"); break;
            default: Serial.print("UNKNOWN"); break;
        }
        Serial.print(", Link: ");
        switch(Ethernet.linkStatus()) {
            case Unknown: Serial.print("UNKNOWN"); break;
            case LinkON: Serial.print("ON"); break;
            case LinkOFF: Serial.print("OFF"); break;
        }
        Serial.println();
        #endif
    }

    #if defined(DEBUG_CONSOLE_GPS)
    SystemState& state = SystemState::getInstance();
    
    // Get current RTC time
    DateTime now = state.getRTC().now();
    Serial.print("RTC DateTime: ");

    char dateTimechr[20];
    sprintf(dateTimechr, "%04d/%02d/%02d %02d:%02d:%02d",
            now.year(), now.month(), now.day(),
            now.hour(), now.minute(), now.second());
    Serial.print(dateTimechr);
    
    // Additional RTC debug info
    static unsigned long lastRtcDetailDebug = 0;
    if (millis() - lastRtcDetailDebug > RTC_DETAIL_DEBUG_INTERVAL) {
        Serial.print(" [I2C Address: 0x68, Wire1 Bus]");
        if (state.getRTC().lostPower()) {
            Serial.print(" [POWER_LOST]");
        }
        Serial.printf(" Temp: %.2f°C", state.getRTC().getTemperature());
        lastRtcDetailDebug = millis();
    }

    switch (now.dayOfTheWeek()) {
        case 1: Serial.print(" Sun"); break;
        case 2: Serial.print(" Mon"); break;
        case 3: Serial.print(" Tue"); break;
        case 4: Serial.print(" Wed"); break;
        case 5: Serial.print(" Thu"); break;
        case 6: Serial.print(" Fri"); break;
        case 7: Serial.print(" Sat"); break;
        default: break;
    }

    Serial.print(" - Temp: ");
    Serial.print(state.getRTC().getTemperature());
    Serial.println();
    delay(1000);
    #endif
}

void MainLoop::handleGnssBlinking() {
    SystemState& state = SystemState::getInstance();
    
    unsigned long gnssBlinkInterval = state.getGnssBlinkInterval();
    if (gnssBlinkInterval > 0) {
        unsigned long currentTime = millis();
        if (currentTime - state.getLastGnssLedUpdate() >= gnssBlinkInterval) {
            bool newState = !state.getGnssLedState();
            state.setGnssLedState(newState);
            digitalWrite(LED_GNSS_FIX_PIN, newState ? HIGH : LOW);
            state.setLastGnssLedUpdate(currentTime);
        }
    }
}