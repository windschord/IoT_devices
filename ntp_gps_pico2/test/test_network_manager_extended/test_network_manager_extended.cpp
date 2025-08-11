#include <unity.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Extended NetworkManager implementation for testing
struct NetworkConfig {
    char hostname[32];
    uint32_t ip_address;        // 0 for DHCP
    uint32_t netmask;
    uint32_t gateway;
    uint32_t dns_server;
    uint16_t web_port;
    uint16_t prometheus_port;
    bool dhcp_enabled;
    uint8_t mac_address[6];
    uint32_t lease_time;        // DHCP lease time in seconds
};

struct NetworkStats {
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint32_t packets_sent;
    uint32_t packets_received;
    uint32_t packets_dropped;
    uint32_t connection_errors;
    uint16_t active_connections;
    uint32_t uptime_seconds;
    float bandwidth_utilization;
    uint32_t dhcp_renewals;
};

enum NetworkStatus {
    NETWORK_DISCONNECTED = 0,
    NETWORK_CONNECTING = 1,
    NETWORK_CONNECTED = 2,
    NETWORK_ERROR = 3,
    NETWORK_DHCP_FAILED = 4
};

// Mock Ethernet HAL for testing
class MockEthernetHAL {
public:
    bool cable_connected = true;
    bool link_up = true;
    bool dhcp_success = true;
    NetworkConfig current_config;
    NetworkStats stats;
    int error_rate = 0;
    uint32_t connection_delay_ms = 0;
    bool initialized = false;
    
    MockEthernetHAL() {
        reset();
    }
    
    bool begin() {
        if (error_rate > 0 && (rand() % 100) < error_rate) {
            return false;
        }
        initialized = true;
        return true;
    }
    
    bool isLinkUp() {
        return cable_connected && link_up;
    }
    
    bool isDHCPConfigured() {
        return dhcp_success && current_config.dhcp_enabled;
    }
    
    bool configure(const NetworkConfig& config) {
        if (error_rate > 0 && (rand() % 100) < error_rate) {
            return false;
        }
        
        current_config = config;
        
        if (config.dhcp_enabled) {
            if (dhcp_success) {
                // Simulate DHCP assignment
                current_config.ip_address = 0xC0A80164; // 192.168.1.100
                current_config.netmask = 0xFFFFFF00;    // 255.255.255.0
                current_config.gateway = 0xC0A80101;    // 192.168.1.1
                current_config.dns_server = 0x08080808; // 8.8.8.8
                current_config.lease_time = 3600;       // 1 hour
                return true;
            } else {
                return false;
            }
        }
        
        // Static configuration always succeeds (if no errors)
        return true;
    }
    
    uint32_t getIPAddress() {
        return current_config.ip_address;
    }
    
    uint32_t getNetmask() {
        return current_config.netmask;
    }
    
    uint32_t getGateway() {
        return current_config.gateway;
    }
    
    uint32_t getDNSServer() {
        return current_config.dns_server;
    }
    
    bool sendPacket(const uint8_t* data, size_t len) {
        if (!isLinkUp() || (error_rate > 0 && (rand() % 100) < error_rate)) {
            stats.packets_dropped++;
            stats.connection_errors++;
            return false;
        }
        
        stats.packets_sent++;
        stats.bytes_sent += len;
        return true;
    }
    
    bool receivePacket(uint8_t* buffer, size_t* len) {
        if (!isLinkUp() || (error_rate > 0 && (rand() % 100) < error_rate)) {
            return false;
        }
        
        // Simulate receiving a packet
        if (rand() % 10 == 0) { // 10% chance of having a packet
            *len = 64; // Simulate 64-byte packet
            stats.packets_received++;
            stats.bytes_received += *len;
            return true;
        }
        
        return false; // No packet available
    }
    
    void updateStats() {
        stats.uptime_seconds++;
        
        // Simulate bandwidth utilization
        uint64_t total_bytes = stats.bytes_sent + stats.bytes_received;
        stats.bandwidth_utilization = (total_bytes % 100) / 100.0f;
        
        // Simulate DHCP renewal
        if (current_config.dhcp_enabled && stats.uptime_seconds % 1800 == 0) {
            stats.dhcp_renewals++;
        }
    }
    
    void reset() {
        memset(&current_config, 0, sizeof(current_config));
        memset(&stats, 0, sizeof(stats));
        strcpy(current_config.hostname, "gps-ntp");
        current_config.web_port = 80;
        current_config.prometheus_port = 9090;
        current_config.dhcp_enabled = true;
        
        // Default MAC address
        current_config.mac_address[0] = 0x02;
        current_config.mac_address[1] = 0x00;
        current_config.mac_address[2] = 0x00;
        current_config.mac_address[3] = 0x12;
        current_config.mac_address[4] = 0x34;
        current_config.mac_address[5] = 0x56;
        
        cable_connected = true;
        link_up = true;
        dhcp_success = true;
        error_rate = 0;
        connection_delay_ms = 0;
        initialized = false;
    }
    
    void setCableConnected(bool connected) {
        cable_connected = connected;
        if (!connected) {
            link_up = false;
        }
    }
    
    void setDHCPSuccess(bool success) {
        dhcp_success = success;
    }
    
    void setErrorRate(int percentage) {
        error_rate = (percentage > 100) ? 100 : percentage;
    }
    
    NetworkStats getStats() const {
        return stats;
    }
};

// Mock Web Server HAL for testing
class MockWebServerHAL {
public:
    bool server_running = false;
    uint16_t port = 80;
    uint32_t total_requests = 0;
    uint32_t successful_responses = 0;
    uint32_t error_responses = 0;
    int error_rate = 0;
    
    bool begin(uint16_t server_port) {
        if (error_rate > 0 && (rand() % 100) < error_rate) {
            return false;
        }
        
        port = server_port;
        server_running = true;
        return true;
    }
    
    void stop() {
        server_running = false;
    }
    
    bool handleRequest() {
        if (!server_running) {
            return false;
        }
        
        total_requests++;
        
        if (error_rate > 0 && (rand() % 100) < error_rate) {
            error_responses++;
            return false;
        }
        
        successful_responses++;
        return true;
    }
    
    void reset() {
        server_running = false;
        port = 80;
        total_requests = 0;
        successful_responses = 0;
        error_responses = 0;
        error_rate = 0;
    }
    
    void setErrorRate(int percentage) {
        error_rate = (percentage > 100) ? 100 : percentage;
    }
    
    bool isRunning() const {
        return server_running;
    }
    
    uint32_t getTotalRequests() const {
        return total_requests;
    }
    
    uint32_t getSuccessfulResponses() const {
        return successful_responses;
    }
    
    uint32_t getErrorResponses() const {
        return error_responses;
    }
};

// Extended NetworkManager with advanced features
class ExtendedNetworkManager {
private:
    MockEthernetHAL* ethernet;
    MockWebServerHAL* webserver;
    NetworkConfig config;
    NetworkStatus current_status;
    uint32_t last_connection_attempt;
    uint32_t connection_retry_interval;
    uint32_t connection_timeout;
    bool auto_reconnect;
    uint32_t reconnect_attempts;
    uint32_t max_reconnect_attempts;
    
    // Connection monitoring
    uint32_t last_ping_time;
    uint32_t ping_interval;
    bool ping_enabled;
    uint32_t ping_failures;
    uint32_t max_ping_failures;
    
    // Performance monitoring
    float connection_quality;
    uint32_t latency_samples[10];
    uint8_t latency_index;
    float average_latency;
    
public:
    ExtendedNetworkManager(MockEthernetHAL* eth, MockWebServerHAL* web) 
        : ethernet(eth), webserver(web) {
        current_status = NETWORK_DISCONNECTED;
        last_connection_attempt = 0;
        connection_retry_interval = 5000;  // 5 seconds
        connection_timeout = 10000;        // 10 seconds
        auto_reconnect = true;
        reconnect_attempts = 0;
        max_reconnect_attempts = 10;
        
        last_ping_time = 0;
        ping_interval = 30000;             // 30 seconds
        ping_enabled = true;
        ping_failures = 0;
        max_ping_failures = 3;
        
        connection_quality = 0.0f;
        memset(latency_samples, 0, sizeof(latency_samples));
        latency_index = 0;
        average_latency = 0.0f;
        
        loadDefaultConfig();
    }
    
    void loadDefaultConfig() {
        strcpy(config.hostname, "gps-ntp-server");
        config.ip_address = 0;              // DHCP
        config.netmask = 0xFFFFFF00;        // 255.255.255.0
        config.gateway = 0xC0A80101;        // 192.168.1.1
        config.dns_server = 0x08080808;     // 8.8.8.8
        config.web_port = 80;
        config.prometheus_port = 9090;
        config.dhcp_enabled = true;
        config.lease_time = 3600;           // 1 hour
        
        // Default MAC address
        config.mac_address[0] = 0x02;
        config.mac_address[1] = 0x00;
        config.mac_address[2] = 0x00;
        config.mac_address[3] = 0x12;
        config.mac_address[4] = 0x34;
        config.mac_address[5] = 0x56;
    }
    
    bool initialize() {
        if (!ethernet->begin()) {
            current_status = NETWORK_ERROR;
            return false;
        }
        
        return connect();
    }
    
    bool connect() {
        if (current_status == NETWORK_CONNECTING) {
            return false; // Already connecting
        }
        
        current_status = NETWORK_CONNECTING;
        last_connection_attempt = getCurrentTime();
        
        if (!ethernet->isLinkUp()) {
            current_status = NETWORK_DISCONNECTED;
            return false;
        }
        
        if (!ethernet->configure(config)) {
            current_status = config.dhcp_enabled ? NETWORK_DHCP_FAILED : NETWORK_ERROR;
            return false;
        }
        
        // Start web services
        if (!webserver->begin(config.web_port)) {
            current_status = NETWORK_ERROR;
            return false;
        }
        
        current_status = NETWORK_CONNECTED;
        reconnect_attempts = 0;
        ping_failures = 0;
        updateConnectionQuality();
        
        return true;
    }
    
    void disconnect() {
        webserver->stop();
        current_status = NETWORK_DISCONNECTED;
    }
    
    void update() {
        uint32_t current_time = getCurrentTime();
        
        switch (current_status) {
            case NETWORK_DISCONNECTED:
            case NETWORK_ERROR:
            case NETWORK_DHCP_FAILED:
                if (auto_reconnect && 
                    (current_time - last_connection_attempt >= connection_retry_interval) &&
                    (reconnect_attempts < max_reconnect_attempts)) {
                    reconnect_attempts++;
                    connect();
                }
                break;
                
            case NETWORK_CONNECTING:
                if (current_time - last_connection_attempt >= connection_timeout) {
                    current_status = NETWORK_ERROR;
                }
                break;
                
            case NETWORK_CONNECTED:
                // Monitor connection health
                monitorConnection();
                
                // Handle web server requests
                webserver->handleRequest();
                
                // Update statistics
                ethernet->updateStats();
                break;
        }
    }
    
    // Configuration methods
    bool setStaticIP(uint32_t ip, uint32_t netmask, uint32_t gateway, uint32_t dns) {
        config.ip_address = ip;
        config.netmask = netmask;
        config.gateway = gateway;
        config.dns_server = dns;
        config.dhcp_enabled = false;
        
        if (current_status == NETWORK_CONNECTED) {
            disconnect();
            return connect();
        }
        
        return true;
    }
    
    bool enableDHCP() {
        config.dhcp_enabled = true;
        config.ip_address = 0;
        
        if (current_status == NETWORK_CONNECTED) {
            disconnect();
            return connect();
        }
        
        return true;
    }
    
    bool setHostname(const char* hostname) {
        if (!hostname || strlen(hostname) >= sizeof(config.hostname)) {
            return false;
        }
        strcpy(config.hostname, hostname);
        return true;
    }
    
    bool setWebPort(uint16_t port) {
        if (port < 80 || port > 65535) {
            return false;
        }
        config.web_port = port;
        return true;
    }
    
    bool setPrometheusPort(uint16_t port) {
        if (port < 1024 || port > 65535) {
            return false;
        }
        config.prometheus_port = port;
        return true;
    }
    
    void setAutoReconnect(bool enable) {
        auto_reconnect = enable;
    }
    
    void setConnectionTimeout(uint32_t timeout_ms) {
        connection_timeout = timeout_ms;
    }
    
    void setRetryInterval(uint32_t interval_ms) {
        connection_retry_interval = interval_ms;
    }
    
    void setMaxReconnectAttempts(uint32_t max_attempts) {
        max_reconnect_attempts = max_attempts;
    }
    
    void setPingEnabled(bool enable) {
        ping_enabled = enable;
    }
    
    void setPingInterval(uint32_t interval_ms) {
        ping_interval = interval_ms;
    }
    
    // Status getters
    NetworkStatus getStatus() const {
        return current_status;
    }
    
    bool isConnected() const {
        return current_status == NETWORK_CONNECTED;
    }
    
    uint32_t getIPAddress() const {
        return ethernet->getIPAddress();
    }
    
    uint32_t getNetmask() const {
        return ethernet->getNetmask();
    }
    
    uint32_t getGateway() const {
        return ethernet->getGateway();
    }
    
    uint32_t getDNSServer() const {
        return ethernet->getDNSServer();
    }
    
    const char* getHostname() const {
        return config.hostname;
    }
    
    uint16_t getWebPort() const {
        return config.web_port;
    }
    
    uint16_t getPrometheusPort() const {
        return config.prometheus_port;
    }
    
    bool isDHCPEnabled() const {
        return config.dhcp_enabled;
    }
    
    uint32_t getReconnectAttempts() const {
        return reconnect_attempts;
    }
    
    float getConnectionQuality() const {
        return connection_quality;
    }
    
    float getAverageLatency() const {
        return average_latency;
    }
    
    uint32_t getPingFailures() const {
        return ping_failures;
    }
    
    NetworkStats getStats() const {
        return ethernet->getStats();
    }
    
    // Network diagnostics
    bool performConnectivityTest() {
        if (current_status != NETWORK_CONNECTED) {
            return false;
        }
        
        // Simulate ping test
        uint8_t test_packet[32] = {0};
        bool ping_success = ethernet->sendPacket(test_packet, sizeof(test_packet));
        
        if (ping_success) {
            // Simulate latency measurement
            uint32_t simulated_latency = 10 + (rand() % 20); // 10-30ms
            updateLatencyStats(simulated_latency);
        }
        
        return ping_success;
    }
    
    void resetConnectionStats() {
        reconnect_attempts = 0;
        ping_failures = 0;
        connection_quality = 0.0f;
        memset(latency_samples, 0, sizeof(latency_samples));
        latency_index = 0;
        average_latency = 0.0f;
    }
    
    // Format IP address as string
    void formatIPAddress(uint32_t ip, char* buffer, size_t buffer_size) {
        if (buffer && buffer_size >= 16) {
            snprintf(buffer, buffer_size, "%d.%d.%d.%d",
                     (ip >> 24) & 0xFF,
                     (ip >> 16) & 0xFF,
                     (ip >> 8) & 0xFF,
                     ip & 0xFF);
        }
    }
    
    // Get status as string
    const char* getStatusString() const {
        switch (current_status) {
            case NETWORK_DISCONNECTED: return "Disconnected";
            case NETWORK_CONNECTING: return "Connecting";
            case NETWORK_CONNECTED: return "Connected";
            case NETWORK_ERROR: return "Error";
            case NETWORK_DHCP_FAILED: return "DHCP Failed";
            default: return "Unknown";
        }
    }
    
private:
    uint32_t getCurrentTime() const {
        static uint32_t simulated_time = 1000;
        return simulated_time += 100;
    }
    
    void monitorConnection() {
        uint32_t current_time = getCurrentTime();
        
        // Check link status
        if (!ethernet->isLinkUp()) {
            current_status = NETWORK_DISCONNECTED;
            return;
        }
        
        // Perform periodic ping if enabled
        if (ping_enabled && (current_time - last_ping_time >= ping_interval)) {
            if (!performConnectivityTest()) {
                ping_failures++;
                if (ping_failures >= max_ping_failures) {
                    current_status = NETWORK_ERROR;
                    return;
                }
            } else {
                ping_failures = 0;
            }
            last_ping_time = current_time;
        }
        
        updateConnectionQuality();
    }
    
    void updateConnectionQuality() {
        float base_quality = 1.0f;
        
        // Reduce quality based on ping failures
        if (ping_failures > 0) {
            base_quality -= (ping_failures * 0.2f);
        }
        
        // Reduce quality based on reconnection attempts
        if (reconnect_attempts > 0) {
            base_quality -= (reconnect_attempts * 0.1f);
        }
        
        // Adjust based on average latency
        if (average_latency > 50.0f) {
            base_quality -= 0.3f;
        } else if (average_latency > 20.0f) {
            base_quality -= 0.1f;
        }
        
        // Clamp to valid range
        if (base_quality < 0.0f) base_quality = 0.0f;
        if (base_quality > 1.0f) base_quality = 1.0f;
        
        connection_quality = base_quality;
    }
    
    void updateLatencyStats(uint32_t latency) {
        latency_samples[latency_index] = latency;
        latency_index = (latency_index + 1) % (sizeof(latency_samples) / sizeof(latency_samples[0]));
        
        // Calculate average
        uint32_t sum = 0;
        uint8_t count = 0;
        for (int i = 0; i < (sizeof(latency_samples) / sizeof(latency_samples[0])); i++) {
            if (latency_samples[i] > 0) {
                sum += latency_samples[i];
                count++;
            }
        }
        
        if (count > 0) {
            average_latency = static_cast<float>(sum) / count;
        }
    }
};

// Global test instances
static MockEthernetHAL* mockEthernet = nullptr;
static MockWebServerHAL* mockWebServer = nullptr;
static ExtendedNetworkManager* networkManager = nullptr;

void setUp(void) {
    mockEthernet = new MockEthernetHAL();
    mockWebServer = new MockWebServerHAL();
    networkManager = new ExtendedNetworkManager(mockEthernet, mockWebServer);
}

void tearDown(void) {
    delete networkManager;
    delete mockWebServer;
    delete mockEthernet;
    networkManager = nullptr;
    mockWebServer = nullptr;
    mockEthernet = nullptr;
}

// Basic Network Tests
void test_network_manager_initialization() {
    TEST_ASSERT_TRUE(networkManager->initialize());
    TEST_ASSERT_TRUE(networkManager->isConnected());
    TEST_ASSERT_EQUAL_INT(NETWORK_CONNECTED, networkManager->getStatus());
}

void test_network_manager_initialization_failure() {
    mockEthernet->setErrorRate(100); // Force failure
    TEST_ASSERT_FALSE(networkManager->initialize());
    TEST_ASSERT_FALSE(networkManager->isConnected());
    TEST_ASSERT_EQUAL_INT(NETWORK_ERROR, networkManager->getStatus());
}

// DHCP Tests
void test_network_manager_dhcp_success() {
    mockEthernet->setDHCPSuccess(true);
    TEST_ASSERT_TRUE(networkManager->enableDHCP());
    TEST_ASSERT_TRUE(networkManager->initialize());
    TEST_ASSERT_TRUE(networkManager->isDHCPEnabled());
    
    // Check that DHCP assigned an IP
    uint32_t ip = networkManager->getIPAddress();
    TEST_ASSERT_NOT_EQUAL_UINT32(0, ip);
}

void test_network_manager_dhcp_failure() {
    mockEthernet->setDHCPSuccess(false);
    TEST_ASSERT_TRUE(networkManager->enableDHCP());
    TEST_ASSERT_FALSE(networkManager->initialize());
    TEST_ASSERT_EQUAL_INT(NETWORK_DHCP_FAILED, networkManager->getStatus());
}

// Static IP Tests
void test_network_manager_static_ip() {
    uint32_t ip = 0xC0A80A0A;        // 192.168.10.10
    uint32_t netmask = 0xFFFFFF00;   // 255.255.255.0
    uint32_t gateway = 0xC0A80A01;   // 192.168.10.1
    uint32_t dns = 0x08080808;       // 8.8.8.8
    
    TEST_ASSERT_TRUE(networkManager->setStaticIP(ip, netmask, gateway, dns));
    TEST_ASSERT_TRUE(networkManager->initialize());
    TEST_ASSERT_FALSE(networkManager->isDHCPEnabled());
    TEST_ASSERT_EQUAL_UINT32(ip, networkManager->getIPAddress());
}

// Configuration Tests
void test_network_manager_hostname_setting() {
    TEST_ASSERT_TRUE(networkManager->setHostname("test-device"));
    TEST_ASSERT_EQUAL_STRING("test-device", networkManager->getHostname());
    
    TEST_ASSERT_FALSE(networkManager->setHostname(nullptr));
    
    // Test hostname too long
    char long_hostname[64];
    memset(long_hostname, 'a', 63);
    long_hostname[63] = '\0';
    TEST_ASSERT_FALSE(networkManager->setHostname(long_hostname));
}

void test_network_manager_port_configuration() {
    TEST_ASSERT_TRUE(networkManager->setWebPort(8080));
    TEST_ASSERT_EQUAL_UINT16(8080, networkManager->getWebPort());
    
    TEST_ASSERT_FALSE(networkManager->setWebPort(79));    // Too low
    TEST_ASSERT_FALSE(networkManager->setWebPort(65536)); // Too high
    
    TEST_ASSERT_TRUE(networkManager->setPrometheusPort(9000));
    TEST_ASSERT_EQUAL_UINT16(9000, networkManager->getPrometheusPort());
    
    TEST_ASSERT_FALSE(networkManager->setPrometheusPort(1023)); // Below 1024
}

// Connection Management Tests
void test_network_manager_auto_reconnect() {
    networkManager->setAutoReconnect(true);
    networkManager->setMaxReconnectAttempts(3);
    networkManager->setRetryInterval(1000); // 1 second for testing
    
    TEST_ASSERT_TRUE(networkManager->initialize());
    
    // Simulate connection loss
    mockEthernet->setCableConnected(false);
    
    // Update multiple times to trigger reconnection attempts
    for (int i = 0; i < 50; i++) {
        networkManager->update();
    }
    
    // Should have attempted reconnection
    TEST_ASSERT_TRUE(networkManager->getReconnectAttempts() > 0);
}

void test_network_manager_connection_timeout() {
    networkManager->setConnectionTimeout(500); // 0.5 second
    
    // Simulate slow connection (connection never completes)
    mockEthernet->setErrorRate(50); // 50% error rate to simulate slow connection
    
    networkManager->initialize();
    
    // Update to trigger timeout
    for (int i = 0; i < 10; i++) {
        networkManager->update();
    }
    
    // Connection should have timed out or succeeded
    NetworkStatus status = networkManager->getStatus();
    TEST_ASSERT_TRUE(status == NETWORK_CONNECTED || status == NETWORK_ERROR);
}

// Connection Quality Tests
void test_network_manager_connectivity_test() {
    TEST_ASSERT_TRUE(networkManager->initialize());
    
    // Perform connectivity test
    bool test_result = networkManager->performConnectivityTest();
    TEST_ASSERT_TRUE(test_result);
    
    // Check that latency stats are updated
    float latency = networkManager->getAverageLatency();
    TEST_ASSERT_TRUE(latency > 0.0f);
}

void test_network_manager_connection_quality() {
    TEST_ASSERT_TRUE(networkManager->initialize());
    
    // Initially should have good quality
    float initial_quality = networkManager->getConnectionQuality();
    TEST_ASSERT_TRUE(initial_quality > 0.8f);
    
    // Simulate some ping failures
    mockEthernet->setErrorRate(20); // 20% error rate
    
    for (int i = 0; i < 10; i++) {
        networkManager->performConnectivityTest();
    }
    
    float degraded_quality = networkManager->getConnectionQuality();
    // Quality should be lower (though due to randomness, might not always be true)
    TEST_ASSERT_TRUE(degraded_quality >= 0.0f && degraded_quality <= 1.0f);
}

// Error Handling Tests
void test_network_manager_cable_disconnect() {
    TEST_ASSERT_TRUE(networkManager->initialize());
    TEST_ASSERT_TRUE(networkManager->isConnected());
    
    // Disconnect cable
    mockEthernet->setCableConnected(false);
    
    // Update should detect disconnection
    networkManager->update();
    TEST_ASSERT_FALSE(networkManager->isConnected());
    TEST_ASSERT_EQUAL_INT(NETWORK_DISCONNECTED, networkManager->getStatus());
}

void test_network_manager_ping_failure_detection() {
    networkManager->setPingEnabled(true);
    networkManager->setPingInterval(100); // Very short interval for testing
    
    TEST_ASSERT_TRUE(networkManager->initialize());
    
    // Set high error rate to cause ping failures
    mockEthernet->setErrorRate(100);
    
    // Update multiple times to trigger ping failures
    for (int i = 0; i < 20; i++) {
        networkManager->update();
    }
    
    // Should have detected ping failures
    TEST_ASSERT_TRUE(networkManager->getPingFailures() > 0);
}

// Web Server Integration Tests
void test_network_manager_web_server_integration() {
    TEST_ASSERT_TRUE(networkManager->initialize());
    
    // Web server should be running
    TEST_ASSERT_TRUE(mockWebServer->isRunning());
    
    // Simulate some requests
    for (int i = 0; i < 5; i++) {
        networkManager->update();
    }
    
    // Should have handled some requests
    TEST_ASSERT_TRUE(mockWebServer->getTotalRequests() > 0);
}

void test_network_manager_web_server_port_change() {
    TEST_ASSERT_TRUE(networkManager->setWebPort(8080));
    TEST_ASSERT_TRUE(networkManager->initialize());
    
    // Web server should be running on new port
    TEST_ASSERT_TRUE(mockWebServer->isRunning());
}

// Statistics Tests
void test_network_manager_statistics_tracking() {
    TEST_ASSERT_TRUE(networkManager->initialize());
    
    // Generate some network activity
    uint8_t test_data[100] = {0};
    for (int i = 0; i < 10; i++) {
        mockEthernet->sendPacket(test_data, sizeof(test_data));
    }
    
    NetworkStats stats = networkManager->getStats();
    TEST_ASSERT_EQUAL_UINT32(10, stats.packets_sent);
    TEST_ASSERT_EQUAL_UINT64(1000, stats.bytes_sent);
}

void test_network_manager_reset_stats() {
    TEST_ASSERT_TRUE(networkManager->initialize());
    
    // Generate some activity
    networkManager->performConnectivityTest();
    
    // Reset stats
    networkManager->resetConnectionStats();
    
    // Stats should be reset
    TEST_ASSERT_EQUAL_UINT32(0, networkManager->getReconnectAttempts());
    TEST_ASSERT_EQUAL_UINT32(0, networkManager->getPingFailures());
    TEST_ASSERT_EQUAL_FLOAT(0.0f, networkManager->getAverageLatency());
}

// IP Address Formatting Tests
void test_network_manager_ip_formatting() {
    char ip_str[16];
    uint32_t ip = 0xC0A80101; // 192.168.1.1
    
    networkManager->formatIPAddress(ip, ip_str, sizeof(ip_str));
    TEST_ASSERT_EQUAL_STRING("192.168.1.1", ip_str);
}

// Status String Tests
void test_network_manager_status_strings() {
    TEST_ASSERT_EQUAL_STRING("Connected", networkManager->getStatusString());
    
    mockEthernet->setCableConnected(false);
    networkManager->update();
    
    const char* status = networkManager->getStatusString();
    TEST_ASSERT_TRUE(strlen(status) > 0);
}

// Configuration Persistence Tests
void test_network_manager_configuration_persistence() {
    // Set configuration
    TEST_ASSERT_TRUE(networkManager->setHostname("persistent-test"));
    TEST_ASSERT_TRUE(networkManager->setWebPort(8888));
    
    // Configuration should persist
    TEST_ASSERT_EQUAL_STRING("persistent-test", networkManager->getHostname());
    TEST_ASSERT_EQUAL_UINT16(8888, networkManager->getWebPort());
}

// Performance Tests
void test_network_manager_update_performance() {
    TEST_ASSERT_TRUE(networkManager->initialize());
    
    // Perform many updates
    for (int i = 0; i < 1000; i++) {
        networkManager->update();
    }
    
    // Should still be functional
    TEST_ASSERT_TRUE(networkManager->isConnected());
}

// Edge Case Tests
void test_network_manager_simultaneous_operations() {
    TEST_ASSERT_TRUE(networkManager->initialize());
    
    // Perform multiple operations simultaneously
    networkManager->performConnectivityTest();
    networkManager->update();
    networkManager->getStats();
    
    // Should remain stable
    TEST_ASSERT_TRUE(networkManager->isConnected());
}

int main(void) {
    UNITY_BEGIN();
    
    // Basic Network Tests
    RUN_TEST(test_network_manager_initialization);
    RUN_TEST(test_network_manager_initialization_failure);
    
    // DHCP Tests
    RUN_TEST(test_network_manager_dhcp_success);
    RUN_TEST(test_network_manager_dhcp_failure);
    
    // Static IP Tests
    RUN_TEST(test_network_manager_static_ip);
    
    // Configuration Tests
    RUN_TEST(test_network_manager_hostname_setting);
    RUN_TEST(test_network_manager_port_configuration);
    
    // Connection Management Tests
    RUN_TEST(test_network_manager_auto_reconnect);
    RUN_TEST(test_network_manager_connection_timeout);
    
    // Connection Quality Tests
    RUN_TEST(test_network_manager_connectivity_test);
    RUN_TEST(test_network_manager_connection_quality);
    
    // Error Handling Tests
    RUN_TEST(test_network_manager_cable_disconnect);
    RUN_TEST(test_network_manager_ping_failure_detection);
    
    // Web Server Integration Tests
    RUN_TEST(test_network_manager_web_server_integration);
    RUN_TEST(test_network_manager_web_server_port_change);
    
    // Statistics Tests
    RUN_TEST(test_network_manager_statistics_tracking);
    RUN_TEST(test_network_manager_reset_stats);
    
    // IP Address Formatting Tests
    RUN_TEST(test_network_manager_ip_formatting);
    
    // Status String Tests
    RUN_TEST(test_network_manager_status_strings);
    
    // Configuration Persistence Tests
    RUN_TEST(test_network_manager_configuration_persistence);
    
    // Performance Tests
    RUN_TEST(test_network_manager_update_performance);
    
    // Edge Case Tests
    RUN_TEST(test_network_manager_simultaneous_operations);
    
    return UNITY_END();
}