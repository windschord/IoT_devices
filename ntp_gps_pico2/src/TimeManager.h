#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <time.h>
#include <uRTCLib.h>
#include "SystemTypes.h"
#include "Gps_model.h"

class TimeManager {
private:
    uRTCLib* rtc;
    TimeSync* timeSync;
    const GpsMonitor* gpsMonitor;
    volatile bool ppsReceived;
    volatile unsigned long ppsTimestamp;
    volatile unsigned long ppsCount;

public:
    TimeManager(uRTCLib* rtcInstance, TimeSync* timeSyncInstance, const GpsMonitor* gpsMonitorInstance);
    
    void init();
    void processPpsSync(const GpsSummaryData& gpsData);
    void onPpsInterrupt();
    void setGpsMonitor(const GpsMonitor* gpsMonitorInstance) { gpsMonitor = gpsMonitorInstance; }
    
    unsigned long getHighPrecisionTime();
    int getNtpStratum();
    
    bool isPpsReceived() const { return ppsReceived; }
    void resetPpsFlag() { ppsReceived = false; }
    unsigned long getPpsCount() const { return ppsCount; }
};

#endif // TIME_MANAGER_H