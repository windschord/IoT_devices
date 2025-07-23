#include "TimeManager.h"
#include "HardwareConfig.h"

// UTC時刻からUnixタイムスタンプを計算する関数
time_t gpsTimeToUnixTimestamp(uint16_t year, uint8_t month, uint8_t day, 
                             uint8_t hour, uint8_t min, uint8_t sec) {
    // Unixエポック (1970年1月1日) からの経過秒数を計算
    
    // 1970年からの経過年数
    int years_since_epoch = year - 1970;
    
    // うるう年の数を計算
    int leap_years = 0;
    for (int y = 1970; y < year; y++) {
        if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) {
            leap_years++;
        }
    }
    
    // 各月の日数（平年）
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    // 現在年がうるう年かチェック
    bool is_leap_year = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
    if (is_leap_year) {
        days_in_month[1] = 29; // 2月
    }
    
    // 年の日数を計算
    int total_days = years_since_epoch * 365 + leap_years;
    
    // 月の日数を追加
    for (int m = 1; m < month; m++) {
        total_days += days_in_month[m - 1];
    }
    
    // 日を追加（1日ベースなので-1）
    total_days += day - 1;
    
    // 秒に変換
    time_t timestamp = (time_t)total_days * 24 * 60 * 60;
    timestamp += hour * 60 * 60;
    timestamp += min * 60;
    timestamp += sec;
    
    return timestamp;
}

TimeManager::TimeManager(RTC_DS3231* rtcInstance, TimeSync* timeSyncInstance, const GpsMonitor* gpsMonitorInstance)
    : rtc(rtcInstance), timeSync(timeSyncInstance), gpsMonitor(gpsMonitorInstance),
      ppsReceived(false), ppsTimestamp(0), ppsCount(0) {
}

void TimeManager::init() {
    // Initialize PPS pin
    pinMode(GPS_PPS_PIN, INPUT_PULLUP);
    
    // Reset sync state
    timeSync->synchronized = false;
    timeSync->accuracy = 1.0;
    timeSync->lastGpsUpdate = 0; // GPS更新時刻を初期化
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
    
    // Debug time source selection
    static unsigned long lastTimeDebug = 0;
    if (millis() - lastTimeDebug > 10000) { // Every 10 seconds
        Serial.print("TimeManager::getHighPrecisionTime() - Using ");
        if (timeSync->synchronized && gpsConnected && gpsMonitor && !gpsMonitor->inFallbackMode) {
            Serial.print("GPS time. GPS time: ");
            Serial.print(timeSync->gpsTime);
            Serial.print(", PPS time: ");
            Serial.println(timeSync->ppsTime);
        } else {
            Serial.print("RTC fallback time. Reasons: ");
            Serial.print("synchronized="); Serial.print(timeSync->synchronized);
            Serial.print(", gpsConnected="); Serial.print(gpsConnected);
            Serial.print(", gpsMonitor="); Serial.print(gpsMonitor ? "OK" : "NULL");
            Serial.print(", inFallbackMode="); 
            if (gpsMonitor) {
                Serial.println(gpsMonitor->inFallbackMode);
            } else {
                Serial.println("N/A");
            }
        }
        lastTimeDebug = millis();
    }
    
    // Debug condition check
    static unsigned long lastConditionDebug = 0;
    if (millis() - lastConditionDebug > 3000) { // Every 3 seconds
        unsigned long timeSinceGpsUpdate = millis() - timeSync->lastGpsUpdate;
        bool gpsTimeValid = (timeSync->synchronized && timeSync->gpsTime > 1000000000);
        bool gpsRecentlyUpdated = (timeSinceGpsUpdate < 30000);
        
        Serial.printf("GPS Condition Debug - gpsTimeValid: %s, recentlyUpdated: %s (age: %lu ms)\n",
                     gpsTimeValid ? "YES" : "NO",
                     gpsRecentlyUpdated ? "YES" : "NO",
                     timeSinceGpsUpdate);
        Serial.printf("GPS Condition Debug - timeSync->gpsTime: %lu, synchronized: %s\n",
                     timeSync->gpsTime, timeSync->synchronized ? "YES" : "NO");
        lastConditionDebug = millis();
    }
    
    // GPS時刻の有効性を優先的にチェック
    bool gpsTimeValid = (timeSync->synchronized && timeSync->gpsTime > 1000000000); // 2001年以降
    bool gpsRecentlyUpdated = (millis() - timeSync->lastGpsUpdate < 30000); // 30秒以内
    
    if (gpsTimeValid && gpsRecentlyUpdated) {
        // Return high-precision PPS-synchronized time
        unsigned long elapsed = micros() - timeSync->ppsTime;
        
        // 64ビット演算を使用してオーバーフローを防ぐ
        uint64_t gpsTimeMs64 = (uint64_t)timeSync->gpsTime * 1000ULL;
        uint64_t elapsedMs64 = elapsed / 1000ULL;
        uint64_t result64 = gpsTimeMs64 + elapsedMs64;
        
        // 32ビットに安全に収まるかチェック
        unsigned long result;
        if (result64 > ULONG_MAX) {
            // オーバーフローの場合は秒単位で計算
            result = timeSync->gpsTime * 1000UL; // 注意：まだオーバーフローする可能性
            Serial.printf("WARNING: 64-bit overflow detected, using approximate calculation\n");
        } else {
            result = (unsigned long)result64;
        }
        
        // Debug GPS time usage
        static unsigned long lastGpsTimeDebug = 0;
        if (millis() - lastGpsTimeDebug > 5000) { // Every 5 seconds
            Serial.printf("GPS Time Detail Debug:\n");
            Serial.printf("  timeSync->gpsTime: %lu (Unix seconds)\n", timeSync->gpsTime);
            Serial.printf("  64-bit gpsTimeMs: %llu (milliseconds)\n", gpsTimeMs64);
            Serial.printf("  32-bit max: %lu\n", ULONG_MAX);
            Serial.printf("  elapsed microseconds: %lu\n", elapsed);
            Serial.printf("  elapsed milliseconds: %llu\n", elapsedMs64);
            Serial.printf("  64-bit result: %llu (milliseconds)\n", result64);
            Serial.printf("  final 32-bit result: %lu (milliseconds)\n", result);
            Serial.printf("  result as seconds: %lu\n", result / 1000);
            lastGpsTimeDebug = millis();
        }
        
        return result;
    } else {
        // RTC fallback time
        static unsigned long lastFallbackDebug = 0;
        if (millis() - lastFallbackDebug > 5000) { // Every 5 seconds
            Serial.printf("Using RTC Fallback - GPS conditions not met\n");
            lastFallbackDebug = millis();
        }
        
        DateTime now = rtc->now();
        struct tm timeinfo = {0};
        timeinfo.tm_year = now.year() - 1900; // Convert to years since 1900
        timeinfo.tm_mon = now.month() - 1;    // Convert to 0-11 range
        timeinfo.tm_mday = now.day();
        timeinfo.tm_hour = now.hour();
        timeinfo.tm_min = now.minute();
        timeinfo.tm_sec = now.second();
        
        // Check if RTC time is reasonable (after 2020)
        time_t rtcTime = mktime(&timeinfo);
        time_t year2020 = 1577836800; // 2020-01-01 00:00:00 UTC
        
        if (rtcTime < year2020) {
            // RTC time is invalid, use a default reasonable time
            // 2025-01-21 12:00:00 as fallback
            timeinfo.tm_year = 125; // 2025
            timeinfo.tm_mon = 0;    // January
            timeinfo.tm_mday = 21;  // 21st
            timeinfo.tm_hour = 12;
            timeinfo.tm_min = 0;
            timeinfo.tm_sec = 0;
            rtcTime = mktime(&timeinfo);
        }
        
        return rtcTime * 1000 + millis() % 1000;
    }
}

time_t TimeManager::getUnixTimestamp() {
    // GPS時刻の有効性を優先的にチェック
    bool gpsTimeValid = (timeSync->synchronized && timeSync->gpsTime > 1000000000);
    bool gpsRecentlyUpdated = (millis() - timeSync->lastGpsUpdate < 30000);
    
    if (gpsTimeValid && gpsRecentlyUpdated) {
        // GPS時刻を秒単位で返す（オーバーフロー無し）
        unsigned long elapsedSec = (micros() - timeSync->ppsTime) / 1000000;
        time_t result = timeSync->gpsTime + elapsedSec;
        
        static unsigned long lastGpsDebug = 0;
        if (millis() - lastGpsDebug > 5000) {
            Serial.printf("GPS Unix Timestamp - GPS base: %lu, elapsed: %lu sec, result: %lu\n",
                         timeSync->gpsTime, elapsedSec, result);
            lastGpsDebug = millis();
        }
        
        return result;
    } else {
        // RTC fallback
        DateTime now = rtc->now();
        struct tm timeinfo = {0};
        timeinfo.tm_year = now.year() - 1900;
        timeinfo.tm_mon = now.month() - 1;
        timeinfo.tm_mday = now.day();
        timeinfo.tm_hour = now.hour();
        timeinfo.tm_min = now.minute();
        timeinfo.tm_sec = now.second();
        
        time_t rtcTime = mktime(&timeinfo);
        time_t year2020 = 1577836800;
        
        if (rtcTime < year2020) {
            // RTCが無効な場合のデフォルト時刻
            return 1737504000; // 2025-01-21 12:00:00
        }
        
        return rtcTime;
    }
}

uint32_t TimeManager::getMicrosecondFraction() {
    // GPS時刻の有効性を優先的にチェック
    bool gpsTimeValid = (timeSync->synchronized && timeSync->gpsTime > 1000000000);
    bool gpsRecentlyUpdated = (millis() - timeSync->lastGpsUpdate < 30000);
    
    if (gpsTimeValid && gpsRecentlyUpdated) {
        // PPS信号からの経過マイクロ秒
        unsigned long elapsed = micros() - timeSync->ppsTime;
        uint32_t microsInSecond = elapsed % 1000000;
        
        // マイクロ秒をNTPフラクション部に変換（2^32 * microseconds / 1000000）
        return (uint32_t)((uint64_t)microsInSecond * 4294967296ULL / 1000000ULL);
    } else {
        // RTCフォールバック時は現在のミリ秒を使用
        return (uint32_t)((uint64_t)(millis() % 1000) * 4294967296ULL / 1000ULL);
    }
}

int TimeManager::getNtpStratum() {
    // GPS時刻の有効性を優先的にチェック
    bool gpsTimeValid = (timeSync->synchronized && timeSync->gpsTime > 1000000000);
    bool gpsRecentlyUpdated = (millis() - timeSync->lastGpsUpdate < 30000);
    
    if (gpsTimeValid && gpsRecentlyUpdated) {
        return 1; // Stratum 1 when GPS synchronized and recent
    } else {
        return 3; // Stratum 3 when RTC fallback
    }
}

void TimeManager::processPpsSync(const GpsSummaryData& gpsData) {
    extern bool gpsConnected;
    
    // Debug output for GPS sync status
    static unsigned long lastDebugTime = 0;
    unsigned long now = millis();
    if (now - lastDebugTime > 5000) { // Every 5 seconds
        Serial.print("GPS Sync Debug - PPS: ");
        Serial.print(ppsReceived ? "YES" : "NO");
        Serial.print(", GPS Connected: ");
        Serial.print(gpsConnected ? "YES" : "NO");
        Serial.print(", Time Valid: ");
        Serial.print(gpsData.timeValid ? "YES" : "NO");
        Serial.print(", Date Valid: ");
        Serial.print(gpsData.dateValid ? "YES" : "NO");
        Serial.print(", Synchronized: ");
        Serial.print(timeSync->synchronized ? "YES" : "NO");
        Serial.print(", Fallback: ");
        Serial.println(gpsMonitor && gpsMonitor->inFallbackMode ? "YES" : "NO");
        lastDebugTime = now;
    }
    
    if (ppsReceived && gpsConnected) {
        ppsReceived = false; // Reset flag
        
        if (gpsData.timeValid && gpsData.dateValid) {
            // Debug GPS date/time data
            Serial.printf("GPS Date/Time Debug - Year: %d, Month: %d, Day: %d, Hour: %d, Min: %d, Sec: %d\n",
                         gpsData.year, gpsData.month, gpsData.day, 
                         gpsData.hour, gpsData.min, gpsData.sec);
            
            // Convert to Unix timestamp
            struct tm timeinfo = {0};
            timeinfo.tm_year = gpsData.year - 1900;
            timeinfo.tm_mon = gpsData.month - 1;
            timeinfo.tm_mday = gpsData.day;
            timeinfo.tm_hour = gpsData.hour;
            timeinfo.tm_min = gpsData.min;
            timeinfo.tm_sec = gpsData.sec;
            
            // Debug converted timeinfo
            Serial.printf("timeinfo - tm_year: %d, tm_mon: %d, tm_mday: %d\n",
                         timeinfo.tm_year, timeinfo.tm_mon, timeinfo.tm_mday);
            
            // 従来のmktime()の結果
            time_t mktime_result = mktime(&timeinfo);
            Serial.printf("mktime() result: %lu (Unix timestamp)\n", mktime_result);
            
            // 新しいUTC計算関数の結果
            time_t utc_result = gpsTimeToUnixTimestamp(gpsData.year, gpsData.month, gpsData.day,
                                                      gpsData.hour, gpsData.min, gpsData.sec);
            Serial.printf("UTC calculation result: %lu (Unix timestamp)\n", utc_result);
            
            // UTCタイムスタンプを使用（より信頼性が高い）
            timeSync->gpsTime = utc_result;
            timeSync->ppsTime = ppsTimestamp;
            timeSync->lastGpsUpdate = millis(); // GPS更新時刻を記録
            timeSync->synchronized = true;
            
            // Debug GPS time sync
            Serial.printf("GPS Time Sync - Set timeSync->gpsTime to: %lu (UTC timestamp)\n", utc_result);
            
            // Synchronize with RTC
            Serial.printf("RTC Update - Setting RTC to GPS time: 20%02d/%02d/%02d %02d:%02d:%02d\n",
                         gpsData.year % 100, gpsData.month, gpsData.day,
                         gpsData.hour, gpsData.min, gpsData.sec);
            
            // Test I2C communication before setting RTC
            extern TwoWire Wire1;
            Wire1.beginTransmission(0x68);
            byte preCommError = Wire1.endTransmission();
            Serial.printf("RTC I2C Pre-write test: %s (error: %d)\n", 
                         preCommError == 0 ? "SUCCESS" : "FAILED", preCommError);
            
            // Manual DS3231 time setting (bypassing uRTCLib)
            Serial.println("Manual DS3231 time setting:");
            
            // Convert decimal to BCD
            byte secBCD = ((gpsData.sec / 10) << 4) | (gpsData.sec % 10);
            byte minBCD = ((gpsData.min / 10) << 4) | (gpsData.min % 10);
            byte hourBCD = ((gpsData.hour / 10) << 4) | (gpsData.hour % 10);
            byte dayBCD = ((gpsData.day / 10) << 4) | (gpsData.day % 10);
            byte monthBCD = ((gpsData.month / 10) << 4) | (gpsData.month % 10);
            byte yearBCD = (((gpsData.year % 100) / 10) << 4) | ((gpsData.year % 100) % 10);
            
            Serial.printf("BCD values: sec=%02X min=%02X hour=%02X day=%02X month=%02X year=%02X\n",
                         secBCD, minBCD, hourBCD, dayBCD, monthBCD, yearBCD);
            
            // Write to DS3231 registers directly
            extern TwoWire Wire1;
            Wire1.beginTransmission(0x68);
            Wire1.write(0x00); // Start at seconds register
            Wire1.write(secBCD);
            Wire1.write(minBCD);
            Wire1.write(hourBCD);
            Wire1.write(0x01); // Day of week (Monday)
            Wire1.write(dayBCD);
            Wire1.write(monthBCD);
            Wire1.write(yearBCD);
            byte manualWriteError = Wire1.endTransmission();
            
            Serial.printf("Manual DS3231 write result: %s (error: %d)\n",
                         manualWriteError == 0 ? "SUCCESS" : "FAILED", manualWriteError);
            
            // RTClib method - create DateTime object and adjust RTC
            DateTime gpsDateTime(gpsData.year, gpsData.month, gpsData.day,
                                gpsData.hour, gpsData.min, gpsData.sec);
            rtc->adjust(gpsDateTime);
            Serial.printf("RTClib adjust() operation completed\n");
            timeSync->rtcTime = timeSync->gpsTime;
            
            // Small delay to ensure write completes
            delay(10);
            
            // Manual verification - read DS3231 registers directly
            Serial.println("Manual DS3231 verification:");
            Wire1.beginTransmission(0x68);
            Wire1.write(0x00); // Point to seconds register
            byte verifyError = Wire1.endTransmission();
            if (verifyError == 0) {
                Wire1.requestFrom(0x68, 7);
                if (Wire1.available() >= 7) {
                    byte seconds = Wire1.read();
                    byte minutes = Wire1.read();
                    byte hours = Wire1.read();
                    byte dayOfWeek = Wire1.read();
                    byte date = Wire1.read();
                    byte month = Wire1.read();
                    byte year = Wire1.read();
                    
                    Serial.printf("Manual read after write: %02X %02X %02X %02X %02X %02X %02X\n",
                                 seconds, minutes, hours, dayOfWeek, date, month, year);
                    
                    // Convert BCD to decimal
                    byte secDec = ((seconds >> 4) * 10) + (seconds & 0x0F);
                    byte minDec = ((minutes >> 4) * 10) + (minutes & 0x0F);
                    byte hourDec = ((hours >> 4) * 10) + (hours & 0x0F);
                    byte dateDec = ((date >> 4) * 10) + (date & 0x0F);
                    byte monthDec = ((month >> 4) * 10) + (month & 0x0F);
                    byte yearDec = ((year >> 4) * 10) + (year & 0x0F);
                    
                    Serial.printf("Manual verification: 20%02d/%02d/%02d %02d:%02d:%02d\n",
                                 yearDec, monthDec, dateDec, hourDec, minDec, secDec);
                }
            }
            
            // Verify RTC was updated correctly using RTClib
            DateTime afterUpdate = rtc->now();
            Serial.printf("RTClib verification: %04d/%02d/%02d %02d:%02d:%02d\n",
                         afterUpdate.year(), afterUpdate.month(), afterUpdate.day(),
                         afterUpdate.hour(), afterUpdate.minute(), afterUpdate.second());
            
            // Test I2C communication after setting RTC
            Wire1.beginTransmission(0x68);
            byte postCommError = Wire1.endTransmission();
            Serial.printf("RTC I2C Post-write test: %s (error: %d)\n", 
                         postCommError == 0 ? "SUCCESS" : "FAILED", postCommError);
            
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