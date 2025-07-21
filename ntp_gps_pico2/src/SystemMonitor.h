#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include <Arduino.h>
#include "SystemTypes.h"
#include "Gps_Client.h"

class SystemMonitor {
private:
    GpsMonitor gpsMonitor;
    GpsClient* gpsClient;
    bool* gpsConnected;
    volatile bool* ppsReceived;

public:
    SystemMonitor(GpsClient* gpsClientInstance, bool* gpsConnectedPtr, volatile bool* ppsReceivedPtr);
    
    void init();
    void monitorGpsSignal();
    
    bool isInFallbackMode() const { return gpsMonitor.inFallbackMode; }
    int getSignalQuality() const { return gpsMonitor.signalQuality; }
    int getSatelliteCount() const { return gpsMonitor.satelliteCount; }
    bool isPpsActive() const { return gpsMonitor.ppsActive; }
    bool isGpsTimeValid() const { return gpsMonitor.gpsTimeValid; }
    
    const GpsMonitor& getGpsMonitor() const { return gpsMonitor; }

private:
    void updateGpsStatus();
    void updatePpsStatus();
    void evaluateFallbackMode();
    void handleFallbackTransition();
};

#endif // SYSTEM_MONITOR_H