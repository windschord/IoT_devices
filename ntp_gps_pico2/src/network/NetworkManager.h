#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include "../system/SystemTypes.h"

// Forward declarations
class LoggingService;
class ConfigManager;

class NetworkManager {
private:
    NetworkMonitor networkMonitor;
    UdpSocketManager udpManager;
    EthernetUDP* ntpUdp;
    LoggingService* loggingService;
    ConfigManager* configManager;
    byte mac[6];
    
    // Performance optimization: Non-blocking initialization state machine
    enum InitState {
        INIT_START,
        RESET_LOW,
        RESET_HIGH,
        STABILIZE_WAIT,
        SPI_INIT,
        ETHERNET_INIT,
        INIT_COMPLETE
    };
    InitState initState;
    unsigned long stateChangeTime;
    
    // 強化された自動復旧機能
    struct AutoRecoveryConfig {
        unsigned long lastRecoveryAttempt;
        uint8_t consecutiveFailures;
        uint8_t maxConsecutiveFailures;
        unsigned long recoveryBackoffTime;
        unsigned long maxBackoffTime;
        bool hardwareResetRequired;
    } autoRecovery;
    
    // W5500ハードウェア監視
    struct HardwareMonitoring {
        unsigned long lastHealthCheck;
        uint8_t healthCheckInterval;
        bool hardwareResponsive;
        uint8_t consecutiveTimeouts;
        uint8_t maxTimeouts;
    } hardwareStatus;

public:
    NetworkManager(EthernetUDP* udpInstance);
    void setLoggingService(LoggingService* loggingServiceInstance) { loggingService = loggingServiceInstance; }
    void setConfigManager(ConfigManager* configManagerInstance) { configManager = configManagerInstance; }
    
    void init();
    void monitorConnection();
    void attemptReconnection();
    void manageUdpSockets();
    
    // Performance optimization: Non-blocking initialization
    bool updateInitialization(); // Returns true when complete
    
    // 強化された自動復旧機能
    void performHealthCheck();
    bool performHardwareReset();
    void handleConnectionFailure();
    bool isAutoRecoveryNeeded();
    void resetAutoRecoveryCounters();
    
    bool isConnected() const { return networkMonitor.isConnected; }
    bool isNtpServerActive() const { return networkMonitor.ntpServerActive; }
    bool isUdpSocketOpen() const { return udpManager.ntpSocketOpen; }
    
    const NetworkMonitor& getNetworkStatus() const { return networkMonitor; }
    const UdpSocketManager& getUdpStatus() const { return udpManager; }
    
private:
    void initializeW5500();
    bool attemptDhcp();
    void setupStaticIp();
    void checkHardwareStatus();
    void checkLinkStatus();
    void maintainDhcp();
};

#endif // NETWORK_MANAGER_H