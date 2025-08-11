#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <memory>

// NetworkManager test structures
struct NetworkConfig {
    char hostname[32];
    uint32_t ip_address;
    uint32_t netmask;
    uint32_t gateway;
    uint32_t dns_server;
    uint16_t web_port;
    uint16_t prometheus_port;
    bool dhcp_enabled;
    uint8_t mac_address[6];
    uint32_t lease_time;
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

// Mock Interfaces
class MockEthernetInterface {
public:
    MOCK_METHOD(bool, begin, ());
    MOCK_METHOD(bool, isLinkUp, ());
    MOCK_METHOD(bool, configure, (const NetworkConfig& config));
    MOCK_METHOD(uint32_t, getIPAddress, ());
    MOCK_METHOD(uint32_t, getNetmask, ());
    MOCK_METHOD(uint32_t, getGateway, ());
    MOCK_METHOD(uint32_t, getDNSServer, ());
    MOCK_METHOD(bool, sendPacket, (const uint8_t* data, size_t len));
    MOCK_METHOD(bool, receivePacket, (uint8_t* buffer, size_t* len));
    MOCK_METHOD(void, updateStats, ());
    MOCK_METHOD(NetworkStats, getStats, ());
};

class MockWebServerInterface {
public:
    MOCK_METHOD(bool, begin, (uint16_t server_port));
    MOCK_METHOD(void, stop, ());
    MOCK_METHOD(bool, handleRequest, ());
    MOCK_METHOD(bool, isRunning, ());
    MOCK_METHOD(uint32_t, getTotalRequests, ());
};

// Concrete Mock HAL for complex scenarios
class ConcreteMockEthernetHAL {
public:
    bool cable_connected = true;
    bool link_up = true;
    bool dhcp_success = true;
    NetworkConfig current_config;
    NetworkStats stats;
    int error_rate = 0;
    bool initialized = false;
    
    ConcreteMockEthernetHAL() {
        reset();
    }
    
    bool begin() {
        if (error_rate > 0 && (std::rand() % 100) < error_rate) {
            return false;
        }
        initialized = true;
        return true;
    }
    
    bool isLinkUp() {
        return cable_connected && link_up;
    }
    
    bool configure(const NetworkConfig& config) {
        if (error_rate > 0 && (std::rand() % 100) < error_rate) {
            return false;
        }
        
        current_config = config;
        
        if (config.dhcp_enabled) {
            if (dhcp_success) {
                current_config.ip_address = 0xC0A80164; // 192.168.1.100
                current_config.netmask = 0xFFFFFF00;
                current_config.gateway = 0xC0A80101;
                current_config.dns_server = 0x08080808;
                current_config.lease_time = 3600;
                return true;
            } else {
                return false;
            }
        }
        
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
        if (!isLinkUp() || (error_rate > 0 && (std::rand() % 100) < error_rate)) {
            stats.packets_dropped++;
            stats.connection_errors++;
            return false;
        }
        
        stats.packets_sent++;
        stats.bytes_sent += len;
        return true;
    }
    
    bool receivePacket(uint8_t* buffer, size_t* len) {
        if (!isLinkUp() || (error_rate > 0 && (std::rand() % 100) < error_rate)) {
            return false;
        }
        
        if (std::rand() % 10 == 0) {
            *len = 64;
            stats.packets_received++;
            stats.bytes_received += *len;
            return true;
        }
        
        return false;
    }
    
    void updateStats() {
        stats.uptime_seconds++;
        uint64_t total_bytes = stats.bytes_sent + stats.bytes_received;
        stats.bandwidth_utilization = (total_bytes % 100) / 100.0f;
        
        if (current_config.dhcp_enabled && stats.uptime_seconds % 1800 == 0) {
            stats.dhcp_renewals++;
        }
    }
    
    NetworkStats getStats() const {
        return stats;
    }
    
    void reset() {
        std::memset(&current_config, 0, sizeof(current_config));
        std::memset(&stats, 0, sizeof(stats));
        std::strcpy(current_config.hostname, "gps-ntp");
        current_config.web_port = 80;
        current_config.prometheus_port = 9090;
        current_config.dhcp_enabled = true;
        
        cable_connected = true;
        link_up = true;
        dhcp_success = true;
        error_rate = 0;
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
};

class ConcreteMockWebServerHAL {
public:
    bool server_running = false;
    uint16_t port = 80;
    uint32_t total_requests = 0;
    uint32_t successful_responses = 0;
    uint32_t error_responses = 0;
    int error_rate = 0;
    
    bool begin(uint16_t server_port) {
        if (error_rate > 0 && (std::rand() % 100) < error_rate) {
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
        
        if (error_rate > 0 && (std::rand() % 100) < error_rate) {
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

// Extended NetworkManager
class ExtendedNetworkManager {
private:
    ConcreteMockEthernetHAL* ethernet;
    ConcreteMockWebServerHAL* webserver;
    NetworkConfig config;
    NetworkStatus current_status;
    uint32_t last_connection_attempt;
    uint32_t connection_retry_interval;
    uint32_t connection_timeout;
    bool auto_reconnect;
    uint32_t reconnect_attempts;
    uint32_t max_reconnect_attempts;
    
    uint32_t last_ping_time;
    uint32_t ping_interval;
    bool ping_enabled;
    uint32_t ping_failures;
    uint32_t max_ping_failures;
    
    float connection_quality;
    uint32_t latency_samples[10];
    uint8_t latency_index;
    float average_latency;
    
public:
    ExtendedNetworkManager(ConcreteMockEthernetHAL* eth, ConcreteMockWebServerHAL* web) 
        : ethernet(eth), webserver(web) {
        current_status = NETWORK_DISCONNECTED;
        last_connection_attempt = 0;
        connection_retry_interval = 5000;
        connection_timeout = 10000;
        auto_reconnect = true;
        reconnect_attempts = 0;
        max_reconnect_attempts = 10;
        
        last_ping_time = 0;
        ping_interval = 30000;
        ping_enabled = true;
        ping_failures = 0;
        max_ping_failures = 3;
        
        connection_quality = 0.0f;
        std::memset(latency_samples, 0, sizeof(latency_samples));
        latency_index = 0;
        average_latency = 0.0f;
        
        loadDefaultConfig();
    }
    
    uint32_t getCurrentTime() const {
        static uint32_t time_counter = 1000;
        return time_counter += 100;
    }
    
    void loadDefaultConfig() {
        std::strcpy(config.hostname, "gps-ntp-server");
        config.ip_address = 0;
        config.netmask = 0xFFFFFF00;
        config.gateway = 0xC0A80101;
        config.dns_server = 0x08080808;
        config.web_port = 80;
        config.prometheus_port = 9090;
        config.dhcp_enabled = true;
        config.lease_time = 3600;
        
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
            return false;
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
                monitorConnection();
                webserver->handleRequest();
                ethernet->updateStats();
                break;
        }
    }
    
    void updateConnectionQuality() {
        connection_quality = 0.9f; // Simulate good quality
    }
    
    void monitorConnection() {
        // Simulate connection monitoring
        if (ping_enabled) {
            uint32_t current_time = getCurrentTime();
            if (current_time - last_ping_time >= ping_interval) {
                last_ping_time = current_time;
                // Perform connectivity test
            }
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
        if (!hostname || std::strlen(hostname) >= sizeof(config.hostname)) {
            return false;
        }
        std::strcpy(config.hostname, hostname);
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
    
    bool performConnectivityTest() {
        if (current_status != NETWORK_CONNECTED) {
            return false;
        }
        
        uint8_t test_packet[32] = {0};
        bool ping_success = ethernet->sendPacket(test_packet, sizeof(test_packet));
        
        return ping_success;
    }
    
    void resetConnectionStats() {
        reconnect_attempts = 0;
        ping_failures = 0;
        connection_quality = 0.0f;
        std::memset(latency_samples, 0, sizeof(latency_samples));
        latency_index = 0;
        average_latency = 0.0f;
    }
    
    void formatIPAddress(uint32_t ip, char* buffer, size_t buffer_size) {
        if (buffer && buffer_size >= 16) {
            std::snprintf(buffer, buffer_size, "%d.%d.%d.%d",
                     (ip >> 24) & 0xFF,
                     (ip >> 16) & 0xFF,
                     (ip >> 8) & 0xFF,
                     ip & 0xFF);
        }
    }
    
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
};

// Test fixture class
class NetworkManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockEthernet = std::make_unique<ConcreteMockEthernetHAL>();
        mockWebServer = std::make_unique<ConcreteMockWebServerHAL>();
        networkManager = std::make_unique<ExtendedNetworkManager>(mockEthernet.get(), mockWebServer.get());
    }
    
    void TearDown() override {
        networkManager.reset();
        mockWebServer.reset();
        mockEthernet.reset();
    }
    
    std::unique_ptr<ConcreteMockEthernetHAL> mockEthernet;
    std::unique_ptr<ConcreteMockWebServerHAL> mockWebServer;
    std::unique_ptr<ExtendedNetworkManager> networkManager;
};

// Basic NetworkManager Tests
TEST_F(NetworkManagerTest, Initialization) {
    EXPECT_TRUE(networkManager->initialize());
    EXPECT_TRUE(networkManager->isConnected());
    EXPECT_EQ(NETWORK_CONNECTED, networkManager->getStatus());
}

TEST_F(NetworkManagerTest, InitializationFailure) {
    mockEthernet->setErrorRate(100);
    
    EXPECT_FALSE(networkManager->initialize());
    EXPECT_EQ(NETWORK_ERROR, networkManager->getStatus());
}

TEST_F(NetworkManagerTest, DHCPConfiguration) {
    EXPECT_TRUE(networkManager->enableDHCP());
    EXPECT_TRUE(networkManager->isDHCPEnabled());
    
    EXPECT_TRUE(networkManager->initialize());
    EXPECT_EQ(0xC0A80164U, networkManager->getIPAddress()); // 192.168.1.100
}

TEST_F(NetworkManagerTest, StaticIPConfiguration) {
    uint32_t static_ip = 0xC0A80165; // 192.168.1.101
    uint32_t netmask = 0xFFFFFF00;   // 255.255.255.0
    uint32_t gateway = 0xC0A80101;   // 192.168.1.1
    uint32_t dns = 0x08080808;       // 8.8.8.8
    
    EXPECT_TRUE(networkManager->setStaticIP(static_ip, netmask, gateway, dns));
    EXPECT_FALSE(networkManager->isDHCPEnabled());
    
    EXPECT_TRUE(networkManager->initialize());
    EXPECT_EQ(static_ip, networkManager->getIPAddress());
}

TEST_F(NetworkManagerTest, HostnameConfiguration) {
    EXPECT_TRUE(networkManager->setHostname("test-server"));
    EXPECT_STREQ("test-server", networkManager->getHostname());
    
    // Test invalid hostname
    EXPECT_FALSE(networkManager->setHostname(nullptr));
    
    char long_hostname[64];
    std::memset(long_hostname, 'a', 63);
    long_hostname[63] = '\0';
    EXPECT_FALSE(networkManager->setHostname(long_hostname));
}

TEST_F(NetworkManagerTest, PortConfiguration) {
    EXPECT_TRUE(networkManager->setWebPort(8080));
    EXPECT_EQ(8080, networkManager->getWebPort());
    
    EXPECT_TRUE(networkManager->setPrometheusPort(8090));
    EXPECT_EQ(8090, networkManager->getPrometheusPort());
    
    // Test invalid ports
    EXPECT_FALSE(networkManager->setWebPort(79));
    EXPECT_FALSE(networkManager->setPrometheusPort(80));
    EXPECT_FALSE(networkManager->setPrometheusPort(65536));
}

TEST_F(NetworkManagerTest, ConnectionManagement) {
    EXPECT_TRUE(networkManager->initialize());
    EXPECT_TRUE(networkManager->isConnected());
    
    networkManager->disconnect();
    EXPECT_FALSE(networkManager->isConnected());
    EXPECT_EQ(NETWORK_DISCONNECTED, networkManager->getStatus());
}

TEST_F(NetworkManagerTest, DHCPFailureHandling) {
    mockEthernet->setDHCPSuccess(false);
    
    EXPECT_FALSE(networkManager->initialize());
    EXPECT_EQ(NETWORK_DHCP_FAILED, networkManager->getStatus());
}

TEST_F(NetworkManagerTest, CableDisconnectionHandling) {
    EXPECT_TRUE(networkManager->initialize());
    EXPECT_TRUE(networkManager->isConnected());
    
    mockEthernet->setCableConnected(false);
    EXPECT_FALSE(networkManager->connect());
    EXPECT_EQ(NETWORK_DISCONNECTED, networkManager->getStatus());
}

TEST_F(NetworkManagerTest, AutoReconnectFeature) {
    networkManager->setAutoReconnect(true);
    networkManager->setMaxReconnectAttempts(3);
    networkManager->setRetryInterval(100); // Short interval for testing
    
    // Start disconnected
    mockEthernet->setCableConnected(false);
    EXPECT_FALSE(networkManager->initialize());
    
    // Reconnect cable
    mockEthernet->setCableConnected(true);
    
    // Update should trigger reconnection attempts
    for (int i = 0; i < 10; i++) {
        networkManager->update();
    }
    
    EXPECT_GT(networkManager->getReconnectAttempts(), 0U);
}

TEST_F(NetworkManagerTest, ConnectivityTesting) {
    EXPECT_TRUE(networkManager->initialize());
    
    EXPECT_TRUE(networkManager->performConnectivityTest());
    
    // Test when disconnected
    networkManager->disconnect();
    EXPECT_FALSE(networkManager->performConnectivityTest());
}

TEST_F(NetworkManagerTest, NetworkStatistics) {
    EXPECT_TRUE(networkManager->initialize());
    
    // Generate some traffic
    uint8_t test_data[100] = {0};
    mockEthernet->sendPacket(test_data, sizeof(test_data));
    
    NetworkStats stats = networkManager->getStats();
    EXPECT_GT(stats.packets_sent, 0U);
    EXPECT_GT(stats.bytes_sent, 0U);
}

TEST_F(NetworkManagerTest, IPAddressFormatting) {
    uint32_t test_ip = 0xC0A80101; // 192.168.1.1
    char ip_str[16];
    
    networkManager->formatIPAddress(test_ip, ip_str, sizeof(ip_str));
    EXPECT_STREQ("192.168.1.1", ip_str);
}

TEST_F(NetworkManagerTest, StatusStringConversion) {
    EXPECT_TRUE(networkManager->initialize());
    EXPECT_STREQ("Connected", networkManager->getStatusString());
    
    networkManager->disconnect();
    EXPECT_STREQ("Disconnected", networkManager->getStatusString());
}

TEST_F(NetworkManagerTest, ConnectionQuality) {
    EXPECT_TRUE(networkManager->initialize());
    EXPECT_GT(networkManager->getConnectionQuality(), 0.0f);
}

// Parameterized Tests for different network conditions
class NetworkConditionTest : public NetworkManagerTest, 
                            public ::testing::WithParamInterface<std::tuple<bool, bool, bool, NetworkStatus>> {};

TEST_P(NetworkConditionTest, NetworkConditions) {
    auto [cable_connected, dhcp_success, ethernet_init_success, expected_status] = GetParam();
    
    mockEthernet->setCableConnected(cable_connected);
    mockEthernet->setDHCPSuccess(dhcp_success);
    if (!ethernet_init_success) {
        mockEthernet->setErrorRate(100);
    }
    
    bool init_result = networkManager->initialize();
    NetworkStatus actual_status = networkManager->getStatus();
    
    if (expected_status == NETWORK_CONNECTED) {
        EXPECT_TRUE(init_result);
        EXPECT_EQ(expected_status, actual_status);
    } else {
        EXPECT_FALSE(init_result);
        EXPECT_EQ(expected_status, actual_status);
    }
}

INSTANTIATE_TEST_SUITE_P(
    NetworkConditions,
    NetworkConditionTest,
    ::testing::Values(
        std::make_tuple(true, true, true, NETWORK_CONNECTED),       // All OK
        std::make_tuple(false, true, true, NETWORK_DISCONNECTED),   // No cable
        std::make_tuple(true, false, true, NETWORK_DHCP_FAILED),    // DHCP failed
        std::make_tuple(true, true, false, NETWORK_ERROR)           // Ethernet init failed
    )
);

TEST_F(NetworkManagerTest, PeriodicUpdate) {
    networkManager->setAutoReconnect(true);
    networkManager->setRetryInterval(100);
    
    // Start with successful connection
    EXPECT_TRUE(networkManager->initialize());
    EXPECT_TRUE(networkManager->isConnected());
    
    // Simulate periodic updates
    for (int i = 0; i < 10; i++) {
        networkManager->update();
    }
    
    // Should still be connected
    EXPECT_TRUE(networkManager->isConnected());
}

TEST_F(NetworkManagerTest, ConnectionStatsReset) {
    EXPECT_TRUE(networkManager->initialize());
    
    // Generate some activity to create stats
    networkManager->performConnectivityTest();
    
    networkManager->resetConnectionStats();
    
    EXPECT_EQ(0U, networkManager->getReconnectAttempts());
    EXPECT_EQ(0U, networkManager->getPingFailures());
    EXPECT_EQ(0.0f, networkManager->getConnectionQuality());
    EXPECT_EQ(0.0f, networkManager->getAverageLatency());
}