#include "SystemMonitor.h"
#include "HardwareConfig.h"
#include "TimeManager.h"

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
    
    // Monitor PPS signal by checking if PPS count is increasing
    // This works better than checking a flag that gets reset immediately
    static unsigned long lastPpsCount = 0;
    extern TimeManager timeManager;
    unsigned long currentPpsCount = timeManager.getPpsCount();
    
    if (currentPpsCount > lastPpsCount) {
        // PPS count increased - signal is active
        gpsMonitor.lastPpsTime = now;
        gpsMonitor.ppsActive = true;
        lastPpsCount = currentPpsCount;
    } else if (now - gpsMonitor.lastPpsTime > gpsMonitor.ppsTimeoutMs) {
        // No PPS count increase for timeout period - signal inactive
        gpsMonitor.ppsActive = false;
    }
}

void SystemMonitor::evaluateFallbackMode() {
    unsigned long now = millis();
    bool shouldFallback = false;
    
    // Debug fallback conditions
    static unsigned long lastFallbackDebug = 0;
    if (now - lastFallbackDebug > 5000) { // Every 5 seconds
        Serial.print("Fallback evaluation - ");
        Serial.print("GPS Connected: "); Serial.print(*gpsConnected ? "YES" : "NO");
        Serial.print(", GPS Time Valid: "); Serial.print(gpsMonitor.gpsTimeValid ? "YES" : "NO");
        Serial.print(", PPS Active: "); Serial.print(gpsMonitor.ppsActive ? "YES" : "NO");
        Serial.print(", Current Fallback: "); Serial.print(gpsMonitor.inFallbackMode ? "YES" : "NO");
    }
    
    if (!*gpsConnected) {
        shouldFallback = true;
        if (now - lastFallbackDebug > 5000) Serial.print(" -> FALLBACK (GPS not connected)");
    } else if (!gpsMonitor.gpsTimeValid && (now - gpsMonitor.lastValidTime > gpsMonitor.gpsTimeoutMs)) {
        shouldFallback = true;
        if (now - lastFallbackDebug > 5000) Serial.print(" -> FALLBACK (GPS time timeout)");
    } else if (!gpsMonitor.ppsActive && (now - gpsMonitor.lastPpsTime > gpsMonitor.ppsTimeoutMs)) {
        shouldFallback = true;
        if (now - lastFallbackDebug > 5000) Serial.print(" -> FALLBACK (PPS timeout)");
    } else {
        if (now - lastFallbackDebug > 5000) Serial.print(" -> GPS OK");
    }
    
    if (now - lastFallbackDebug > 5000) {
        Serial.println();
        lastFallbackDebug = now;
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
        
        Serial.println("✅ GPS signal recovered - exiting fallback mode");
        Serial.print("   GPS Connected: "); Serial.println(*gpsConnected ? "YES" : "NO");
        Serial.print("   GPS Time Valid: "); Serial.println(gpsMonitor.gpsTimeValid ? "YES" : "NO");
        Serial.print("   PPS Active: "); Serial.println(gpsMonitor.ppsActive ? "YES" : "NO");
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