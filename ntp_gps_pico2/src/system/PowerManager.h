#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>

// Forward declaration
class LoggingService;

/**
 * @brief 電源管理・安定性向上クラス
 * Raspberry Pi Pico2 の電源監視、ウォッチドッグ、安定性向上機能を提供
 */
class PowerManager {
private:
    LoggingService* loggingService;
    
    // 電圧監視設定
    struct VoltageMonitoring {
        float currentVoltage;
        float minVoltage;
        float maxVoltage;
        float warningThreshold;
        float criticalThreshold;
        unsigned long lastCheck;
        uint16_t checkInterval; // seconds
        bool voltageStable;
    } voltageMonitor;
    
    // ウォッチドッグ設定
    struct WatchdogConfig {
        bool enabled;
        unsigned long timeout;
        unsigned long lastFeed;
        unsigned long feedInterval;
        uint8_t maxMissedFeeds;
        uint8_t missedFeedCount;
    } watchdog;
    
    // システム安定性監視
    struct StabilityMonitor {
        unsigned long uptimeSeconds;
        uint16_t rebootCount;
        unsigned long lastReboot;
        float cpuTemperature;
        bool thermalThrottling;
        unsigned long freeHeapMemory;
        unsigned long minFreeHeap;
    } stability;
    
    // 電源状態
    enum PowerState {
        POWER_NORMAL,
        POWER_WARNING,
        POWER_CRITICAL,
        POWER_EMERGENCY
    } currentPowerState;

public:
    PowerManager();
    void setLoggingService(LoggingService* loggingServiceInstance) { loggingService = loggingServiceInstance; }
    
    void init();
    void update();
    
    // 電圧監視機能
    void checkVoltage();
    float getVoltage() const { return voltageMonitor.currentVoltage; }
    bool isVoltageStable() const { return voltageMonitor.voltageStable; }
    PowerState getPowerState() const { return currentPowerState; }
    
    // ウォッチドッグ機能
    void enableWatchdog(unsigned long timeoutMs);
    void disableWatchdog();
    void feedWatchdog();
    bool isWatchdogEnabled() const { return watchdog.enabled; }
    
    // システム安定性監視
    void updateSystemMetrics();
    float getCpuTemperature();
    unsigned long getFreeHeapMemory();
    unsigned long getUptimeSeconds() const { return stability.uptimeSeconds; }
    uint16_t getRebootCount() const { return stability.rebootCount; }
    
    // 緊急時対応
    void handlePowerEmergency();
    void performControlledReboot(const char* reason);
    
private:
    void initializeVoltageMonitor();
    void initializeWatchdog();
    void initializeStabilityMonitor();
    
    void handleVoltageWarning();
    void handleVoltageCritical();
    void updatePowerState();
    
    void checkWatchdogTimeout();
    void resetWatchdog();
    
    float readInternalVoltage();
    float readCpuTemperatureInternal();
};

#endif // POWER_MANAGER_H