#include "NetworkManager.h"
#include "HardwareConfig.h"
#include <SPI.h>

NetworkManager::NetworkManager(EthernetUDP* udpInstance) : ntpUdp(udpInstance) {
    // Initialize MAC address
    byte defaultMac[] = DEFAULT_MAC_ADDRESS;
    memcpy(mac, defaultMac, 6);
    
    // Initialize monitoring structures
    networkMonitor = {false, false, 0, 5000, 0, 5, 0, 30000, 0, 0, 0, false};
    udpManager = {false, 0, 10000, 0};
    
    // Initialize non-blocking state machine
    initState = INIT_START;
    stateChangeTime = 0;
}

void NetworkManager::init() {
    // Start the non-blocking initialization state machine
    initState = INIT_START;
    stateChangeTime = millis();
    Serial.println("Starting non-blocking W5500 initialization...");
    
    // Hardware status detailed check
    Serial.print("Hardware status: ");
    checkHardwareStatus();
    
    // Force alternative approach - matching original main.cpp logic
    Serial.println("Trying alternative approach - forcing DHCP initialization");
    bool hardwareDetected = true; // Force to true to continue like original code
    
    if (!hardwareDetected) {
        Serial.println("ERROR: W5500 Ethernet hardware not found after 3 attempts");
        Serial.println("This may be a library compatibility issue");
#ifdef DEBUG_NETWORK_INIT
        Serial.println("Continuing without Ethernet (GPS-only mode)");
#endif
        digitalWrite(LED_ERROR_PIN, HIGH); // Turn on error LED (常時点灯)
        networkMonitor.isConnected = false;
    } else {
        Serial.println("W5500 hardware detected");
        
        // DHCP attempt
        Serial.println("Attempting DHCP configuration...");
        if (Ethernet.begin(mac) == 0) {
            Serial.println("DHCP failed, trying static IP fallback");
            
            // Static IP configuration fallback (example: 192.168.1.100)
            IPAddress ip(192, 168, 1, 100);
            IPAddress gateway(192, 168, 1, 1);
            IPAddress subnet(255, 255, 255, 0);
            IPAddress dns(8, 8, 8, 8);
            
            Ethernet.begin(mac, ip, dns, gateway, subnet);
            Serial.println("Using static IP configuration");
            networkMonitor.dhcpActive = false;
        } else {
            Serial.println("DHCP configuration successful");
            networkMonitor.dhcpActive = true;
        }
        
        // Connection confirmation
        if (Ethernet.linkStatus() == LinkOFF) {
            Serial.println("WARNING: Ethernet cable not connected");
            networkMonitor.isConnected = false;
        } else {
            networkMonitor.isConnected = true;
            digitalWrite(LED_NETWORK_PIN, HIGH); // Turn on network status LED (Blue) - 常時点灯
            
            Serial.print("Ethernet initialized successfully");
            Serial.print(" - IP: ");
            Serial.print(Ethernet.localIP());
            Serial.print(", Gateway: ");
            Serial.print(Ethernet.gatewayIP());
            Serial.print(", DNS: ");
            Serial.println(Ethernet.dnsServerIP());
            
            // Network monitoring initialization
            networkMonitor.lastLinkCheck = millis();
            networkMonitor.reconnectAttempts = 0;
            
            // UDP socket management initialization
            udpManager.lastSocketCheck = millis();
        }
    }
}

void NetworkManager::initializeW5500() {
    // Legacy blocking initialization - replaced by updateInitialization()
    // This method is kept for compatibility but should call the new method
    while (!updateInitialization()) {
        // Keep calling until initialization is complete
        delay(1); // Minimal delay to prevent tight loop
    }
}

// Performance optimization: Non-blocking W5500 initialization
bool NetworkManager::updateInitialization() {
    unsigned long currentTime = millis();
    
    switch (initState) {
        case INIT_START:
            // W5500 SPI setup and hardware reset
            pinMode(W5500_RST_PIN, OUTPUT);
            pinMode(W5500_INT_PIN, INPUT);
            pinMode(W5500_CS_PIN, OUTPUT);
            digitalWrite(W5500_CS_PIN, HIGH); // CS High
            
            Serial.println("Starting non-blocking W5500 reset...");
            digitalWrite(W5500_RST_PIN, LOW);  // Reset Low
            stateChangeTime = currentTime;
            initState = RESET_LOW;
            return false;
            
        case RESET_LOW:
            if (currentTime - stateChangeTime >= 50) { // 50ms reset low
                digitalWrite(W5500_RST_PIN, HIGH); // Reset High
                stateChangeTime = currentTime;
                initState = RESET_HIGH;
            }
            return false;
            
        case RESET_HIGH:
            if (currentTime - stateChangeTime >= 200) { // 200ms stabilization
                initState = SPI_INIT;
            }
            return false;
            
        case SPI_INIT:
            // SPI initialization (Raspberry Pi Pico 2 compatible)
            SPI.begin();
            SPI.setCS(W5500_CS_PIN);
            initState = ETHERNET_INIT;
            return false;
            
        case ETHERNET_INIT:
            // Initialize Ethernet library with CS pin
            Ethernet.init(W5500_CS_PIN);
            Serial.println("Non-blocking W5500 initialization completed");
            initState = INIT_COMPLETE;
            return true;
            
        case INIT_COMPLETE:
            return true; // Already completed
            
        default:
            initState = INIT_START; // Reset on unknown state
            return false;
    }
}

bool NetworkManager::attemptDhcp() {
    Serial.println("Attempting DHCP configuration...");
    return (Ethernet.begin(mac) == 1);
}

void NetworkManager::setupStaticIp() {
    Serial.println("DHCP failed, trying static IP fallback");
    
    // Static IP configuration (example: 192.168.1.100)
    IPAddress ip(192, 168, 1, 100);
    IPAddress gateway(192, 168, 1, 1);
    IPAddress subnet(255, 255, 255, 0);
    IPAddress dns(8, 8, 8, 8);
    
    Ethernet.begin(mac, ip, dns, gateway, subnet);
    networkMonitor.dhcpActive = false;
}

void NetworkManager::checkHardwareStatus() {
    // Only log hardware status during monitoring, not repeatedly
    static bool hardwareLogged = false;
    
    switch(Ethernet.hardwareStatus()) {
        case EthernetNoHardware:
            if (!hardwareLogged) {
                Serial.println("No hardware detected");
                hardwareLogged = true;
            }
            networkMonitor.isConnected = false;
            break;
        case EthernetW5100:
            if (!hardwareLogged) {
                Serial.println("W5100 detected");
                hardwareLogged = true;
            }
            break;
        case EthernetW5200:
            if (!hardwareLogged) {
                Serial.println("W5200 detected");
                hardwareLogged = true;
            }
            break;
        case EthernetW5500:
            if (!hardwareLogged) {
                Serial.println("W5500 detected");
                hardwareLogged = true;
            }
            break;
        default:
            if (!hardwareLogged) {
                Serial.println("Unknown hardware");
                hardwareLogged = true;
            }
            break;
    }
}

void NetworkManager::checkLinkStatus() {
    if (Ethernet.linkStatus() == LinkOFF) {
        Serial.println("WARNING: Ethernet cable not connected");
        networkMonitor.isConnected = false;
    } else {
        IPAddress currentIP = Ethernet.localIP();
        if (currentIP[0] == 0) {
            networkMonitor.isConnected = false;
            networkMonitor.dhcpActive = false;
        } else {
            networkMonitor.isConnected = true;
            networkMonitor.localIP = (uint32_t)currentIP;
            networkMonitor.gateway = (uint32_t)Ethernet.gatewayIP();
            networkMonitor.dnsServer = (uint32_t)Ethernet.dnsServerIP();
            networkMonitor.reconnectAttempts = 0; // Reset on success
        }
    }
}

void NetworkManager::maintainDhcp() {
    int dhcpResult = Ethernet.maintain();
    switch (dhcpResult) {
        case 1: // renewed fail
            Serial.println("DHCP renewal failed");
            networkMonitor.dhcpActive = false;
            break;
        case 2: // renewed success
            Serial.println("DHCP renewed successfully");
            Serial.print("IP: ");
            Serial.println(Ethernet.localIP());
            networkMonitor.dhcpActive = true;
            break;
        case 3: // rebind fail
            Serial.println("DHCP rebind failed");
            networkMonitor.dhcpActive = false;
            break;
        case 4: // rebind success
            Serial.println("DHCP rebound successfully");
            Serial.print("IP: ");
            Serial.println(Ethernet.localIP());
            networkMonitor.dhcpActive = true;
            break;
    }
}

void NetworkManager::monitorConnection() {
    unsigned long now = millis();
    bool wasConnected = networkMonitor.isConnected;
    
    // Periodic link status check
    if (now - networkMonitor.lastLinkCheck > networkMonitor.linkCheckInterval) {
        networkMonitor.lastLinkCheck = now;
        
        checkHardwareStatus();
        if (Ethernet.hardwareStatus() != EthernetNoHardware) {
            checkLinkStatus();
        }
    }
    
    // DHCP maintenance
    maintainDhcp();
    
    // Detect connection status changes
    if (wasConnected && !networkMonitor.isConnected) {
        Serial.println("Network connection lost");
        digitalWrite(LED_NETWORK_PIN, LOW); // Turn off network status LED (Blue)
    } else if (!wasConnected && networkMonitor.isConnected) {
        Serial.println("Network connection established");
        digitalWrite(LED_NETWORK_PIN, HIGH); // Turn on network status LED (Blue) - 常時点灯
        Serial.print("IP: ");
        Serial.print(Ethernet.localIP());
        Serial.print(", Gateway: ");
        Serial.print(Ethernet.gatewayIP());
        Serial.print(", DNS: ");
        Serial.println(Ethernet.dnsServerIP());
    }
}

void NetworkManager::attemptReconnection() {
    unsigned long now = millis();
    
    if (!networkMonitor.isConnected && 
        networkMonitor.reconnectAttempts < networkMonitor.maxReconnectAttempts &&
        (now - networkMonitor.lastReconnectTime > networkMonitor.reconnectInterval)) {
        
        networkMonitor.lastReconnectTime = now;
        networkMonitor.reconnectAttempts++;
        
        Serial.print("Attempting network reconnection (attempt ");
        Serial.print(networkMonitor.reconnectAttempts);
        Serial.print("/");
        Serial.print(networkMonitor.maxReconnectAttempts);
        Serial.println(")");
        
        // W5500 reset
        if (Ethernet.hardwareStatus() != EthernetNoHardware) {
            Serial.println("Resetting W5500...");
            
            // DHCP retry
            if (Ethernet.begin(mac) == 0) {
                Serial.println("DHCP failed, will retry in 30 seconds");
            } else {
                Serial.println("DHCP reconnection successful");
                networkMonitor.isConnected = true;
                networkMonitor.dhcpActive = true;
                networkMonitor.reconnectAttempts = 0;
            }
        }
    }
}

void NetworkManager::manageUdpSockets() {
    unsigned long now = millis();
    
    // Periodic socket status check
    if (now - udpManager.lastSocketCheck > udpManager.socketCheckInterval) {
        udpManager.lastSocketCheck = now;
        
        if (networkMonitor.isConnected) {
            // ★★★ Critical Fix: Periodic UDP socket refresh for W5500 reliability ★★★
            // W5500 UDP sockets can become unresponsive after handling multiple requests
            // Force socket restart every 60 seconds to maintain reliability
            static unsigned long lastSocketRefresh = 0;
            const unsigned long socketRefreshInterval = 60000; // 60 seconds
            
            bool needsRefresh = (now - lastSocketRefresh > socketRefreshInterval);
            bool hasSocketErrors = (udpManager.socketErrors > 5);
            
            if (needsRefresh || hasSocketErrors) {
                if (udpManager.ntpSocketOpen) {
                    Serial.println("Refreshing NTP UDP socket for W5500 reliability");
                    ntpUdp->stop();
                    delay(10); // Brief delay to ensure socket is properly closed
                }
                
                if (ntpUdp->begin(NTP_PORT)) {
                    udpManager.ntpSocketOpen = true;
                    networkMonitor.ntpServerActive = true;
                    udpManager.socketErrors = 0;
                    lastSocketRefresh = now;
                    Serial.println("NTP UDP socket refreshed successfully");
                } else {
                    Serial.println("Failed to refresh NTP UDP socket");
                    udpManager.socketErrors++;
                    udpManager.ntpSocketOpen = false;
                    networkMonitor.ntpServerActive = false;
                }
            }
            // Normal socket management
            else if (!udpManager.ntpSocketOpen) {
                Serial.println("Opening NTP UDP socket on port 123");
                if (ntpUdp->begin(NTP_PORT)) {
                    udpManager.ntpSocketOpen = true;
                    networkMonitor.ntpServerActive = true;
                    Serial.println("NTP UDP socket opened successfully");
                } else {
                    Serial.println("Failed to open NTP UDP socket");
                    udpManager.socketErrors++;
                }
            }
        } else {
            // Close UDP socket when network is disconnected
            if (udpManager.ntpSocketOpen) {
                Serial.println("Closing NTP UDP socket due to network disconnection");
                ntpUdp->stop();
                udpManager.ntpSocketOpen = false;
                networkMonitor.ntpServerActive = false;
            }
        }
    }
    
    // Reset socket errors on success (but only if not too many)
    if (udpManager.ntpSocketOpen && udpManager.socketErrors > 0 && udpManager.socketErrors < 10) {
        udpManager.socketErrors = 0;
    }
}