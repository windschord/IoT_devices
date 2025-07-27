#include "SystemMonitor.h"
#include "HardwareConfig.h"
#include "TimeManager.h"
#include "LoggingService.h"

SystemMonitor::SystemMonitor(GpsClient* gpsClientInstance, bool* gpsConnectedPtr, volatile bool* ppsReceivedPtr)
    : gpsClient(gpsClientInstance), gpsConnected(gpsConnectedPtr), ppsReceived(ppsReceivedPtr), loggingService(nullptr) {
    
    // Initialize GPS monitoring structure
    gpsMonitor = {0, 0, 30000, 60000, false, false, 0, 0, false};
}

void SystemMonitor::setLoggingService(LoggingService* loggingServiceInstance) {
    loggingService = loggingServiceInstance;
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
#ifdef DEBUG_GPS_MONITOR
        LOG_DEBUG_F("GPS", "Fallback evaluation - GPS Connected: %s, GPS Time Valid: %s, PPS Active: %s, Current Fallback: %s",
                    *gpsConnected ? "YES" : "NO", gpsMonitor.gpsTimeValid ? "YES" : "NO",
                    gpsMonitor.ppsActive ? "YES" : "NO", gpsMonitor.inFallbackMode ? "YES" : "NO");
#endif
    }
    
    if (!*gpsConnected) {
        shouldFallback = true;
#ifdef DEBUG_GPS_MONITOR
        if (now - lastFallbackDebug > 5000) {
            LOG_DEBUG_MSG("GPS", "-> FALLBACK (GPS not connected)");
        }
#endif
    } else if (!gpsMonitor.gpsTimeValid && (now - gpsMonitor.lastValidTime > gpsMonitor.gpsTimeoutMs)) {
        shouldFallback = true;
#ifdef DEBUG_GPS_MONITOR
        if (now - lastFallbackDebug > 5000) {
            LOG_DEBUG_MSG("GPS", "-> FALLBACK (GPS time timeout)");
        }
#endif
    } else if (!gpsMonitor.ppsActive && (now - gpsMonitor.lastPpsTime > gpsMonitor.ppsTimeoutMs)) {
        shouldFallback = true;
#ifdef DEBUG_GPS_MONITOR
        if (now - lastFallbackDebug > 5000) {
            LOG_DEBUG_MSG("GPS", "-> FALLBACK (PPS timeout)");
        }
#endif
    } else {
#ifdef DEBUG_GPS_MONITOR
        if (now - lastFallbackDebug > 5000) {
            LOG_DEBUG_MSG("GPS", "-> GPS OK");
        }
#endif
    }
    
    if (now - lastFallbackDebug > 5000) {
        lastFallbackDebug = now;
    }
    
    // Update fallback mode status
    if (shouldFallback && !gpsMonitor.inFallbackMode) {
        // Enter fallback mode
        gpsMonitor.inFallbackMode = true;
        digitalWrite(LED_ERROR_PIN, HIGH); // Turn on error LED (常時点灯)
        
#if defined(DEBUG_CONSOLE_GPS)
        LOG_WARN_MSG("GPS", "GPS signal lost - entering fallback mode (using RTC)");
#endif
    } else if (!shouldFallback && gpsMonitor.inFallbackMode) {
        // Exit fallback mode
        gpsMonitor.inFallbackMode = false;
        digitalWrite(LED_ERROR_PIN, LOW); // Turn off error LED
        
#ifdef DEBUG_GPS_FALLBACK
        LOG_INFO_MSG("GPS", "GPS signal recovered - exiting fallback mode");
        LOG_INFO_F("GPS", "   GPS Connected: %s, GPS Time Valid: %s, PPS Active: %s",
                   *gpsConnected ? "YES" : "NO", gpsMonitor.gpsTimeValid ? "YES" : "NO",
                   gpsMonitor.ppsActive ? "YES" : "NO");
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
        LOG_INFO_F("GPS", "GPS Monitor - Sats: %d, Quality: %d, PPS: %s, Mode: %s",
                   gpsMonitor.satelliteCount, gpsMonitor.signalQuality,
                   gpsMonitor.ppsActive ? "OK" : "FAIL",
                   gpsMonitor.inFallbackMode ? "FALLBACK" : "GPS");
    }
#endif
}