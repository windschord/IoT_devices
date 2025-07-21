#include "NtpServer.h"
#include "HardwareConfig.h"

NtpServer::NtpServer(EthernetUDP* udpInstance, TimeManager* timeManagerInstance, UdpSocketManager* udpManagerInstance)
    : ntpUdp(udpInstance), timeManager(timeManagerInstance), udpManager(udpManagerInstance) {
    memset(ntpPacketBuffer, 0, NTP_PACKET_SIZE);
}

void NtpServer::init() {
    // Initialize NTP packet buffer
    memset(ntpPacketBuffer, 0, NTP_PACKET_SIZE);
}

void NtpServer::processRequests() {
    if (!udpManager->ntpSocketOpen) {
        return;
    }
    
    int packetSize = ntpUdp->parsePacket();
    if (packetSize >= NTP_PACKET_SIZE) {
        handleNtpRequest();
    }
}

void NtpServer::handleNtpRequest() {
    Serial.print("Received NTP request from ");
    Serial.print(ntpUdp->remoteIP());
    Serial.print(":");
    Serial.println(ntpUdp->remotePort());
    
    // Read NTP packet
    ntpUdp->read(ntpPacketBuffer, NTP_PACKET_SIZE);
    
    // Create NTP response (simplified implementation for Task 3 completion)
    // Full implementation will be completed in Task 4
    createNtpResponse();
    sendNtpResponse();
}

void NtpServer::createNtpResponse() {
    // Clear packet buffer
    memset(ntpPacketBuffer, 0, NTP_PACKET_SIZE);
    
    // Basic NTP response (simplified for testing purposes)
    ntpPacketBuffer[0] = 0b00100100; // LI=0, VN=4, Mode=4 (server)
    ntpPacketBuffer[1] = timeManager->getNtpStratum(); // Stratum
    
    // Time stamp implementation (basic version for Task 3)
    // Full implementation will be completed in Task 4
    unsigned long currentTime = timeManager->getHighPrecisionTime() / 1000; // seconds
    // NTP timestamp is seconds from 1900
    unsigned long ntpTime = currentTime + 2208988800UL;
    
    // Transmit timestamp (bytes 40-47)
    ntpPacketBuffer[40] = (ntpTime >> 24) & 0xFF;
    ntpPacketBuffer[41] = (ntpTime >> 16) & 0xFF;
    ntpPacketBuffer[42] = (ntpTime >> 8) & 0xFF;
    ntpPacketBuffer[43] = ntpTime & 0xFF;
}

void NtpServer::sendNtpResponse() {
    // Send response
    ntpUdp->beginPacket(ntpUdp->remoteIP(), ntpUdp->remotePort());
    ntpUdp->write(ntpPacketBuffer, NTP_PACKET_SIZE);
    ntpUdp->endPacket();
    
    Serial.println("NTP response sent");
}