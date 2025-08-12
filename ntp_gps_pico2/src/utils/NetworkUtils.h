#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

#include <Arduino.h>
#include <Ethernet.h>

/**
 * @brief Network utilities for common networking operations
 * 
 * Provides helper functions for IP address manipulation, network validation,
 * and common networking patterns used across the application.
 */
class NetworkUtils {
public:
    /**
     * @brief Validate IP address format
     * @param ip IPAddress to validate
     * @return true if IP address is valid (not 0.0.0.0 or 255.255.255.255)
     */
    static bool isValidIPAddress(const IPAddress& ip) {
        // Check for invalid addresses
        return !(ip == IPAddress(0, 0, 0, 0) || ip == IPAddress(255, 255, 255, 255));
    }

    /**
     * @brief Check if IP address is in private range
     * @param ip IPAddress to check
     * @return true if IP is in private range (RFC 1918)
     */
    static bool isPrivateIPAddress(const IPAddress& ip) {
        uint32_t addr = (uint32_t)ip;
        
        // 10.0.0.0/8 (10.0.0.0 - 10.255.255.255)
        if ((addr & 0xFF000000) == 0x0A000000) {
            return true;
        }
        
        // 172.16.0.0/12 (172.16.0.0 - 172.31.255.255)
        if ((addr & 0xFFF00000) == 0xAC100000) {
            return true;
        }
        
        // 192.168.0.0/16 (192.168.0.0 - 192.168.255.255)
        if ((addr & 0xFFFF0000) == 0xC0A80000) {
            return true;
        }
        
        return false;
    }

    /**
     * @brief Check if IP address is link-local
     * @param ip IPAddress to check
     * @return true if IP is link-local (169.254.x.x)
     */
    static bool isLinkLocalIPAddress(const IPAddress& ip) {
        uint32_t addr = (uint32_t)ip;
        // 169.254.0.0/16
        return (addr & 0xFFFF0000) == 0xA9FE0000;
    }

    /**
     * @brief Parse IP address from string
     * @param ipStr IP address string (e.g., "192.168.1.1")
     * @param ip Output IPAddress object
     * @return true if parsing succeeded
     */
    static bool parseIPAddress(const String& ipStr, IPAddress& ip) {
        if (ipStr.length() == 0) {
            return false;
        }
        
        // Simple validation - count dots
        int dotCount = 0;
        for (unsigned int i = 0; i < ipStr.length(); i++) {
            if (ipStr.charAt(i) == '.') {
                dotCount++;
            } else if (!isDigit(ipStr.charAt(i))) {
                return false; // Invalid character
            }
        }
        
        if (dotCount != 3) {
            return false; // Must have exactly 3 dots
        }
        
        // Try to parse using Arduino's fromString method
        if (ip.fromString(ipStr)) {
            return isValidIPAddress(ip);
        }
        
        return false;
    }

    /**
     * @brief Convert IP address to string
     * @param ip IPAddress to convert
     * @return String representation of IP address
     */
    static String ipToString(const IPAddress& ip) {
        return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
    }

    /**
     * @brief Generate broadcast address from IP and subnet mask
     * @param ip Network IP address
     * @param subnet Subnet mask
     * @return Broadcast address
     */
    static IPAddress calculateBroadcastAddress(const IPAddress& ip, const IPAddress& subnet) {
        uint32_t ipAddr = (uint32_t)ip;
        uint32_t subnetAddr = (uint32_t)subnet;
        uint32_t broadcastAddr = ipAddr | (~subnetAddr);
        
        return IPAddress(broadcastAddr);
    }

    /**
     * @brief Calculate network address from IP and subnet mask
     * @param ip IP address
     * @param subnet Subnet mask
     * @return Network address
     */
    static IPAddress calculateNetworkAddress(const IPAddress& ip, const IPAddress& subnet) {
        uint32_t ipAddr = (uint32_t)ip;
        uint32_t subnetAddr = (uint32_t)subnet;
        uint32_t networkAddr = ipAddr & subnetAddr;
        
        return IPAddress(networkAddr);
    }

    /**
     * @brief Check if two IP addresses are in the same subnet
     * @param ip1 First IP address
     * @param ip2 Second IP address
     * @param subnet Subnet mask
     * @return true if both IPs are in same subnet
     */
    static bool areInSameSubnet(const IPAddress& ip1, const IPAddress& ip2, const IPAddress& subnet) {
        IPAddress network1 = calculateNetworkAddress(ip1, subnet);
        IPAddress network2 = calculateNetworkAddress(ip2, subnet);
        
        return network1 == network2;
    }

    /**
     * @brief Validate MAC address format
     * @param mac MAC address bytes (6 bytes)
     * @return true if MAC address is valid (not all zeros or all 0xFF)
     */
    static bool isValidMACAddress(const byte* mac) {
        if (!mac) {
            return false;
        }
        
        // Check for all zeros
        bool allZeros = true;
        bool allOnes = true;
        
        for (int i = 0; i < 6; i++) {
            if (mac[i] != 0x00) {
                allZeros = false;
            }
            if (mac[i] != 0xFF) {
                allOnes = false;
            }
        }
        
        return !allZeros && !allOnes;
    }

    /**
     * @brief Format MAC address as string
     * @param mac MAC address bytes (6 bytes)
     * @return MAC address string (e.g., "AA:BB:CC:DD:EE:FF")
     */
    static String macToString(const byte* mac) {
        if (!mac) {
            return "00:00:00:00:00:00";
        }
        
        String result = "";
        for (int i = 0; i < 6; i++) {
            if (i > 0) {
                result += ":";
            }
            if (mac[i] < 0x10) {
                result += "0";
            }
            result += String(mac[i], HEX);
        }
        result.toUpperCase();
        return result;
    }

    /**
     * @brief Check if port number is valid
     * @param port Port number to validate
     * @return true if port is in valid range (1-65535)
     */
    static bool isValidPort(uint16_t port) {
        return port > 0 && port <= 65535;
    }

    /**
     * @brief Check if port is in well-known range
     * @param port Port number to check
     * @return true if port is well-known (1-1023)
     */
    static bool isWellKnownPort(uint16_t port) {
        return port >= 1 && port <= 1023;
    }

    /**
     * @brief Check if port is in registered range
     * @param port Port number to check
     * @return true if port is registered (1024-49151)
     */
    static bool isRegisteredPort(uint16_t port) {
        return port >= 1024 && port <= 49151;
    }

    /**
     * @brief Check if port is in dynamic/private range
     * @param port Port number to check
     * @return true if port is dynamic (49152-65535)
     */
    static bool isDynamicPort(uint16_t port) {
        return port >= 49152 && port <= 65535;
    }

    /**
     * @brief Simple network connectivity check
     * @param client EthernetClient instance
     * @param host Hostname or IP to connect to
     * @param port Port to connect to
     * @param timeoutMs Connection timeout in milliseconds
     * @return true if connection successful
     */
    static bool testConnectivity(EthernetClient& client, const char* host, uint16_t port, uint32_t timeoutMs = 5000) {
        if (!host || !isValidPort(port)) {
            return false;
        }
        
        uint32_t startTime = millis();
        
        // Attempt connection
        int result = client.connect(host, port);
        if (result == 1) {
            client.stop();
            return true;
        }
        
        // Wait for connection or timeout
        while (client.connected() == 0 && (millis() - startTime) < timeoutMs) {
            delay(10);
        }
        
        bool connected = client.connected();
        if (connected) {
            client.stop();
        }
        
        return connected;
    }

    /**
     * @brief Check if Ethernet hardware is responding
     * @return true if W5500 hardware is responsive
     */
    static bool isEthernetHardwareResponsive() {
        // Try to read a register from W5500
        // This is a basic hardware check
        return Ethernet.hardwareStatus() != EthernetNoHardware;
    }

    /**
     * @brief Get Ethernet hardware status string
     * @return Human readable hardware status
     */
    static String getEthernetHardwareStatus() {
        switch (Ethernet.hardwareStatus()) {
            case EthernetNoHardware:
                return "No Hardware";
            case EthernetW5100:
                return "W5100";
            case EthernetW5200:
                return "W5200";
            case EthernetW5500:
                return "W5500";
            default:
                return "Unknown";
        }
    }

    /**
     * @brief Get Ethernet link status string
     * @return Human readable link status
     */
    static String getEthernetLinkStatus() {
        switch (Ethernet.linkStatus()) {
            case Unknown:
                return "Unknown";
            case LinkON:
                return "Link ON";
            case LinkOFF:
                return "Link OFF";
            default:
                return "Invalid";
        }
    }

    /**
     * @brief Calculate backoff time for exponential backoff
     * @param attempt Attempt number (0-based)
     * @param baseDelayMs Base delay in milliseconds
     * @param maxDelayMs Maximum delay in milliseconds
     * @return Calculated backoff time in milliseconds
     */
    static uint32_t calculateExponentialBackoff(uint8_t attempt, uint32_t baseDelayMs = 1000, uint32_t maxDelayMs = 60000) {
        if (attempt == 0) {
            return baseDelayMs;
        }
        
        // Calculate 2^attempt * baseDelayMs, with overflow protection
        uint32_t backoff = baseDelayMs;
        for (uint8_t i = 0; i < attempt && backoff <= (maxDelayMs / 2); i++) {
            backoff *= 2;
        }
        
        return (backoff > maxDelayMs) ? maxDelayMs : backoff;
    }

    /**
     * @brief Safe UDP packet sending with error checking
     * @param udp UDP instance
     * @param ip Destination IP
     * @param port Destination port
     * @param data Data to send
     * @param length Data length
     * @return Number of bytes sent, or 0 on error
     */
    static size_t sendUDPPacket(EthernetUDP& udp, const IPAddress& ip, uint16_t port, const uint8_t* data, size_t length) {
        if (!isValidIPAddress(ip) || !isValidPort(port) || !data || length == 0) {
            return 0;
        }
        
        int result = udp.beginPacket(ip, port);
        if (result != 1) {
            return 0;
        }
        
        size_t written = udp.write(data, length);
        
        int endResult = udp.endPacket();
        if (endResult != 1) {
            return 0;
        }
        
        return written;
    }

    /**
     * @brief Safe UDP packet receiving with bounds checking
     * @param udp UDP instance
     * @param buffer Buffer to receive data
     * @param bufferSize Size of buffer
     * @param remoteIP Output parameter for sender IP
     * @param remotePort Output parameter for sender port
     * @return Number of bytes received, or 0 if no data
     */
    static size_t receiveUDPPacket(EthernetUDP& udp, uint8_t* buffer, size_t bufferSize, IPAddress& remoteIP, uint16_t& remotePort) {
        if (!buffer || bufferSize == 0) {
            return 0;
        }
        
        int packetSize = udp.parsePacket();
        if (packetSize == 0) {
            return 0;
        }
        
        // Get sender information
        remoteIP = udp.remoteIP();
        remotePort = udp.remotePort();
        
        // Read data with bounds checking
        size_t readSize = (packetSize < (int)bufferSize) ? packetSize : bufferSize;
        return udp.read(buffer, readSize);
    }

    /**
     * @brief Generate unique identifier based on MAC address
     * @param mac MAC address bytes
     * @return 32-bit unique identifier
     */
    static uint32_t generateUniqueId(const byte* mac) {
        if (!mac) {
            return random(0xFFFFFFFF);
        }
        
        uint32_t id = 0;
        for (int i = 0; i < 6; i++) {
            id = (id << 4) ^ mac[i];
        }
        
        // Ensure non-zero result
        return (id == 0) ? 0x12345678 : id;
    }
};

#endif // NETWORK_UTILS_H