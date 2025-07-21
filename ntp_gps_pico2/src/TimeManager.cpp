#include "TimeManager.h"
#include "HardwareConfig.h"

TimeManager::TimeManager(uRTCLib* rtcInstance, TimeSync* timeSyncInstance, const GpsMonitor* gpsMonitorInstance)
    : rtc(rtcInstance), timeSync(timeSyncInstance), gpsMonitor(gpsMonitorInstance),
      ppsReceived(false), ppsTimestamp(0), ppsCount(0) {
}

void TimeManager::init() {
    // Initialize PPS pin
    pinMode(GPS_PPS_PIN, INPUT_PULLUP);
    
    // Reset sync state
    timeSync->synchronized = false;
    timeSync->accuracy = 1.0;
}

void TimeManager::onPpsInterrupt() {
    unsigned long now = micros();
    ppsTimestamp = now;
    ppsReceived = true;
    ppsCount++;
    
    // LED flash (non-blocking)
    analogWrite(LED_PPS_PIN, 255);
    
#if defined(DEBUG_CONSOLE_PPS)
    // Minimal processing in interrupt
#endif
}

unsigned long TimeManager::getHighPrecisionTime() {
    extern bool gpsConnected;
    
    if (timeSync->synchronized && gpsConnected && !gpsMonitor->inFallbackMode) {
        // Return high-precision PPS-synchronized time
        unsigned long elapsed = micros() - timeSync->ppsTime;
        return timeSync->gpsTime * 1000 + elapsed / 1000; // milliseconds
    } else {
        // RTC fallback time
        rtc->refresh();
        struct tm timeinfo = {0};
        timeinfo.tm_year = rtc->year() + 100; // RTC is 2-digit year
        timeinfo.tm_mon = rtc->month() - 1;
        timeinfo.tm_mday = rtc->day();
        timeinfo.tm_hour = rtc->hour();
        timeinfo.tm_min = rtc->minute();
        timeinfo.tm_sec = rtc->second();
        return mktime(&timeinfo) * 1000 + millis() % 1000;
    }
}

int TimeManager::getNtpStratum() {
    if (timeSync->synchronized && !gpsMonitor->inFallbackMode) {
        return 1; // Stratum 1 when GPS synchronized
    } else {
        return 3; // Stratum 3 when RTC fallback
    }
}

void TimeManager::processPpsSync(const GpsSummaryData& gpsData) {
    extern bool gpsConnected;
    
    if (ppsReceived && gpsConnected) {
        ppsReceived = false; // Reset flag
        
        if (gpsData.timeValid && gpsData.dateValid) {
            // Convert to Unix timestamp
            struct tm timeinfo = {0};
            timeinfo.tm_year = gpsData.year - 1900;
            timeinfo.tm_mon = gpsData.month - 1;
            timeinfo.tm_mday = gpsData.day;
            timeinfo.tm_hour = gpsData.hour;
            timeinfo.tm_min = gpsData.min;
            timeinfo.tm_sec = gpsData.sec;
            
            timeSync->gpsTime = mktime(&timeinfo);
            timeSync->ppsTime = ppsTimestamp;
            timeSync->synchronized = true;
            
            // Synchronize with RTC
            rtc->set(gpsData.sec, gpsData.min, gpsData.hour, 1, 
                    gpsData.day, gpsData.month, gpsData.year % 100);
            timeSync->rtcTime = timeSync->gpsTime;
            
            // Accuracy calculation (high precision with PPS signal)
            timeSync->accuracy = 0.001; // 1ms accuracy
            
#if defined(DEBUG_CONSOLE_PPS)
            Serial.print("PPS-GPS sync: ");
            Serial.print(gpsData.hour);
            Serial.print(":");
            Serial.print(gpsData.min);
            Serial.print(":");
            Serial.print(gpsData.sec);
            Serial.print(".");
            Serial.print(gpsData.msec);
            Serial.print(" PPS count: ");
            Serial.println(ppsCount);
#endif
        }
    }
}