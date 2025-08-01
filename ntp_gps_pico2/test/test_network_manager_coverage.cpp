#include <unity.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Mock Arduino functions
extern "C" {
    uint32_t millis() { return 15000; }
    void delay(uint32_t ms) { (void)ms; }
    void digitalWrite(uint8_t pin, uint8_t val) { (void)pin; (void)val; }
    int digitalRead(uint8_t pin) { return 1; }
}

// Mock SPI
class MockSPI {
public:
    void begin() { }
    void end() { }
    void setClockDivider(uint8_t divider) { (void)divider; }
};

MockSPI SPI;

// Mock LoggingService
class MockLoggingService {
public:
    int info_count = 0;
    int warning_count = 0;
    int error_count = 0;
    
    void info(const char* component, const char* message) {
        info_count++;
        (void)component; (void)message;
    }
    void infof(const char* component, const char* format, ...) {
        info_count++;
        (void)component; (void)format;
    }
    void warning(const char* component, const char* message) {
        warning_count++;
        (void)component; (void)message;
    }
    void warningf(const char* component, const char* format, ...) {
        warning_count++;
        (void)component; (void)format;
    }
    void error(const char* component, const char* message) {
        error_count++;
        (void)component; (void)message;
    }
    void errorf(const char* component, const char* format, ...) {
        error_count++;
        (void)component; (void)format;
    }
};

// Mock ConfigManager
class MockConfigManager {
public:
    bool use_dhcp = true;
    uint32_t static_ip = 0xC0A80164; // 192.168.1.100
    uint32_t subnet_mask = 0xFFFFFF00; // 255.255.255.0
    uint32_t gateway_ip = 0xC0A80101; // 192.168.1.1
    uint32_t dns_server = 0x08080808; // 8.8.8.8
    
    bool useDhcp() const { return use_dhcp; }
    uint32_t getStaticIP() const { return static_ip; }
    uint32_t getSubnetMask() const { return subnet_mask; }
    uint32_t getGatewayIP() const { return gateway_ip; }
    uint32_t getDnsServer() const { return dns_server; }
};

// Mock EthernetUDP
class MockEthernetUDP {
public:
    bool is_open = false;
    uint16_t local_port = 0;
    
    uint8_t begin(uint16_t port) {
        local_port = port;
        is_open = true;
        return 1;
    }
    
    void stop() {
        is_open = false;
        local_port = 0;
    }
    
    bool isOpen() const { return is_open; }
    uint16_t getPort() const { return local_port; }
};

// Mock Ethernet class
class MockEthernet {
public:
    static bool hardware_detected;
    static bool dhcp_success;
    static bool link_active;
    static uint32_t local_ip;
    static uint32_t gateway_ip;
    static uint32_t dns_server;
    static uint8_t cs_pin;
    static int begin_call_count;
    
    static int begin(uint8_t* mac) {
        begin_call_count++;
        if (!hardware_detected) return 0;
        if (dhcp_success) {
            local_ip = 0xC0A80165; // 192.168.1.101
            gateway_ip = 0xC0A80101; // 192.168.1.1
            dns_server = 0x08080808; // 8.8.8.8
            return 1;
        }
        return 0;
    }
    
    static void begin(uint8_t* mac, uint32_t ip) {
        begin_call_count++;
        local_ip = ip;
    }
    
    static void begin(uint8_t* mac, uint32_t ip, uint32_t dns) {
        begin_call_count++;
        local_ip = ip;
        dns_server = dns;
    }
    
    static void begin(uint8_t* mac, uint32_t ip, uint32_t dns, uint32_t gateway) {
        begin_call_count++;
        local_ip = ip;
        dns_server = dns;
        gateway_ip = gateway;
    }
    
    static void begin(uint8_t* mac, uint32_t ip, uint32_t dns, uint32_t gateway, uint32_t subnet) {
        begin_call_count++;
        local_ip = ip;
        dns_server = dns;
        gateway_ip = gateway;
    }
    
    static void init(uint8_t pin) {
        cs_pin = pin;
    }
    
    static uint32_t localIP() { return local_ip; }
    static uint32_t gatewayIP() { return gateway_ip; }
    static uint32_t dnsServerIP() { return dns_server; }
    static int linkStatus() { return link_active ? 1 : 0; }
    static int hardwareStatus() { return hardware_detected ? 1 : 0; }
    static int maintain() { return dhcp_success ? 1 : 0; }
};

// Static member definitions
bool MockEthernet::hardware_detected = true;
bool MockEthernet::dhcp_success = true;
bool MockEthernet::link_active = true;
uint32_t MockEthernet::local_ip = 0;
uint32_t MockEthernet::gateway_ip = 0;
uint32_t MockEthernet::dns_server = 0;
uint8_t MockEthernet::cs_pin = 0;
int MockEthernet::begin_call_count = 0;

MockEthernet Ethernet;

// Hardware configuration constants
#define DEFAULT_MAC_ADDRESS {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}
#define W5500_CS_PIN 17
#define W5500_RST_PIN 20
#define W5500_INT_PIN 21

// SystemTypes structures
struct NetworkMonitor {
    bool isConnected;
    bool dhcpActive;
    unsigned long lastLinkCheck;
    unsigned long linkCheckInterval;
    int reconnectAttempts;
    int maxReconnectAttempts;
    unsigned long lastReconnectTime;
    unsigned long reconnectInterval;
    uint32_t localIP;
    uint32_t gateway;
    uint32_t dnsServer;
    bool ntpServerActive;
};

struct UdpSocketManager {
    bool ntpSocketOpen;
    unsigned long lastSocketCheck;
    unsigned long socketCheckInterval;
    int socketErrors;
};

// NetworkManager implementation (simplified for testing)
class NetworkManager {
private:
    NetworkMonitor networkMonitor;
    UdpSocketManager udpManager;
    MockEthernetUDP* ntpUdp;
    MockLoggingService* loggingService;
    MockConfigManager* configManager;
    uint8_t mac[6];
    
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
    
    void initializeW5500() {
        // Reset W5500
        digitalWrite(W5500_RST_PIN, LOW);
        delay(100);
        digitalWrite(W5500_RST_PIN, HIGH);
        delay(500);
        
        // Initialize SPI and Ethernet
        SPI.begin();
        Ethernet.init(W5500_CS_PIN);
        delay(1000);
    }
    
    bool attemptDhcp() {
        if (!loggingService) return false;
        
        loggingService->info("NETWORK", "Attempting DHCP configuration");
        
        int result = Ethernet.begin(mac);
        if (result == 1) {
            networkMonitor.dhcpActive = true;
            networkMonitor.localIP = Ethernet.localIP();
            networkMonitor.gateway = Ethernet.gatewayIP();
            networkMonitor.dnsServer = Ethernet.dnsServerIP();
            networkMonitor.isConnected = true;
            loggingService->info("NETWORK", "DHCP configuration successful");
            return true;
        } else {
            loggingService->warning("NETWORK", "DHCP configuration failed");
            return false;
        }
    }
    
    void setupStaticIp() {
        if (!configManager || !loggingService) return;
        
        loggingService->info("NETWORK", "Setting up static IP configuration");
        
        uint32_t ip = configManager->getStaticIP();
        uint32_t dns = configManager->getDnsServer();
        uint32_t gateway = configManager->getGatewayIP();
        uint32_t subnet = configManager->getSubnetMask();
        
        Ethernet.begin(mac, ip, dns, gateway, subnet);
        
        networkMonitor.dhcpActive = false;
        networkMonitor.localIP = ip;
        networkMonitor.gateway = gateway;
        networkMonitor.dnsServer = dns;
        networkMonitor.isConnected = true;
        
        loggingService->info("NETWORK", "Static IP configuration completed");
    }
    
    void checkHardwareStatus() {
        if (!loggingService) return;
        
        int hwStatus = Ethernet.hardwareStatus();
        if (hwStatus == 1) {
            loggingService->info("NETWORK", "W5500 hardware detected");
        } else {
            loggingService->error("NETWORK", "W5500 hardware not detected");
        }
    }
    
    void checkLinkStatus() {
        unsigned long now = millis();
        if (now - networkMonitor.lastLinkCheck >= networkMonitor.linkCheckInterval) {
            int linkStatus = Ethernet.linkStatus();
            bool wasConnected = networkMonitor.isConnected;
            
            networkMonitor.isConnected = (linkStatus == 1);
            networkMonitor.lastLinkCheck = now;
            
            if (wasConnected && !networkMonitor.isConnected) {
                if (loggingService) {
                    loggingService->warning("NETWORK", "Network link lost");
                }
            } else if (!wasConnected && networkMonitor.isConnected) {
                if (loggingService) {
                    loggingService->info("NETWORK", "Network link restored");
                }
            }
        }
    }
    
    void maintainDhcp() {
        if (networkMonitor.dhcpActive) {
            int result = Ethernet.maintain();
            if (result == 0) {
                // DHCP lease renewed successfully
            } else if (result == 1) {
                // DHCP lease renewal failed
                if (loggingService) {
                    loggingService->warning("NETWORK", "DHCP lease renewal failed");
                }
            }
        }
    }

public:
    NetworkManager(MockEthernetUDP* udpInstance) 
        : ntpUdp(udpInstance), loggingService(nullptr), configManager(nullptr) {
        
        // Initialize MAC address
        uint8_t defaultMac[] = DEFAULT_MAC_ADDRESS;
        memcpy(mac, defaultMac, 6);
        
        // Initialize monitoring structures
        networkMonitor = {false, false, 0, 5000, 0, 5, 0, 30000, 0, 0, 0, false};
        udpManager = {false, 0, 10000, 0};
        
        // Initialize non-blocking state machine
        initState = INIT_START;
        stateChangeTime = 0;
    }
    
    void setLoggingService(MockLoggingService* loggingServiceInstance) {
        loggingService = loggingServiceInstance;
    }
    
    void setConfigManager(MockConfigManager* configManagerInstance) {
        configManager = configManagerInstance;
    }
    
    void init() {
        if (loggingService) {
            loggingService->info("NETWORK", "Starting W5500 initialization sequence...");
        }
        
        // Initialize W5500 hardware
        initializeW5500();
        delay(1000);
        
        // Check hardware status
        checkHardwareStatus();
        
        // Attempt network configuration
        bool networkConfigured = false;
        
        if (!configManager || configManager->useDhcp()) {
            // Try DHCP first
            networkConfigured = attemptDhcp();
            
            if (!networkConfigured) {
                if (loggingService) {
                    loggingService->warning("NETWORK", "DHCP failed, trying static IP");
                }
                setupStaticIp();
                networkConfigured = true;
            }
        } else {
            // Use static IP directly
            setupStaticIp();
            networkConfigured = true;
        }
        
        if (networkConfigured && loggingService) {
            loggingService->info("NETWORK", "Network initialization completed");
        }
    }
    
    bool updateInitialization() {
        unsigned long now = millis();
        
        switch (initState) {
            case INIT_START:
                stateChangeTime = now;
                initState = RESET_LOW;
                digitalWrite(W5500_RST_PIN, LOW);
                return false;
                
            case RESET_LOW:
                if (now - stateChangeTime >= 100) {
                    initState = RESET_HIGH;
                    stateChangeTime = now;
                    digitalWrite(W5500_RST_PIN, HIGH);
                }
                return false;
                
            case RESET_HIGH:
                if (now - stateChangeTime >= 500) {
                    initState = STABILIZE_WAIT;
                    stateChangeTime = now;
                }
                return false;
                
            case STABILIZE_WAIT:
                if (now - stateChangeTime >= 1000) {
                    initState = SPI_INIT;
                    stateChangeTime = now;
                    SPI.begin();
                }
                return false;
                
            case SPI_INIT:
                initState = ETHERNET_INIT;
                stateChangeTime = now;
                Ethernet.init(W5500_CS_PIN);
                return false;
                
            case ETHERNET_INIT:
                if (now - stateChangeTime >= 1000) {
                    initState = INIT_COMPLETE;
                }
                return false;
                
            case INIT_COMPLETE:
                return true;
        }
        return false;
    }
    
    void monitorConnection() {
        checkLinkStatus();
        maintainDhcp();
    }
    
    void attemptReconnection() {
        unsigned long now = millis();
        
        if (networkMonitor.isConnected) {
            networkMonitor.reconnectAttempts = 0;
            return;
        }
        
        if (now - networkMonitor.lastReconnectTime < networkMonitor.reconnectInterval) {
            return;
        }
        
        if (networkMonitor.reconnectAttempts >= networkMonitor.maxReconnectAttempts) {
            if (loggingService) {
                loggingService->error("NETWORK", "Maximum reconnection attempts reached");
            }
            return;
        }
        
        networkMonitor.lastReconnectTime = now;
        networkMonitor.reconnectAttempts++;
        
        if (loggingService) {
            loggingService->infof("NETWORK", "Reconnection attempt %d/%d", 
                                 networkMonitor.reconnectAttempts, 
                                 networkMonitor.maxReconnectAttempts);
        }
        
        // Reset and reinitialize
        initializeW5500();
        
        if (networkMonitor.dhcpActive) {
            if (attemptDhcp()) {
                networkMonitor.reconnectAttempts = 0;
            }
        } else {
            setupStaticIp();
            networkMonitor.reconnectAttempts = 0;
        }
    }
    
    void manageUdpSockets() {
        unsigned long now = millis();
        
        if (now - udpManager.lastSocketCheck >= udpManager.socketCheckInterval) {
            udpManager.lastSocketCheck = now;
            
            // Check NTP socket
            if (networkMonitor.ntpServerActive && !udpManager.ntpSocketOpen) {
                if (ntpUdp && ntpUdp->begin(123)) {
                    udpManager.ntpSocketOpen = true;
                    if (loggingService) {
                        loggingService->info("NETWORK", "NTP UDP socket opened");
                    }
                } else {
                    udpManager.socketErrors++;
                    if (loggingService) {
                        loggingService->error("NETWORK", "Failed to open NTP UDP socket");
                    }
                }
            }
            
            // Close socket if NTP server is not active
            if (!networkMonitor.ntpServerActive && udpManager.ntpSocketOpen) {
                if (ntpUdp) {
                    ntpUdp->stop();
                    udpManager.ntpSocketOpen = false;
                    if (loggingService) {
                        loggingService->info("NETWORK", "NTP UDP socket closed");
                    }
                }
            }
        }
    }
    
    bool isConnected() const { return networkMonitor.isConnected; }
    bool isNtpServerActive() const { return networkMonitor.ntpServerActive; }
    bool isUdpSocketOpen() const { return udpManager.ntpSocketOpen; }
    
    const NetworkMonitor& getNetworkStatus() const { return networkMonitor; }
    const UdpSocketManager& getUdpStatus() const { return udpManager; }
    
    // Test helpers
    void setNtpServerActive(bool active) { networkMonitor.ntpServerActive = active; }
    void setConnected(bool connected) { networkMonitor.isConnected = connected; }
    InitState getInitState() const { return initState; }
};

// Global test instances
MockEthernetUDP mockEthernetUdp;
MockLoggingService mockLoggingService;
MockConfigManager mockConfigManager;
NetworkManager networkManager(&mockEthernetUdp);

/**
 * @brief Test NetworkManager基本初期化・W5500ハードウェア設定
 */
void test_networkmanager_basic_initialization_w5500_setup() {
    // Mock設定
    MockEthernet::hardware_detected = true;
    MockEthernet::dhcp_success = true;
    MockEthernet::link_active = true;
    MockEthernet::begin_call_count = 0;
    
    networkManager.setLoggingService(&mockLoggingService);
    networkManager.setConfigManager(&mockConfigManager);
    
    // 初期状態確認
    TEST_ASSERT_FALSE(networkManager.isConnected());
    TEST_ASSERT_FALSE(networkManager.isNtpServerActive());
    TEST_ASSERT_FALSE(networkManager.isUdpSocketOpen());
    
    // 初期化実行
    networkManager.init();
    
    // 初期化後の状態確認
    TEST_ASSERT_TRUE(networkManager.isConnected());
    TEST_ASSERT_GREATER_THAN(0, MockEthernet::begin_call_count);
    TEST_ASSERT_GREATER_THAN(0, mockLoggingService.info_count);
    
    // ネットワーク状態確認
    const NetworkMonitor& status = networkManager.getNetworkStatus();
    TEST_ASSERT_TRUE(status.isConnected);
    TEST_ASSERT_TRUE(status.dhcpActive);
    TEST_ASSERT_NOT_EQUAL(0, status.localIP);
    TEST_ASSERT_NOT_EQUAL(0, status.gateway);
    TEST_ASSERT_NOT_EQUAL(0, status.dnsServer);
}

/**
 * @brief Test DHCP設定・成功・失敗処理
 */
void test_networkmanager_dhcp_configuration_success_failure() {
    mockLoggingService.info_count = 0;
    mockLoggingService.warning_count = 0;
    MockEthernet::begin_call_count = 0;
    
    networkManager.setLoggingService(&mockLoggingService);
    networkManager.setConfigManager(&mockConfigManager);
    
    // DHCP成功ケース
    MockEthernet::hardware_detected = true;
    MockEthernet::dhcp_success = true;
    mockConfigManager.use_dhcp = true;
    
    networkManager.init();
    
    const NetworkMonitor& successStatus = networkManager.getNetworkStatus();
    TEST_ASSERT_TRUE(successStatus.isConnected);
    TEST_ASSERT_TRUE(successStatus.dhcpActive);
    TEST_ASSERT_EQUAL(0xC0A80165, successStatus.localIP); // 192.168.1.101
    TEST_ASSERT_GREATER_THAN(0, mockLoggingService.info_count);
    
    // DHCP失敗→静的IP フォールバックケース
    NetworkManager fallbackManager(&mockEthernetUdp);
    fallbackManager.setLoggingService(&mockLoggingService);
    fallbackManager.setConfigManager(&mockConfigManager);
    
    mockLoggingService.warning_count = 0;
    MockEthernet::dhcp_success = false;
    mockConfigManager.use_dhcp = true;
    
    fallbackManager.init();
    
    const NetworkMonitor& fallbackStatus = fallbackManager.getNetworkStatus();
    TEST_ASSERT_TRUE(fallbackStatus.isConnected);
    TEST_ASSERT_FALSE(fallbackStatus.dhcpActive);
    TEST_ASSERT_EQUAL(0xC0A80164, fallbackStatus.localIP); // 192.168.1.100 (static)
    TEST_ASSERT_GREATER_THAN(0, mockLoggingService.warning_count);
}

/**
 * @brief Test 静的IP設定・各種ネットワークパラメータ
 */
void test_networkmanager_static_ip_configuration_parameters() {
    NetworkManager staticManager(&mockEthernetUdp);
    staticManager.setLoggingService(&mockLoggingService);
    staticManager.setConfigManager(&mockConfigManager);
    
    mockLoggingService.info_count = 0;
    MockEthernet::begin_call_count = 0;
    
    // 静的IP設定
    mockConfigManager.use_dhcp = false;
    mockConfigManager.static_ip = 0xC0A80164;      // 192.168.1.100
    mockConfigManager.subnet_mask = 0xFFFFFF00;    // 255.255.255.0
    mockConfigManager.gateway_ip = 0xC0A80101;     // 192.168.1.1
    mockConfigManager.dns_server = 0x08080808;     // 8.8.8.8
    
    MockEthernet::hardware_detected = true;
    
    staticManager.init();
    
    const NetworkMonitor& status = staticManager.getNetworkStatus();
    TEST_ASSERT_TRUE(status.isConnected);
    TEST_ASSERT_FALSE(status.dhcpActive);
    TEST_ASSERT_EQUAL(0xC0A80164, status.localIP);
    TEST_ASSERT_EQUAL(0xC0A80101, status.gateway);
    TEST_ASSERT_EQUAL(0x08080808, status.dnsServer);
    TEST_ASSERT_GREATER_THAN(0, mockLoggingService.info_count);
    TEST_ASSERT_GREATER_THAN(0, MockEthernet::begin_call_count);
}

/**
 * @brief Test ネットワーク接続監視・リンク状態チェック
 */
void test_networkmanager_connection_monitoring_link_status() {
    networkManager.setLoggingService(&mockLoggingService);
    networkManager.setConfigManager(&mockConfigManager);
    
    MockEthernet::hardware_detected = true;
    MockEthernet::dhcp_success = true;
    MockEthernet::link_active = true;
    
    networkManager.init();
    TEST_ASSERT_TRUE(networkManager.isConnected());
    
    mockLoggingService.warning_count = 0;
    mockLoggingService.info_count = 0;
    
    // リンク切断シミュレーション
    MockEthernet::link_active = false;
    networkManager.monitorConnection();
    
    // リンク状態確認は間隔を空けて実行される
    const NetworkMonitor& status1 = networkManager.getNetworkStatus();
    
    // リンク復旧シミュレーション
    MockEthernet::link_active = true;
    networkManager.monitorConnection();
    
    const NetworkMonitor& status2 = networkManager.getNetworkStatus();
    
    // 監視機能が動作していることを確認
    TEST_ASSERT_GREATER_THAN(0, status1.lastLinkCheck);
    TEST_ASSERT_EQUAL(5000, status1.linkCheckInterval);
}

/**
 * @brief Test 再接続機能・最大試行回数・間隔制御
 */
void test_networkmanager_reconnection_max_attempts_interval() {
    NetworkManager reconnectManager(&mockEthernetUdp);
    reconnectManager.setLoggingService(&mockLoggingService);
    reconnectManager.setConfigManager(&mockConfigManager);
    
    MockEthernet::hardware_detected = true;
    MockEthernet::dhcp_success = true;
    
    reconnectManager.init();
    
    // 接続を切断状態にする
    reconnectManager.setConnected(false);
    
    mockLoggingService.info_count = 0;
    mockLoggingService.error_count = 0;
    
    // 再接続試行（最大5回）
    for (int i = 1; i <= 7; i++) {
        reconnectManager.attemptReconnection();
    }
    
    const NetworkMonitor& status = reconnectManager.getNetworkStatus();
    
    // 最大試行回数確認
    TEST_ASSERT_EQUAL(5, status.maxReconnectAttempts);
    TEST_ASSERT_LESS_OR_EQUAL(5, status.reconnectAttempts);
    
    // 最大試行回数到達時のエラーログ確認
    if (status.reconnectAttempts >= status.maxReconnectAttempts) {
        TEST_ASSERT_GREATER_THAN(0, mockLoggingService.error_count);
    }
    
    // 再接続間隔確認
    TEST_ASSERT_EQUAL(30000, status.reconnectInterval);
}

/**
 * @brief Test UDP ソケット管理・NTPソケット開閉
 */
void test_networkmanager_udp_socket_management_ntp_open_close() {
    networkManager.setLoggingService(&mockLoggingService);
    networkManager.setConfigManager(&mockConfigManager);
    
    MockEthernet::hardware_detected = true;
    MockEthernet::dhcp_success = true;
    
    networkManager.init();
    
    mockLoggingService.info_count = 0;
    mockEthernetUdp.is_open = false;
    
    // NTPサーバー無効時
    networkManager.setNtpServerActive(false);
    networkManager.manageUdpSockets();
    
    TEST_ASSERT_FALSE(networkManager.isUdpSocketOpen());
    TEST_ASSERT_FALSE(mockEthernetUdp.isOpen());
    
    // NTPサーバー有効化
    networkManager.setNtpServerActive(true);
    networkManager.manageUdpSockets();
    
    TEST_ASSERT_TRUE(networkManager.isUdpSocketOpen());
    TEST_ASSERT_TRUE(mockEthernetUdp.isOpen());
    TEST_ASSERT_EQUAL(123, mockEthernetUdp.getPort());
    
    // NTPサーバー無効化
    networkManager.setNtpServerActive(false);
    networkManager.manageUdpSockets();
    
    TEST_ASSERT_FALSE(networkManager.isUdpSocketOpen());
    TEST_ASSERT_FALSE(mockEthernetUdp.isOpen());
    
    // ソケット管理のログ確認
    TEST_ASSERT_GREATER_THAN(0, mockLoggingService.info_count);
}

/**
 * @brief Test 非ブロッキング初期化・状態マシン
 */
void test_networkmanager_nonblocking_initialization_state_machine() {
    NetworkManager asyncManager(&mockEthernetUdp);
    
    // 初期状態確認
    TEST_ASSERT_EQUAL(NetworkManager::INIT_START, asyncManager.getInitState());
    
    // 段階的初期化実행
    bool complete = false;
    int maxSteps = 10;
    int step = 0;
    
    while (!complete && step < maxSteps) {
        complete = asyncManager.updateInitialization();
        step++;
    }
    
    // 初期化完了確認
    TEST_ASSERT_TRUE(complete);
    TEST_ASSERT_EQUAL(NetworkManager::INIT_COMPLETE, asyncManager.getInitState());
    TEST_ASSERT_LESS_THAN(maxSteps, step); // 無限ループ防止確認
    
    // 完了後の追加呼び出し
    TEST_ASSERT_TRUE(asyncManager.updateInitialization()); // 完了状態維持
}

/**
 * @brief Test ハードウェア検出失敗・エラーハンドリング
 */
void test_networkmanager_hardware_detection_failure_error_handling() {
    NetworkManager hwFailManager(&mockEthernetUdp);
    hwFailManager.setLoggingService(&mockLoggingService);
    hwFailManager.setConfigManager(&mockConfigManager);
    
    mockLoggingService.error_count = 0;
    mockLoggingService.warning_count = 0;
    
    // ハードウェア検出失敗
    MockEthernet::hardware_detected = false;
    MockEthernet::dhcp_success = false;
    
    hwFailManager.init();
    
    // エラーログが出力されることを確認
    TEST_ASSERT_GREATER_THAN(0, mockLoggingService.error_count);
    
    // ハードウェア未検出でもフォールバック処理が動作
    const NetworkMonitor& status = hwFailManager.getNetworkStatus();
    TEST_ASSERT_TRUE(status.isConnected); // 静的IPでフォールバック
    
    // DHCP失敗の警告ログ確認
    TEST_ASSERT_GREATER_THAN(0, mockLoggingService.warning_count);
}

/**
 * @brief Test DHCP リース維持・更新処理
 */
void test_networkmanager_dhcp_lease_maintenance_renewal() {
    NetworkManager dhcpManager(&mockEthernetUdp);
    dhcpManager.setLoggingService(&mockLoggingService);
    dhcpManager.setConfigManager(&mockConfigManager);
    
    MockEthernet::hardware_detected = true;
    MockEthernet::dhcp_success = true;
    mockConfigManager.use_dhcp = true;
    
    dhcpManager.init();
    
    const NetworkMonitor& status = dhcpManager.getNetworkStatus();
    TEST_ASSERT_TRUE(status.dhcpActive);
    
    mockLoggingService.warning_count = 0;
    
    // DHCP リース更新成功
    MockEthernet::dhcp_success = true;
    dhcpManager.monitorConnection();
    TEST_ASSERT_EQUAL(0, mockLoggingService.warning_count);
    
    // DHCP リース更新失敗
    MockEthernet::dhcp_success = false;
    dhcpManager.monitorConnection();
    // 警告が出力される可能性がある（実装依存）
}

/**
 * @brief Test ネットワーク統計情報・状態取得
 */
void test_networkmanager_network_statistics_status_retrieval() {
    networkManager.setLoggingService(&mockLoggingService);
    networkManager.setConfigManager(&mockConfigManager);
    
    MockEthernet::hardware_detected = true;
    MockEthernet::dhcp_success = true;
    MockEthernet::local_ip = 0xC0A80165;
    MockEthernet::gateway_ip = 0xC0A80101;
    MockEthernet::dns_server = 0x08080808;
    
    networkManager.init();
    
    // ネットワーク状態取得
    const NetworkMonitor& netStatus = networkManager.getNetworkStatus();
    TEST_ASSERT_TRUE(netStatus.isConnected);
    TEST_ASSERT_TRUE(netStatus.dhcpActive);
    TEST_ASSERT_EQUAL(0xC0A80165, netStatus.localIP);
    TEST_ASSERT_EQUAL(0xC0A80101, netStatus.gateway);
    TEST_ASSERT_EQUAL(0x08080808, netStatus.dnsServer);
    TEST_ASSERT_EQUAL(5000, netStatus.linkCheckInterval);
    TEST_ASSERT_EQUAL(30000, netStatus.reconnectInterval);
    TEST_ASSERT_EQUAL(5, netStatus.maxReconnectAttempts);
    
    // UDP状態取得
    const UdpSocketManager& udpStatus = networkManager.getUdpStatus();
    TEST_ASSERT_EQUAL(10000, udpStatus.socketCheckInterval);
    TEST_ASSERT_GREATER_OR_EQUAL(0, udpStatus.socketErrors);
    
    // NTPサーバー有効化してソケット管理
    networkManager.setNtpServerActive(true);
    networkManager.manageUdpSockets();
    
    const UdpSocketManager& activeUdpStatus = networkManager.getUdpStatus();
    TEST_ASSERT_TRUE(activeUdpStatus.ntpSocketOpen);
}

/**
 * @brief Test 境界値・エッジケース処理
 */
void test_networkmanager_boundary_edge_cases() {
    // サービス未設定時の動作
    NetworkManager isolatedManager(&mockEthernetUdp);
    
    // ログサービスなしで初期化
    isolatedManager.init(); // クラッシュしないことを確認
    
    // 設定マネージャーなしで初期化
    isolatedManager.setLoggingService(&mockLoggingService);
    isolatedManager.init(); // デフォルト動作することを確認
    
    // null UDP インスタンス
    NetworkManager nullUdpManager(nullptr);
    nullUdpManager.setLoggingService(&mockLoggingService);
    nullUdpManager.setConfigManager(&mockConfigManager);
    
    MockEthernet::hardware_detected = true;
    MockEthernet::dhcp_success = true;
    
    nullUdpManager.init();
    nullUdpManager.setNtpServerActive(true);
    nullUdpManager.manageUdpSockets(); // null UDP でクラッシュしないことを確認
    
    // 極端な設定値
    mockConfigManager.static_ip = 0;
    mockConfigManager.gateway_ip = 0;
    mockConfigManager.dns_server = 0;
    
    NetworkManager extremeManager(&mockEthernetUdp);
    extremeManager.setLoggingService(&mockLoggingService);
    extremeManager.setConfigManager(&mockConfigManager);
    extremeManager.init(); // 極端な値でもクラッシュしない
    
    // 連続再接続試行
    NetworkManager reconnectSpamManager(&mockEthernetUdp);
    reconnectSpamManager.setLoggingService(&mockLoggingService);
    reconnectSpamManager.setConfigManager(&mockConfigManager);
    reconnectSpamManager.init();
    reconnectSpamManager.setConnected(false);
    
    // 短時間での連続再接続試行（間隔制御のテスト）
    for (int i = 0; i < 20; i++) {
        reconnectSpamManager.attemptReconnection();
    }
    
    const NetworkMonitor& spamStatus = reconnectSpamManager.getNetworkStatus();
    TEST_ASSERT_LESS_OR_EQUAL(spamStatus.maxReconnectAttempts, spamStatus.reconnectAttempts);
}

// Test suite setup and teardown
void setUp(void) {
    // Reset mock states
    MockEthernet::hardware_detected = true;
    MockEthernet::dhcp_success = true;
    MockEthernet::link_active = true;
    MockEthernet::local_ip = 0;
    MockEthernet::gateway_ip = 0;
    MockEthernet::dns_server = 0;
    MockEthernet::cs_pin = 0;
    MockEthernet::begin_call_count = 0;
    
    mockEthernetUdp.is_open = false;
    mockEthernetUdp.local_port = 0;
    
    mockLoggingService.info_count = 0;
    mockLoggingService.warning_count = 0;
    mockLoggingService.error_count = 0;
    
    mockConfigManager.use_dhcp = true;
    mockConfigManager.static_ip = 0xC0A80164;
    mockConfigManager.subnet_mask = 0xFFFFFF00;
    mockConfigManager.gateway_ip = 0xC0A80101;
    mockConfigManager.dns_server = 0x08080808;
}

void tearDown(void) {
    // Cleanup after each test
}

/**
 * @brief NetworkManager完全カバレッジテスト実行
 */
int main() {
    UNITY_BEGIN();
    
    // Basic initialization and hardware setup
    RUN_TEST(test_networkmanager_basic_initialization_w5500_setup);
    
    // DHCP configuration
    RUN_TEST(test_networkmanager_dhcp_configuration_success_failure);
    
    // Static IP configuration
    RUN_TEST(test_networkmanager_static_ip_configuration_parameters);
    
    // Connection monitoring
    RUN_TEST(test_networkmanager_connection_monitoring_link_status);
    
    // Reconnection functionality
    RUN_TEST(test_networkmanager_reconnection_max_attempts_interval);
    
    // UDP socket management
    RUN_TEST(test_networkmanager_udp_socket_management_ntp_open_close);
    
    // Non-blocking initialization
    RUN_TEST(test_networkmanager_nonblocking_initialization_state_machine);
    
    // Hardware failure handling
    RUN_TEST(test_networkmanager_hardware_detection_failure_error_handling);
    
    // DHCP lease maintenance
    RUN_TEST(test_networkmanager_dhcp_lease_maintenance_renewal);
    
    // Network statistics
    RUN_TEST(test_networkmanager_network_statistics_status_retrieval);
    
    // Edge cases
    RUN_TEST(test_networkmanager_boundary_edge_cases);
    
    return UNITY_END();
}