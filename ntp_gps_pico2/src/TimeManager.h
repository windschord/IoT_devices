#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <time.h>
#include "RTClib.h"
#include "SystemTypes.h"
#include "Gps_model.h"

// Forward declaration for LoggingService
class LoggingService;

class TimeManager {
private:
    RTC_DS3231* rtc;
    TimeSync* timeSync;
    const GpsMonitor* gpsMonitor;
    LoggingService* loggingService;
    volatile bool ppsReceived;
    volatile unsigned long ppsTimestamp;
    volatile unsigned long ppsCount;

public:
    TimeManager(RTC_DS3231* rtcInstance, TimeSync* timeSyncInstance, const GpsMonitor* gpsMonitorInstance);
    
    void init();
    void processPpsSync(const GpsSummaryData& gpsData);
    void onPpsInterrupt();
    void setGpsMonitor(const GpsMonitor* gpsMonitorInstance) { gpsMonitor = gpsMonitorInstance; }
    
    // LoggingService integration
    void setLoggingService(LoggingService* loggingServiceInstance);
    
    unsigned long getHighPrecisionTime();
    time_t getUnixTimestamp();  // GPS時刻を秒単位で取得（オーバーフロー回避）
    uint32_t getMicrosecondFraction(); // マイクロ秒精度の小数部を取得
    int getNtpStratum();
    
    bool isPpsReceived() const { return ppsReceived; }
    void resetPpsFlag() { ppsReceived = false; }
    unsigned long getPpsCount() const { return ppsCount; }
};

#endif // TIME_MANAGER_H