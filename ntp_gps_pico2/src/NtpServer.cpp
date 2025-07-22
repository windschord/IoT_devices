#include "NtpServer.h"
#include "HardwareConfig.h"

NtpServer::NtpServer(EthernetUDP* udpInstance, TimeManager* timeManagerInstance, UdpSocketManager* udpManagerInstance)
    : ntpUdp(udpInstance), timeManager(timeManagerInstance), udpManager(udpManagerInstance) {
    
    // Initialize packet buffers
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    memset(&receivedPacket, 0, sizeof(NtpPacket));
    memset(&responsePacket, 0, sizeof(NtpPacket));
    
    // Initialize statistics
    memset(&stats, 0, sizeof(NtpStatistics));
    
    // Initialize timing variables
    receiveTimestamp_us = 0;
    transmitTimestamp_us = 0;
}

void NtpServer::init() {
    resetStatistics();
    Serial.println("NTP Server initialized - Ready to serve time");
}

void NtpServer::processRequests() {
    if (!udpManager->ntpSocketOpen) {
        return;
    }
    
    int packetSize = ntpUdp->parsePacket();
    if (packetSize > 0) {
        // Record receive timestamp as early as possible for precision
        receiveTimestamp_us = micros();
        
        currentClientIP = ntpUdp->remoteIP();
        currentClientPort = ntpUdp->remotePort();
        
        uint32_t startTime = millis();
        bool validRequest = false;
        
        if (packetSize >= NTP_PACKET_SIZE) {
            // Read the packet
            int bytesRead = ntpUdp->read(packetBuffer, NTP_PACKET_SIZE);
            
            if (bytesRead == NTP_PACKET_SIZE) {
                // Parse and validate the request
                if (parseNtpRequest(packetBuffer, bytesRead)) {
                    if (validateNtpRequest(receivedPacket)) {
                        // Create and send response
                        createNtpResponse();
                        if (sendNtpResponse()) {
                            validRequest = true;
                            stats.responses_sent++;
                        }
                    }
                }
            }
        }
        
        // Update statistics
        float processingTime = millis() - startTime;
        updateStatistics(validRequest, processingTime);
        logRequest(currentClientIP, validRequest);
    }
}

bool NtpServer::parseNtpRequest(const byte* buffer, size_t length) {
    if (length < NTP_PACKET_SIZE) {
        return false;
    }
    
    // Parse NTP packet fields
    receivedPacket.li_vn_mode = buffer[0];
    receivedPacket.stratum = buffer[1];
    receivedPacket.poll = buffer[2];
    receivedPacket.precision = buffer[3];
    
    // Parse 32-bit fields (convert from network byte order)
    receivedPacket.root_delay = ntohl(*(uint32_t*)&buffer[4]);
    receivedPacket.root_dispersion = ntohl(*(uint32_t*)&buffer[8]);
    receivedPacket.reference_id = ntohl(*(uint32_t*)&buffer[12]);
    
    // Parse timestamp fields
    receivedPacket.reference_timestamp.seconds = ntohl(*(uint32_t*)&buffer[16]);
    receivedPacket.reference_timestamp.fraction = ntohl(*(uint32_t*)&buffer[20]);
    
    receivedPacket.origin_timestamp.seconds = ntohl(*(uint32_t*)&buffer[24]);
    receivedPacket.origin_timestamp.fraction = ntohl(*(uint32_t*)&buffer[28]);
    
    receivedPacket.receive_timestamp.seconds = ntohl(*(uint32_t*)&buffer[32]);
    receivedPacket.receive_timestamp.fraction = ntohl(*(uint32_t*)&buffer[36]);
    
    receivedPacket.transmit_timestamp.seconds = ntohl(*(uint32_t*)&buffer[40]);
    receivedPacket.transmit_timestamp.fraction = ntohl(*(uint32_t*)&buffer[44]);
    
    return true;
}

bool NtpServer::validateNtpRequest(const NtpPacket& packet) {
    uint8_t version = NTP_GET_VN(packet.li_vn_mode);
    uint8_t mode = NTP_GET_MODE(packet.li_vn_mode);
    
    // Check version (accept NTPv3 or NTPv4)
    if (version < 3 || version > 4) {
        return false;
    }
    
    // Check mode (must be client mode)
    if (mode != NTP_MODE_CLIENT) {
        return false;
    }
    
    // Check rate limiting
    if (isRateLimited(currentClientIP)) {
        return false;
    }
    
    return true;
}

bool NtpServer::isRateLimited(IPAddress clientIP) {
    // Simple rate limiting: allow max 1 request per second per client
    // In a full implementation, this would track multiple clients
    static IPAddress lastClientIP;
    static uint32_t lastRequestTime = 0;
    
    uint32_t now = millis();
    if (clientIP == lastClientIP && (now - lastRequestTime) < 1000) {
        return true; // Rate limited
    }
    
    lastClientIP = clientIP;
    lastRequestTime = now;
    return false;
}

void NtpServer::createNtpResponse() {
    // Clear response packet
    memset(&responsePacket, 0, sizeof(NtpPacket));
    
    // Set Leap Indicator, Version, and Mode
    uint8_t leapIndicator = NTP_LI_NO_WARNING;  // No leap second warning
    uint8_t version = NTP_GET_VN(receivedPacket.li_vn_mode);  // Echo client version
    uint8_t mode = NTP_MODE_SERVER;  // Server mode
    
    responsePacket.li_vn_mode = NTP_SET_LI_VN_MODE(leapIndicator, version, mode);
    
    // Set server parameters
    responsePacket.stratum = calculateStratum();
    responsePacket.poll = receivedPacket.poll;  // Echo client poll interval
    responsePacket.precision = calculatePrecision();
    responsePacket.root_delay = htonl(calculateRootDelay());
    responsePacket.root_dispersion = htonl(calculateRootDispersion());
    responsePacket.reference_id = htonl(getReferenceId());
    
    // Set timestamps
    responsePacket.reference_timestamp = htonTimestamp(getReferenceTimestamp());
    responsePacket.origin_timestamp = htonTimestamp(receivedPacket.transmit_timestamp);  // Echo client's transmit time (convert to network byte order)
    
    // Set receive timestamp (when we received the client's request)
    uint32_t receiveUnixTime = timeManager->getHighPrecisionTime() / 1000;
    uint32_t receiveMicroseconds = receiveTimestamp_us % 1000000;
    responsePacket.receive_timestamp = htonTimestamp(unixToNtpTimestamp(receiveUnixTime, receiveMicroseconds));
    
    // Transmit timestamp will be set just before sending
}

bool NtpServer::sendNtpResponse() {
    // Set transmit timestamp as late as possible for precision
    transmitTimestamp_us = micros();
    uint32_t transmitUnixTime = timeManager->getHighPrecisionTime() / 1000;
    uint32_t transmitMicroseconds = transmitTimestamp_us % 1000000;
    
    // Debug NTP timestamp conversion
    NtpTimestamp ntpTs = unixToNtpTimestamp(transmitUnixTime, transmitMicroseconds);
    Serial.printf("NTP Timestamp Debug - Unix: %lu, NTP: %lu (0x%08X), Expected: %lu\n", 
                  transmitUnixTime, ntpTs.seconds, ntpTs.seconds, transmitUnixTime + 2208988800UL);
    
    responsePacket.transmit_timestamp = htonTimestamp(ntpTs);
    
    // Convert response packet to byte array
    byte* response = (byte*)&responsePacket;
    
    // Send response
    bool success = ntpUdp->beginPacket(currentClientIP, currentClientPort);
    if (success) {
        size_t bytesWritten = ntpUdp->write(response, NTP_PACKET_SIZE);
        success = (bytesWritten == NTP_PACKET_SIZE) && ntpUdp->endPacket();
    }
    
    if (success) {
        Serial.print("NTP response sent to ");
        Serial.print(currentClientIP);
        Serial.print(" (Stratum ");
        Serial.print(responsePacket.stratum);
        Serial.println(")");
    } else {
        Serial.println("Failed to send NTP response");
    }
    
    return success;
}

NtpTimestamp NtpServer::getCurrentNtpTimestamp() {
    uint32_t unixTime = timeManager->getHighPrecisionTime() / 1000;
    uint32_t microseconds = micros() % 1000000;
    return unixToNtpTimestamp(unixTime, microseconds);
}

NtpTimestamp NtpServer::getHighPrecisionTimestamp(uint32_t microsecond_offset) {
    uint32_t unixTime = timeManager->getHighPrecisionTime() / 1000;
    uint32_t totalMicroseconds = (micros() + microsecond_offset) % 1000000;
    return unixToNtpTimestamp(unixTime, totalMicroseconds);
}

uint8_t NtpServer::calculateStratum() {
    return timeManager->getNtpStratum();
}

int8_t NtpServer::calculatePrecision() {
    // Return precision based on clock source
    // GPS with PPS: ~1 microsecond precision = 2^-20 seconds ≈ -20
    // RTC fallback: ~1 millisecond precision = 2^-10 seconds ≈ -10
    if (timeManager->getNtpStratum() == 1) {
        return -20;  // High precision with GPS+PPS
    } else {
        return -10;  // Lower precision with RTC
    }
}

uint32_t NtpServer::calculateRootDelay() {
    // Root delay to primary reference source
    // For GPS: very small delay (microseconds)
    // Convert to NTP short format (16.16 fixed point)
    if (timeManager->getNtpStratum() == 1) {
        return 0x00000001;  // ~15 microseconds
    } else {
        return 0x00001000;  // ~250 microseconds
    }
}

uint32_t NtpServer::calculateRootDispersion() {
    // Maximum error due to clock frequency tolerance
    // Convert to NTP short format (16.16 fixed point)
    if (timeManager->getNtpStratum() == 1) {
        return 0x00000010;  // ~1 microsecond
    } else {
        return 0x00001000;  // ~250 microseconds
    }
}

uint32_t NtpServer::getReferenceId() {
    if (timeManager->getNtpStratum() == 1) {
        return NTP_REFID_GPS;  // "GPS\0"
    } else {
        // For stratum > 1, typically would be IP of upstream server
        // For fallback, use a generic identifier
        return 0x52544300;  // "RTC\0"
    }
}

NtpTimestamp NtpServer::getReferenceTimestamp() {
    // Time when local clock was last set or corrected
    // For simplicity, use current time minus a small offset
    uint32_t refTime = timeManager->getHighPrecisionTime() / 1000 - 1;
    NtpTimestamp refTimestamp = unixToNtpTimestamp(refTime, 0);
    
    // Debug reference timestamp
    Serial.printf("Reference Timestamp Debug - Unix: %lu, NTP: %lu (0x%08X)\n", 
                  refTime, refTimestamp.seconds, refTimestamp.seconds);
    
    return refTimestamp;
}

void NtpServer::updateStatistics(bool validRequest, float processingTimeMs) {
    stats.requests_total++;
    stats.last_request_time = millis();
    
    if (validRequest) {
        stats.requests_valid++;
    } else {
        stats.requests_invalid++;
    }
    
    // Update rolling average processing time
    if (stats.requests_total == 1) {
        stats.avg_processing_time = processingTimeMs;
    } else {
        stats.avg_processing_time = (stats.avg_processing_time * 0.9f) + (processingTimeMs * 0.1f);
    }
}

void NtpServer::logRequest(IPAddress clientIP, bool valid) {
    Serial.print("NTP ");
    Serial.print(valid ? "VALID" : "INVALID");
    Serial.print(" request from ");
    Serial.print(clientIP);
    Serial.print(" - Total: ");
    Serial.print(stats.requests_total);
    Serial.print(", Valid: ");
    Serial.print(stats.requests_valid);
    Serial.print(", Avg processing: ");
    Serial.print(stats.avg_processing_time, 2);
    Serial.println("ms");
}

void NtpServer::resetStatistics() {
    memset(&stats, 0, sizeof(NtpStatistics));
    Serial.println("NTP Server statistics reset");
}