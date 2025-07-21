#ifndef NTP_SERVER_H
#define NTP_SERVER_H

#include <Arduino.h>
#include <EthernetUdp.h>
#include "SystemTypes.h"
#include "TimeManager.h"

class NtpServer {
private:
    EthernetUDP* ntpUdp;
    TimeManager* timeManager;
    UdpSocketManager* udpManager;
    byte ntpPacketBuffer[48]; // NTP packet buffer
    
public:
    NtpServer(EthernetUDP* udpInstance, TimeManager* timeManagerInstance, UdpSocketManager* udpManagerInstance);
    
    void init();
    void processRequests();
    
private:
    void handleNtpRequest();
    void createNtpResponse();
    void sendNtpResponse();
};

#endif // NTP_SERVER_H