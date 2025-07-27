#include "NetworkManager.h"
#include "HardwareConfig.h"
#include "LoggingService.h"
#include "ConfigManager.h"
#include <SPI.h>

NetworkManager::NetworkManager(EthernetUDP* udpInstance) : ntpUdp(udpInstance), loggingService(nullptr), configManager(nullptr) {
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
    if (loggingService) {
        loggingService->info("NETWORK", "Starting W5500 initialization sequence...");
    }
    
    // CRITICAL FIX: Complete W5500 initialization BEFORE any Ethernet operations
    if (loggingService) {
        loggingService->info("NETWORK", "Phase 1: W5500 hardware reset and SPI initialization");
    }
    
    // Complete the full W5500 initialization sequence synchronously
    initializeW5500();
    
    // Wait for full initialization to complete
    delay(1000);
    
    // Hardware detection after proper initialization
    if (loggingService) {
        loggingService->info("NETWORK", "Phase 2: Hardware detection after initialization");
    }
    checkHardwareStatus();
    
    bool hardwareDetected = (Ethernet.hardwareStatus() != EthernetNoHardware);
    
    if (!hardwareDetected) {
        if (loggingService) {
            loggingService->error("NETWORK", "W5500 hardware not detected after proper initialization");
            loggingService->error("NETWORK", "Check SPI connections and W5500 module power");
        }
        digitalWrite(LED_ERROR_PIN, HIGH); // Turn on error LED
        networkMonitor.isConnected = false;
        return; // Exit early if hardware not detected
    }
    
    if (loggingService) {
        loggingService->info("NETWORK", "W5500 hardware successfully detected");
        loggingService->info("NETWORK", "Phase 3: Network configuration (DHCP/Static IP)");
    }
    
    // CRITICAL FIX: Proper DHCP sequence with W5500-specific optimizations
    int dhcpResult = 0;
    
    for (int attempt = 1; attempt <= 3; attempt++) {
        if (loggingService) {
            loggingService->infof("NETWORK", "DHCP attempt %d/3 (W5500 optimized)", attempt);
        }
        
        // W5500-specific DHCP approach: Use default timeout first
        dhcpResult = Ethernet.begin(mac);
        
        if (dhcpResult == 1) {
            if (loggingService) {
                loggingService->info("NETWORK", "DHCP configuration successful");
            }
            break;
        } else {
            if (loggingService) {
                loggingService->warningf("NETWORK", "DHCP attempt %d failed (result: %d)", attempt, dhcpResult);
            }
            
            if (attempt < 3) {
                // Reset W5500 between attempts
                if (loggingService) {
                    loggingService->info("NETWORK", "Resetting W5500 before retry...");
                }
                digitalWrite(W5500_RST_PIN, LOW);
                delay(100);
                digitalWrite(W5500_RST_PIN, HIGH);
                delay(500);
                Ethernet.init(W5500_CS_PIN); // Reinitialize
                delay(1000);
            }
        }
    }
        
    if (dhcpResult == 0) {
        if (loggingService) {
            loggingService->warning("NETWORK", "All DHCP attempts failed - trying static IP");
        }
        
        // CRITICAL FIX: Improved static IP fallback with W5500 reset
        if (loggingService) {
            loggingService->info("NETWORK", "Performing W5500 reset before static IP configuration");
        }
        
        // Reset W5500 completely before static IP
        digitalWrite(W5500_RST_PIN, LOW);
        delay(200);
        digitalWrite(W5500_RST_PIN, HIGH);
        delay(1000);
        Ethernet.init(W5500_CS_PIN);
        delay(500);
        
        // Static IP configuration
        IPAddress ip, gateway, subnet, dns;
        
        if (configManager && configManager->getIpAddress() != 0) {
            // Use configuration from ConfigManager
            uint32_t configIp = configManager->getIpAddress();
            uint32_t configGateway = configManager->getGateway();
            uint32_t configNetmask = configManager->getNetmask();
            
            ip = IPAddress((configIp >> 24) & 0xFF, (configIp >> 16) & 0xFF, 
                          (configIp >> 8) & 0xFF, configIp & 0xFF);
            gateway = IPAddress((configGateway >> 24) & 0xFF, (configGateway >> 16) & 0xFF,
                               (configGateway >> 8) & 0xFF, configGateway & 0xFF);
            subnet = IPAddress((configNetmask >> 24) & 0xFF, (configNetmask >> 16) & 0xFF,
                              (configNetmask >> 8) & 0xFF, configNetmask & 0xFF);
            dns = IPAddress(8, 8, 8, 8);
            
            if (loggingService) {
                loggingService->infof("NETWORK", "Using static IP from config: %d.%d.%d.%d", 
                                    ip[0], ip[1], ip[2], ip[3]);
            }
        } else {
            // Use network-appropriate fallback
            ip = IPAddress(192, 168, 1, 100);
            gateway = IPAddress(192, 168, 1, 1);
            subnet = IPAddress(255, 255, 255, 0);
            dns = IPAddress(8, 8, 8, 8);
            
            if (loggingService) {
                loggingService->warning("NETWORK", "Using fallback static IP: 192.168.1.100");
            }
        }
        
        // Apply static IP configuration with error checking
        Ethernet.begin(mac, ip, dns, gateway, subnet);
        delay(2000); // Extended delay for W5500 static IP application
        
        // Verify static IP configuration
        IPAddress assignedIP = Ethernet.localIP();
        if (assignedIP == IPAddress(0, 0, 0, 0)) {
            if (loggingService) {
                loggingService->error("NETWORK", "Static IP configuration failed - W5500 not responding");
                loggingService->error("NETWORK", "Check hardware connections and power supply");
            }
            networkMonitor.isConnected = false;
        } else {
            if (loggingService) {
                loggingService->infof("NETWORK", "Static IP configured successfully: %d.%d.%d.%d", 
                                    assignedIP[0], assignedIP[1], assignedIP[2], assignedIP[3]);
            }
            networkMonitor.isConnected = true;
        }
        networkMonitor.dhcpActive = false;
    } else {
        if (loggingService) {
            loggingService->info("NETWORK", "DHCP configuration successful");
        }
        networkMonitor.dhcpActive = true;
        networkMonitor.isConnected = true;
    }
        
    // Final connection verification and status reporting
    if (loggingService) {
        loggingService->info("NETWORK", "Phase 4: Final connection verification");
    }
    
    // Check physical link status
    if (Ethernet.linkStatus() == LinkOFF) {
        if (loggingService) {
            loggingService->warning("NETWORK", "No physical Ethernet link detected");
        }
        networkMonitor.isConnected = false;
        digitalWrite(LED_NETWORK_PIN, LOW);
    } else {
        // Verify we have a valid IP address
        IPAddress finalIP = Ethernet.localIP();
        if (finalIP == IPAddress(0, 0, 0, 0)) {
            if (loggingService) {
                loggingService->error("NETWORK", "Physical link OK but no IP address assigned");
            }
            networkMonitor.isConnected = false;
            digitalWrite(LED_NETWORK_PIN, LOW);
        } else {
            // Success - we have both link and IP
            networkMonitor.isConnected = true;
            digitalWrite(LED_NETWORK_PIN, HIGH);
            
            if (loggingService) {
                loggingService->infof("NETWORK", "Network initialization completed successfully");
                loggingService->infof("NETWORK", "IP: %d.%d.%d.%d, Gateway: %d.%d.%d.%d, DNS: %d.%d.%d.%d",
                                    finalIP[0], finalIP[1], finalIP[2], finalIP[3],
                                    Ethernet.gatewayIP()[0], Ethernet.gatewayIP()[1], 
                                    Ethernet.gatewayIP()[2], Ethernet.gatewayIP()[3],
                                    Ethernet.dnsServerIP()[0], Ethernet.dnsServerIP()[1],
                                    Ethernet.dnsServerIP()[2], Ethernet.dnsServerIP()[3]);
            }
            
            // Initialize network monitoring
            networkMonitor.lastLinkCheck = millis();
            networkMonitor.reconnectAttempts = 0;
            udpManager.lastSocketCheck = millis();
        }
    }
}

void NetworkManager::initializeW5500() {
    if (loggingService) {
        loggingService->info("NETWORK", "W5500 synchronous initialization starting...");
    }
    
    // W5500 GPIO pin setup
    pinMode(W5500_RST_PIN, OUTPUT);
    pinMode(W5500_INT_PIN, INPUT);
    pinMode(W5500_CS_PIN, OUTPUT);
    digitalWrite(W5500_CS_PIN, HIGH); // CS High (inactive)
    
    if (loggingService) {
        loggingService->info("NETWORK", "W5500 GPIO pins configured");
    }
    
    // Hardware reset sequence - extended timing for reliability
    digitalWrite(W5500_RST_PIN, LOW);  // Assert reset
    delay(200);  // Extended reset pulse (was 100ms)
    digitalWrite(W5500_RST_PIN, HIGH); // Release reset
    delay(1000); // Extended stabilization time (was 500ms)
    
    if (loggingService) {
        loggingService->info("NETWORK", "W5500 hardware reset completed (200ms low, 1000ms stabilization)");
    }
    
    // SPI initialization with explicit settings
    SPI.begin();
    SPI.setCS(W5500_CS_PIN);
    // Set SPI parameters for W5500 compatibility
    SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0)); // 10MHz, MSB first, Mode 0
    SPI.endTransaction();
    
    if (loggingService) {
        loggingService->info("NETWORK", "SPI interface initialized (10MHz, Mode 0)");
    }
    
    // Initialize Ethernet library with CS pin
    Ethernet.init(W5500_CS_PIN);
    delay(500); // Additional delay for Ethernet library initialization
    
    if (loggingService) {
        loggingService->info("NETWORK", "Ethernet library initialization completed");
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
            
            if (loggingService) {
                loggingService->info("NETWORK", "Starting non-blocking W5500 reset...");
            }
            digitalWrite(W5500_RST_PIN, LOW);  // Reset Low
            stateChangeTime = currentTime;
            initState = RESET_LOW;
            return false;
            
        case RESET_LOW:
            if (currentTime - stateChangeTime >= 100) { // Extended 100ms reset low
                digitalWrite(W5500_RST_PIN, HIGH); // Reset High
                stateChangeTime = currentTime;
                initState = RESET_HIGH;
                if (loggingService) {
                    loggingService->info("NETWORK", "W5500 hardware reset completed - waiting for stabilization");
                }
            }
            return false;
            
        case RESET_HIGH:
            if (currentTime - stateChangeTime >= 500) { // Extended 500ms stabilization
                initState = SPI_INIT;
                if (loggingService) {
                    loggingService->info("NETWORK", "W5500 stabilization completed - initializing SPI");
                }
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
            if (loggingService) {
                loggingService->info("NETWORK", "Non-blocking W5500 initialization completed");
            }
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
    if (loggingService) {
        loggingService->info("NETWORK", "Attempting DHCP configuration...");
    }
    return (Ethernet.begin(mac) == 1);
}

void NetworkManager::setupStaticIp() {
    if (loggingService) {
        loggingService->warning("NETWORK", "DHCP failed, trying static IP fallback");
    }
    
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
                if (loggingService) {
                    loggingService->warning("NETWORK", "No hardware detected");
                }
                hardwareLogged = true;
            }
            networkMonitor.isConnected = false;
            break;
        case EthernetW5100:
            if (!hardwareLogged) {
                if (loggingService) {
                    loggingService->info("NETWORK", "W5100 detected");
                }
                hardwareLogged = true;
            }
            break;
        case EthernetW5200:
            if (!hardwareLogged) {
                if (loggingService) {
                    loggingService->info("NETWORK", "W5200 detected");
                }
                hardwareLogged = true;
            }
            break;
        case EthernetW5500:
            if (!hardwareLogged) {
                if (loggingService) {
                    loggingService->info("NETWORK", "W5500 detected");
                }
                hardwareLogged = true;
            }
            break;
        default:
            if (!hardwareLogged) {
                if (loggingService) {
                    loggingService->warning("NETWORK", "Unknown hardware");
                }
                hardwareLogged = true;
            }
            break;
    }
}

void NetworkManager::checkLinkStatus() {
    if (Ethernet.linkStatus() == LinkOFF) {
        if (loggingService) {
            loggingService->warning("NETWORK", "Ethernet cable not connected");
        }
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
            if (loggingService) {
                loggingService->warning("NETWORK", "DHCP renewal failed - attempting fallback");
            }
            networkMonitor.dhcpActive = false;
            digitalWrite(LED_NETWORK_PIN, LOW); // Turn off network LED
            break;
        case 2: // renewed success
            if (loggingService) {
                IPAddress currentIP = Ethernet.localIP();
                loggingService->infof("NETWORK", "DHCP renewed successfully - IP: %d.%d.%d.%d",
                                    currentIP[0], currentIP[1], currentIP[2], currentIP[3]);
            }
            networkMonitor.dhcpActive = true;
            networkMonitor.isConnected = true;
            digitalWrite(LED_NETWORK_PIN, HIGH); // Turn on network LED
            break;
        case 3: // rebind fail
            if (loggingService) {
                loggingService->warning("NETWORK", "DHCP rebind failed - network connectivity lost");
            }
            networkMonitor.dhcpActive = false;
            networkMonitor.isConnected = false;
            digitalWrite(LED_NETWORK_PIN, LOW); // Turn off network LED
            break;
        case 4: // rebind success
            if (loggingService) {
                IPAddress currentIP = Ethernet.localIP();
                loggingService->infof("NETWORK", "DHCP rebound successfully - IP: %d.%d.%d.%d",
                                    currentIP[0], currentIP[1], currentIP[2], currentIP[3]);
            }
            networkMonitor.dhcpActive = true;
            networkMonitor.isConnected = true;
            digitalWrite(LED_NETWORK_PIN, HIGH); // Turn on network LED
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
        
        // DHCP maintenance only every 5 seconds to reduce spam
        static unsigned long lastDhcpMaintain = 0;
        if (now - lastDhcpMaintain > 5000) {
            lastDhcpMaintain = now;
            maintainDhcp();
        }
    }
    
    // Detect connection status changes
    if (wasConnected && !networkMonitor.isConnected) {
        if (loggingService) {
            loggingService->warning("NETWORK", "Network connection lost - LED status updated");
        }
        digitalWrite(LED_NETWORK_PIN, LOW); // Turn off network status LED (Blue)
    } else if (!wasConnected && networkMonitor.isConnected) {
        IPAddress currentIP = Ethernet.localIP();
        if (loggingService) {
            loggingService->infof("NETWORK", "Network connection established - IP: %d.%d.%d.%d, Gateway: %d.%d.%d.%d",
                                currentIP[0], currentIP[1], currentIP[2], currentIP[3],
                                Ethernet.gatewayIP()[0], Ethernet.gatewayIP()[1], 
                                Ethernet.gatewayIP()[2], Ethernet.gatewayIP()[3]);
        }
        digitalWrite(LED_NETWORK_PIN, HIGH); // Turn on network status LED (Blue) - 常時点灯
    }
}

void NetworkManager::attemptReconnection() {
    unsigned long now = millis();
    
    if (!networkMonitor.isConnected && 
        networkMonitor.reconnectAttempts < networkMonitor.maxReconnectAttempts &&
        (now - networkMonitor.lastReconnectTime > networkMonitor.reconnectInterval)) {
        
        networkMonitor.lastReconnectTime = now;
        networkMonitor.reconnectAttempts++;
        
        if (loggingService) {
            loggingService->infof("NETWORK", "Attempting reconnection (attempt %d/%d)",
                                networkMonitor.reconnectAttempts, networkMonitor.maxReconnectAttempts);
        }
        
        // W5500 reset and reconnection
        if (Ethernet.hardwareStatus() != EthernetNoHardware) {
            if (loggingService) {
                loggingService->info("NETWORK", "Resetting W5500 hardware for reconnection");
            }
            
            // Hardware reset sequence
            digitalWrite(W5500_RST_PIN, LOW);
            delay(10);
            digitalWrite(W5500_RST_PIN, HIGH);
            delay(100);
            
            // DHCP retry
            if (Ethernet.begin(mac) == 0) {
                if (loggingService) {
                    loggingService->warning("NETWORK", "DHCP reconnection failed - will retry in 30 seconds");
                }
                digitalWrite(LED_NETWORK_PIN, LOW); // Ensure LED is off
            } else {
                IPAddress reconnectedIP = Ethernet.localIP();
                if (loggingService) {
                    loggingService->infof("NETWORK", "DHCP reconnection successful - IP: %d.%d.%d.%d",
                                        reconnectedIP[0], reconnectedIP[1], reconnectedIP[2], reconnectedIP[3]);
                }
                networkMonitor.isConnected = true;
                networkMonitor.dhcpActive = true;
                networkMonitor.reconnectAttempts = 0;
                digitalWrite(LED_NETWORK_PIN, HIGH); // Turn on LED for successful connection
            }
        } else {
            if (loggingService) {
                loggingService->error("NETWORK", "W5500 hardware not detected during reconnection attempt");
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
                    if (loggingService) {
                        loggingService->info("NETWORK", "Refreshing NTP UDP socket for W5500 reliability");
                    }
                    ntpUdp->stop();
                    delay(10); // Brief delay to ensure socket is properly closed
                }
                
                if (ntpUdp->begin(NTP_PORT)) {
                    udpManager.ntpSocketOpen = true;
                    networkMonitor.ntpServerActive = true;
                    udpManager.socketErrors = 0;
                    lastSocketRefresh = now;
                    if (loggingService) {
                        loggingService->info("NETWORK", "NTP UDP socket refreshed successfully");
                    }
                } else {
                    if (loggingService) {
                        loggingService->warning("NETWORK", "Failed to refresh NTP UDP socket");
                    }
                    udpManager.socketErrors++;
                    udpManager.ntpSocketOpen = false;
                    networkMonitor.ntpServerActive = false;
                }
            }
            // Normal socket management
            else if (!udpManager.ntpSocketOpen) {
                if (loggingService) {
                    loggingService->info("NETWORK", "Opening NTP UDP socket on port 123");
                }
                if (ntpUdp->begin(NTP_PORT)) {
                    udpManager.ntpSocketOpen = true;
                    networkMonitor.ntpServerActive = true;
                    if (loggingService) {
                        loggingService->info("NETWORK", "NTP UDP socket opened successfully");
                    }
                } else {
                    if (loggingService) {
                        loggingService->warning("NETWORK", "Failed to open NTP UDP socket");
                    }
                    udpManager.socketErrors++;
                }
            }
        } else {
            // Close UDP socket when network is disconnected
            if (udpManager.ntpSocketOpen) {
                if (loggingService) {
                    loggingService->info("NETWORK", "Closing NTP UDP socket due to network disconnection");
                }
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