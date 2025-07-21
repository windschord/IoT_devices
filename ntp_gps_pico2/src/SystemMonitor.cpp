#include "SystemMonitor.h"
#include "HardwareConfig.h"

SystemMonitor::SystemMonitor(GpsClient* gpsClientInstance, bool* gpsConnectedPtr, volatile bool* ppsReceivedPtr)
    : gpsClient(gpsClientInstance), gpsConnected(gpsConnectedPtr), ppsReceived(ppsReceivedPtr) {
    
    // Initialize GPS monitoring structure
    gpsMonitor = {0, 0, 30000, 60000, false, false, 0, 0, false};
}

void SystemMonitor::init() {
    unsigned long now = millis();
    gpsMonitor.lastValidTime = now;
    gpsMonitor.lastPpsTime = now;
    gpsMonitor.inFallbackMode = false;
}

void SystemMonitor::updateGpsStatus() {
    if (*gpsConnected) {
        // Get GPS time data
        GpsSummaryData gpsData = gpsClient->getGpsSummaryData();
        
        // Check GPS time validity
        if (gpsData.timeValid && gpsData.dateValid) {
            gpsMonitor.lastValidTime = millis();
            gpsMonitor.gpsTimeValid = true;
            gpsMonitor.satelliteCount = gpsData.SIV;
            
            // Estimate signal quality from satellite count (simplified version)
            if (gpsData.SIV >= 8) gpsMonitor.signalQuality = 10;
            else if (gpsData.SIV >= 6) gpsMonitor.signalQuality = 8;
            else if (gpsData.SIV >= 4) gpsMonitor.signalQuality = 6;
            else if (gpsData.SIV >= 3) gpsMonitor.signalQuality = 4;
            else gpsMonitor.signalQuality = 2;
        } else {
            gpsMonitor.gpsTimeValid = false;
            gpsMonitor.signalQuality = 0;
        }
    }
}

void SystemMonitor::updatePpsStatus() {
    unsigned long now = millis();
    
    // Monitor PPS signal
    if (*ppsReceived) {
        gpsMonitor.lastPpsTime = now;
        gpsMonitor.ppsActive = true;
    } else if (now - gpsMonitor.lastPpsTime > gpsMonitor.ppsTimeoutMs) {
        gpsMonitor.ppsActive = false;
    }
}

void SystemMonitor::evaluateFallbackMode() {
    unsigned long now = millis();
    bool shouldFallback = false;
    
    if (!*gpsConnected) {
        shouldFallback = true;
    } else if (!gpsMonitor.gpsTimeValid && (now - gpsMonitor.lastValidTime > gpsMonitor.gpsTimeoutMs)) {
        shouldFallback = true;
    } else if (!gpsMonitor.ppsActive && (now - gpsMonitor.lastPpsTime > gpsMonitor.ppsTimeoutMs)) {
        shouldFallback = true;
    }
    
    // Update fallback mode status
    if (shouldFallback && !gpsMonitor.inFallbackMode) {
        // Enter fallback mode
        gpsMonitor.inFallbackMode = true;
        analogWrite(LED_ERROR_PIN, 255); // Turn on error LED
        
#if defined(DEBUG_CONSOLE_GPS)
        Serial.println("GPS signal lost - entering fallback mode (using RTC)");
#endif
    } else if (!shouldFallback && gpsMonitor.inFallbackMode) {
        // Exit fallback mode
        gpsMonitor.inFallbackMode = false;
        analogWrite(LED_ERROR_PIN, 0); // Turn off error LED
        
#if defined(DEBUG_CONSOLE_GPS)
        Serial.println("GPS signal recovered - exiting fallback mode");
#endif
    }
}

void SystemMonitor::monitorGpsSignal() {
    updateGpsStatus();
    updatePpsStatus();
    evaluateFallbackMode();
    
    // Debug information output
#if defined(DEBUG_CONSOLE_GPS)
    unsigned long now = millis();
    if (now % 10000 < 100) { // Every 10 seconds
        Serial.print("GPS Monitor - Sats: ");
        Serial.print(gpsMonitor.satelliteCount);
        Serial.print(" Quality: ");
        Serial.print(gpsMonitor.signalQuality);
        Serial.print(" PPS: ");
        Serial.print(gpsMonitor.ppsActive ? "OK" : "FAIL");
        Serial.print(" Mode: ");
        Serial.println(gpsMonitor.inFallbackMode ? "FALLBACK" : "GPS");
    }
#endif
}