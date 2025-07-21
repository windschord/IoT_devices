#ifndef NTP_SERVER_H
#define NTP_SERVER_H

#include <Arduino.h>
#include <EthernetUdp.h>
#include "SystemTypes.h"
#include "TimeManager.h"
#include "NtpTypes.h"

class NtpServer {
private:
    EthernetUDP* ntpUdp;
    TimeManager* timeManager;
    UdpSocketManager* udpManager;
    
    // NTP packet buffers
    byte packetBuffer[NTP_PACKET_SIZE];
    NtpPacket receivedPacket;
    NtpPacket responsePacket;
    
    // NTP server statistics
    NtpStatistics stats;
    
    // Client information for current request
    IPAddress currentClientIP;
    uint16_t currentClientPort;
    
    // Timing for high-precision timestamps
    uint32_t receiveTimestamp_us;  // Microsecond timestamp when packet was received
    uint32_t transmitTimestamp_us; // Microsecond timestamp when response is sent
    
public:
    NtpServer(EthernetUDP* udpInstance, TimeManager* timeManagerInstance, UdpSocketManager* udpManagerInstance);
    
    void init();
    void processRequests();
    
    // Statistics and monitoring
    const NtpStatistics& getStatistics() const { return stats; }
    void resetStatistics();
    
private:
    // Packet processing
    bool parseNtpRequest(const byte* buffer, size_t length);
    void createNtpResponse();
    bool sendNtpResponse();
    
    // Timestamp generation
    NtpTimestamp getCurrentNtpTimestamp();
    NtpTimestamp getHighPrecisionTimestamp(uint32_t microsecond_offset = 0);
    
    // Validation and filtering
    bool validateNtpRequest(const NtpPacket& packet);
    bool isRateLimited(IPAddress clientIP);
    
    // Packet field calculations
    uint8_t calculateStratum();
    int8_t calculatePrecision();
    uint32_t calculateRootDelay();
    uint32_t calculateRootDispersion();
    uint32_t getReferenceId();
    NtpTimestamp getReferenceTimestamp();
    
    // Statistics update
    void updateStatistics(bool validRequest, float processingTimeMs);
    void logRequest(IPAddress clientIP, bool valid);
};

#endif // NTP_SERVER_H