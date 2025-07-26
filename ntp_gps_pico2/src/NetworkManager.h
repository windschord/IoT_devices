#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include "SystemTypes.h"

class NetworkManager {
private:
    NetworkMonitor networkMonitor;
    UdpSocketManager udpManager;
    EthernetUDP* ntpUdp;
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

public:
    NetworkManager(EthernetUDP* udpInstance);
    
    void init();
    void monitorConnection();
    void attemptReconnection();
    void manageUdpSockets();
    
    // Performance optimization: Non-blocking initialization
    bool updateInitialization(); // Returns true when complete
    
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