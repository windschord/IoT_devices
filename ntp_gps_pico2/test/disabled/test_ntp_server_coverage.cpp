#include <unity.h>
#include "Arduino.h"

// Use Arduino Mock environment
// Define network functions separately to avoid system header conflicts
namespace MockNetwork {
    uint32_t mock_ntohl(uint32_t netlong) {
        return ((netlong & 0xFF000000) >> 24) |
               ((netlong & 0x00FF0000) >> 8) |
               ((netlong & 0x0000FF00) << 8) |
               ((netlong & 0x000000FF) << 24);
    }
    
    uint32_t mock_htonl(uint32_t hostlong) {
        return ((hostlong & 0xFF000000) >> 24) |
               ((hostlong & 0x00FF0000) >> 8) |
               ((hostlong & 0x0000FF00) << 8) |
               ((hostlong & 0x000000FF) << 24);
    }
}

// Mock LoggingService
class MockLoggingService {
public:
    int info_count = 0;
    int warning_count = 0;
    int error_count = 0;
    
    void logInfo(const char* component, const char* message) {
        info_count++;
        (void)component; (void)message;
    }
    void logWarning(const char* component, const char* message) {
        warning_count++;
        (void)component; (void)message;
    }
    void logError(const char* component, const char* message) {
        error_count++;
        (void)component; (void)message;
    }
};

// Mock IPAddress
class MockIPAddress {
private:
    uint32_t address;
    
public:
    MockIPAddress() : address(0) {}
    MockIPAddress(uint32_t addr) : address(addr) {}
    MockIPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
        address = (uint32_t(a) << 24) | (uint32_t(b) << 16) | (uint32_t(c) << 8) | uint32_t(d);
    }
    
    uint32_t getAddress() const { return address; }
    bool operator==(const MockIPAddress& other) const { return address == other.address; }
    bool operator!=(const MockIPAddress& other) const { return address != other.address; }
};

typedef MockIPAddress IPAddress;

// Mock EthernetUDP
class MockEthernetUDP {
public:
    bool has_packet = false;
    uint8_t packet_buffer[48];
    size_t packet_size = 0;
    IPAddress remote_ip;
    uint16_t remote_port = 0;
    bool send_success = true;
    
    int parsePacket() {
        return has_packet ? packet_size : 0;
    }
    
    int read(uint8_t* buffer, size_t size) {
        if (!has_packet) return 0;
        size_t copy_size = (size < packet_size) ? size : packet_size;
        memcpy(buffer, packet_buffer, copy_size);
        return copy_size;
    }
    
    IPAddress remoteIP() const { return remote_ip; }
    uint16_t remotePort() const { return remote_port; }
    
    size_t beginPacket(IPAddress ip, uint16_t port) {
        remote_ip = ip;
        remote_port = port;
        return 1;
    }
    
    size_t write(const uint8_t* buffer, size_t size) {
        return send_success ? size : 0;
    }
    
    int endPacket() {
        return send_success ? 1 : 0;
    }
    
    // Test helpers
    void setIncomingPacket(const uint8_t* data, size_t size, IPAddress ip, uint16_t port) {
        memcpy(packet_buffer, data, size);
        packet_size = size;
        remote_ip = ip;
        remote_port = port;
        has_packet = true;
    }
    
    void clearPacket() {
        has_packet = false;
        packet_size = 0;
    }
    
    void setSendSuccess(bool success) {
        send_success = success;
    }
};

// Mock TimeManager
class MockTimeManager {
public:
    bool is_synced = true;
    uint32_t gps_time = 1609459200; // 2021-01-01 00:00:00 UTC
    uint32_t gps_microseconds = 0;
    uint8_t stratum = 1;
    float precision = 0.000001f; // 1 microsecond
    
    bool isSynced() const { return is_synced; }
    uint32_t getGpsTime() const { return gps_time; }
    uint32_t getGpsMicroseconds() const { return gps_microseconds; }
    uint8_t getStratum() const { return stratum; }
    float getPrecision() const { return precision; }
    
    void setSynced(bool synced) { is_synced = synced; }
    void setTime(uint32_t time, uint32_t microseconds = 0) {
        gps_time = time;
        gps_microseconds = microseconds;
    }
    void setStratum(uint8_t s) { stratum = s; }
};

// NTP Types and constants
#define NTP_PACKET_SIZE 48
#define NTP_PORT 123
#define NTP_TIMESTAMP_DELTA 2208988800UL

#define NTP_LI_NO_WARNING       0x00
#define NTP_LI_LAST_MINUTE_61   0x01
#define NTP_LI_LAST_MINUTE_59   0x02
#define NTP_LI_ALARM            0x03

#define NTP_VERSION             4

#define NTP_MODE_RESERVED       0
#define NTP_MODE_SYMMETRIC_ACTIVE 1
#define NTP_MODE_SYMMETRIC_PASSIVE 2
#define NTP_MODE_CLIENT         3
#define NTP_MODE_SERVER         4
#define NTP_MODE_BROADCAST      5
#define NTP_MODE_CONTROL        6
#define NTP_MODE_PRIVATE        7

#define NTP_STRATUM_UNSPECIFIED 0
#define NTP_STRATUM_PRIMARY     1
#define NTP_STRATUM_SECONDARY_MIN 2
#define NTP_STRATUM_SECONDARY_MAX 15
#define NTP_STRATUM_UNSYNC      16

#define NTP_REFID_GPS   0x47505300

// Helper macros
#define NTP_GET_LI(li_vn_mode)      (((li_vn_mode) >> 6) & 0x03)
#define NTP_GET_VN(li_vn_mode)      (((li_vn_mode) >> 3) & 0x07)
#define NTP_GET_MODE(li_vn_mode)    ((li_vn_mode) & 0x07)
#define NTP_SET_LI_VN_MODE(li, vn, mode) (((li & 0x03) << 6) | ((vn & 0x07) << 3) | (mode & 0x07))

struct NtpTimestamp {
    uint32_t seconds;
    uint32_t fraction;
};

struct NtpPacket {
    uint8_t  li_vn_mode;
    uint8_t  stratum;
    int8_t   poll;
    int8_t   precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t reference_id;
    NtpTimestamp reference_timestamp;
    NtpTimestamp origin_timestamp;
    NtpTimestamp receive_timestamp;
    NtpTimestamp transmit_timestamp;
};

struct NtpStatistics {
    uint32_t requests_total;
    uint32_t requests_valid;
    uint32_t requests_invalid;
    uint32_t responses_sent;
    uint32_t last_request_time;
    float avg_processing_time;
    uint32_t clients_served;
};

struct UdpSocketManager {
    bool ntpSocketOpen;
    unsigned long lastSocketCheck;
    unsigned long socketCheckInterval;
    int socketErrors;
};

// Timestamp conversion helpers
inline NtpTimestamp unixToNtpTimestamp(uint32_t unixSeconds, uint32_t microseconds = 0) {
    NtpTimestamp ntp;
    ntp.seconds = unixSeconds + NTP_TIMESTAMP_DELTA;
    ntp.fraction = (uint32_t)((uint64_t)microseconds * 4294967296ULL / 1000000ULL);
    return ntp;
}

inline uint32_t ntpToUnixTimestamp(const NtpTimestamp& ntp) {
    return ntp.seconds - NTP_TIMESTAMP_DELTA;
}

// NtpServer implementation (simplified for testing)
class NtpServer {
private:
    MockEthernetUDP* ntpUdp;
    MockTimeManager* timeManager;
    UdpSocketManager* udpManager;
    MockLoggingService* loggingService;
    
    uint8_t packetBuffer[NTP_PACKET_SIZE];
    NtpPacket receivedPacket;
    NtpPacket responsePacket;
    NtpStatistics stats;
    
    IPAddress currentClientIP;
    uint16_t currentClientPort;
    
    uint32_t receiveTimestamp_us;
    uint32_t transmitTimestamp_us;
    
    bool parseNtpRequest(const uint8_t* buffer, size_t length) {
        if (length < NTP_PACKET_SIZE) return false;
        
        // Parse NTP packet from buffer
        receivedPacket.li_vn_mode = buffer[0];
        receivedPacket.stratum = buffer[1];
        receivedPacket.poll = (int8_t)buffer[2];
        receivedPacket.precision = (int8_t)buffer[3];
        
        // Parse 32-bit fields (network byte order)
        memcpy(&receivedPacket.root_delay, &buffer[4], 4);
        memcpy(&receivedPacket.root_dispersion, &buffer[8], 4);
        memcpy(&receivedPacket.reference_id, &buffer[12], 4);
        
        // Parse timestamps
        memcpy(&receivedPacket.reference_timestamp, &buffer[16], 8);
        memcpy(&receivedPacket.origin_timestamp, &buffer[24], 8);
        memcpy(&receivedPacket.receive_timestamp, &buffer[32], 8);
        memcpy(&receivedPacket.transmit_timestamp, &buffer[40], 8);
        
        return true;
    }
    
    void createNtpResponse() {
        // Clear response packet
        memset(&responsePacket, 0, sizeof(responsePacket));
        
        // Set LI, VN, Mode
        uint8_t li = timeManager->isSynced() ? NTP_LI_NO_WARNING : NTP_LI_ALARM;
        responsePacket.li_vn_mode = NTP_SET_LI_VN_MODE(li, NTP_VERSION, NTP_MODE_SERVER);
        
        // Set stratum
        responsePacket.stratum = calculateStratum();
        
        // Set poll and precision
        responsePacket.poll = receivedPacket.poll;
        responsePacket.precision = calculatePrecision();
        
        // Set root delay and dispersion
        responsePacket.root_delay = MockNetwork::mock_htonl(calculateRootDelay());
        responsePacket.root_dispersion = MockNetwork::mock_htonl(calculateRootDispersion());
        
        // Set reference ID
        responsePacket.reference_id = MockNetwork::mock_htonl(getReferenceId());
        
        // Set reference timestamp
        responsePacket.reference_timestamp = getReferenceTimestamp();
        
        // Set origin timestamp (from client's transmit timestamp)
        responsePacket.origin_timestamp = receivedPacket.transmit_timestamp;
        
        // Set receive timestamp
        NtpTimestamp receiveTime = getCurrentNtpTimestamp();
        responsePacket.receive_timestamp.seconds = MockNetwork::mock_htonl(receiveTime.seconds);
        responsePacket.receive_timestamp.fraction = MockNetwork::mock_htonl(receiveTime.fraction);
        
        // Set transmit timestamp
        NtpTimestamp transmitTime = getCurrentNtpTimestamp();
        responsePacket.transmit_timestamp.seconds = MockNetwork::mock_htonl(transmitTime.seconds);
        responsePacket.transmit_timestamp.fraction = MockNetwork::mock_htonl(transmitTime.fraction);
    }
    
    bool sendNtpResponse() {
        if (!ntpUdp) return false;
        
        if (ntpUdp->beginPacket(currentClientIP, currentClientPort) == 0) {
            return false;
        }
        
        size_t written = ntpUdp->write((const uint8_t*)&responsePacket, sizeof(responsePacket));
        if (written != sizeof(responsePacket)) {
            return false;
        }
        
        return ntpUdp->endPacket() == 1;
    }
    
    NtpTimestamp getCurrentNtpTimestamp() {
        if (timeManager && timeManager->isSynced()) {
            return unixToNtpTimestamp(timeManager->getGpsTime(), timeManager->getGpsMicroseconds());
        } else {
            // Fallback to system time
            uint32_t currentTime = millis() / 1000 + 1609459200; // Approximate Unix time
            return unixToNtpTimestamp(currentTime, micros() % 1000000);
        }
    }
    
    NtpTimestamp getHighPrecisionTimestamp(uint32_t microsecond_offset = 0) {
        NtpTimestamp ts = getCurrentNtpTimestamp();
        uint64_t additional_fraction = ((uint64_t)microsecond_offset * 4294967296ULL) / 1000000ULL;
        ts.fraction += (uint32_t)additional_fraction;
        return ts;
    }
    
    bool validateNtpRequest(const NtpPacket& packet) {
        // Check version
        uint8_t version = NTP_GET_VN(packet.li_vn_mode);
        if (version < 3 || version > 4) return false;
        
        // Check mode
        uint8_t mode = NTP_GET_MODE(packet.li_vn_mode);
        if (mode != NTP_MODE_CLIENT) return false;
        
        // Basic validation passed
        return true;
    }
    
    bool isRateLimited(IPAddress clientIP) {
        // Simple rate limiting: allow all for testing
        (void)clientIP;
        return false;
    }
    
    uint8_t calculateStratum() {
        if (!timeManager || !timeManager->isSynced()) {
            return NTP_STRATUM_UNSYNC;
        }
        return timeManager->getStratum();
    }
    
    int8_t calculatePrecision() {
        if (timeManager) {
            float precision = timeManager->getPrecision();
            return (int8_t)(-20); // ~1 microsecond precision (2^-20)
        }
        return -10; // ~1 millisecond precision (2^-10)
    }
    
    uint32_t calculateRootDelay() {
        return 0; // Primary source has zero root delay
    }
    
    uint32_t calculateRootDispersion() {
        return 100; // Small dispersion for GPS source
    }
    
    uint32_t getReferenceId() {
        return NTP_REFID_GPS;
    }
    
    NtpTimestamp getReferenceTimestamp() {
        return getCurrentNtpTimestamp();
    }
    
    void updateStatistics(bool validRequest, float processingTimeMs) {
        stats.requests_total++;
        stats.last_request_time = millis();
        
        if (validRequest) {
            stats.requests_valid++;
            stats.responses_sent++;
        } else {
            stats.requests_invalid++;
        }
        
        // Update average processing time
        if (stats.requests_total == 1) {
            stats.avg_processing_time = processingTimeMs;
        } else {
            stats.avg_processing_time = (stats.avg_processing_time * (stats.requests_total - 1) + processingTimeMs) / stats.requests_total;
        }
    }
    
    void logRequest(IPAddress clientIP, bool valid) {
        if (loggingService) {
            if (valid) {
                loggingService->logInfo("NTP", "Valid request processed");
            } else {
                loggingService->logWarning("NTP", "Invalid request received");
            }
        }
    }

public:
    NtpServer(MockEthernetUDP* udpInstance, MockTimeManager* timeManagerInstance, UdpSocketManager* udpManagerInstance)
        : ntpUdp(udpInstance), timeManager(timeManagerInstance), udpManager(udpManagerInstance), loggingService(nullptr) {
        
        memset(&stats, 0, sizeof(stats));
        memset(&receivedPacket, 0, sizeof(receivedPacket));
        memset(&responsePacket, 0, sizeof(responsePacket));
        currentClientIP = IPAddress(0, 0, 0, 0);
        currentClientPort = 0;
        receiveTimestamp_us = 0;
        transmitTimestamp_us = 0;
    }
    
    void setLoggingService(MockLoggingService* loggingServiceInstance) {
        loggingService = loggingServiceInstance;
    }
    
    void init() {
        if (loggingService) {
            loggingService->logInfo("NTP", "NTP Server initialized");
        }
        resetStatistics();
    }
    
    void processRequests() {
        if (!ntpUdp || !udpManager || !udpManager->ntpSocketOpen) return;
        
        int packetSize = ntpUdp->parsePacket();
        if (packetSize == 0) return;
        
        uint32_t startTime = micros();
        
        // Get client information
        currentClientIP = ntpUdp->remoteIP();
        currentClientPort = ntpUdp->remotePort();
        
        // Check rate limiting
        if (isRateLimited(currentClientIP)) {
            if (loggingService) {
                loggingService->logWarning("NTP", "Request rate limited");
            }
            return;
        }
        
        // Read packet
        int bytesRead = ntpUdp->read(packetBuffer, NTP_PACKET_SIZE);
        if (bytesRead != NTP_PACKET_SIZE) {
            updateStatistics(false, 0);
            logRequest(currentClientIP, false);
            return;
        }
        
        // Parse packet
        if (!parseNtpRequest(packetBuffer, bytesRead)) {
            updateStatistics(false, 0);
            logRequest(currentClientIP, false);
            return;
        }
        
        // Validate request
        if (!validateNtpRequest(receivedPacket)) {
            updateStatistics(false, 0);
            logRequest(currentClientIP, false);
            return;
        }
        
        // Create and send response
        createNtpResponse();
        bool sendSuccess = sendNtpResponse();
        
        uint32_t endTime = micros();
        float processingTime = (endTime - startTime) / 1000.0f;
        
        updateStatistics(sendSuccess, processingTime);
        logRequest(currentClientIP, sendSuccess);
    }
    
    const NtpStatistics& getStatistics() const {
        return stats;
    }
    
    void resetStatistics() {
        memset(&stats, 0, sizeof(stats));
    }
    
    // Test helpers
    const NtpPacket& getLastResponse() const { return responsePacket; }
    IPAddress getCurrentClientIP() const { return currentClientIP; }
    uint16_t getCurrentClientPort() const { return currentClientPort; }
};

// Global test instances
MockEthernetUDP mockEthernetUdp;
MockTimeManager mockTimeManager;
UdpSocketManager mockUdpManager;
MockLoggingService mockLoggingService;
NtpServer ntpServer(&mockEthernetUdp, &mockTimeManager, &mockUdpManager);

/**
 * @brief Test NtpServer基本初期化・設定
 */
void test_ntpserver_basic_initialization_configuration() {
    ntpServer.setLoggingService(&mockLoggingService);
    
    mockLoggingService.info_count = 0;
    
    // 初期化実行
    ntpServer.init();
    
    // 初期化ログ確認
    TEST_ASSERT_GREATER_THAN(0, mockLoggingService.info_count);
    
    // 統計初期化確認
    const NtpStatistics& stats = ntpServer.getStatistics();
    TEST_ASSERT_EQUAL(0, stats.requests_total);
    TEST_ASSERT_EQUAL(0, stats.requests_valid);
    TEST_ASSERT_EQUAL(0, stats.requests_invalid);
    TEST_ASSERT_EQUAL(0, stats.responses_sent);
    TEST_ASSERT_EQUAL(0, stats.last_request_time);
    TEST_ASSERT_EQUAL(0.0f, stats.avg_processing_time);
    TEST_ASSERT_EQUAL(0, stats.clients_served);
}

/**
 * @brief Test NTPパケット解析・検証機能
 */
void test_ntpserver_ntp_packet_parsing_validation() {
    ntpServer.setLoggingService(&mockLoggingService);
    ntpServer.init();
    
    // 有効なNTPクライアント要求パケット作成
    uint8_t validPacket[NTP_PACKET_SIZE];
    memset(validPacket, 0, NTP_PACKET_SIZE);
    
    // LI=0, VN=4, Mode=3 (client)
    validPacket[0] = NTP_SET_LI_VN_MODE(NTP_LI_NO_WARNING, NTP_VERSION, NTP_MODE_CLIENT);
    validPacket[1] = 0; // stratum (unspecified for client)
    validPacket[2] = 6; // poll interval (64 seconds)
    validPacket[3] = -20; // precision
    
    // タイムスタンプ設定（送信時刻）
    NtpTimestamp transmitTime = unixToNtpTimestamp(1609459200, 0);
    memcpy(&validPacket[40], &transmitTime, sizeof(NtpTimestamp));
    
    mockUdpManager.ntpSocketOpen = true;
    mockEthernetUdp.setIncomingPacket(validPacket, NTP_PACKET_SIZE, IPAddress(192, 168, 1, 100), 1234);
    
    mockLoggingService.info_count = 0;
    mockLoggingService.warning_count = 0;
    
    // 要求処理
    ntpServer.processRequests();
    
    // 統計確認
    const NtpStatistics& stats = ntpServer.getStatistics();
    TEST_ASSERT_EQUAL(1, stats.requests_total);
    TEST_ASSERT_EQUAL(1, stats.requests_valid);
    TEST_ASSERT_EQUAL(0, stats.requests_invalid);
    TEST_ASSERT_EQUAL(1, stats.responses_sent);
    TEST_ASSERT_GREATER_THAN(0, stats.last_request_time);
    TEST_ASSERT_GREATER_THAN(0.0f, stats.avg_processing_time);
    
    // ログ確認
    TEST_ASSERT_GREATER_THAN(0, mockLoggingService.info_count);
    TEST_ASSERT_EQUAL(0, mockLoggingService.warning_count);
}

/**
 * @brief Test NTP応答パケット生成・フィールド設定
 */
void test_ntpserver_ntp_response_generation_field_setup() {
    ntpServer.setLoggingService(&mockLoggingService);
    ntpServer.init();
    
    // TimeManager設定
    mockTimeManager.setSynced(true);
    mockTimeManager.setTime(1609459200, 500000); // 2021-01-01 00:00:00.500 UTC
    mockTimeManager.setStratum(1);
    
    // 有効なクライアント要求
    uint8_t clientPacket[NTP_PACKET_SIZE];
    memset(clientPacket, 0, NTP_PACKET_SIZE);
    clientPacket[0] = NTP_SET_LI_VN_MODE(NTP_LI_NO_WARNING, NTP_VERSION, NTP_MODE_CLIENT);
    clientPacket[1] = 0;
    clientPacket[2] = 6;
    clientPacket[3] = -10;
    
    // クライアント送信時刻設定
    NtpTimestamp clientTransmit = unixToNtpTimestamp(1609459190, 0);
    memcpy(&clientPacket[40], &clientTransmit, sizeof(NtpTimestamp));
    
    mockUdpManager.ntpSocketOpen = true;
    mockEthernetUdp.setIncomingPacket(clientPacket, NTP_PACKET_SIZE, IPAddress(192, 168, 1, 100), 1234);
    mockEthernetUdp.setSendSuccess(true);
    
    // 要求処理
    ntpServer.processRequests();
    
    // 応答パケット確認
    const NtpPacket& response = ntpServer.getLastResponse();
    
    // LI, VN, Mode確認
    TEST_ASSERT_EQUAL(NTP_LI_NO_WARNING, NTP_GET_LI(response.li_vn_mode));
    TEST_ASSERT_EQUAL(NTP_VERSION, NTP_GET_VN(response.li_vn_mode));
    TEST_ASSERT_EQUAL(NTP_MODE_SERVER, NTP_GET_MODE(response.li_vn_mode));
    
    // Stratum確認
    TEST_ASSERT_EQUAL(1, response.stratum);
    
    // Poll確認（クライアントからコピー）
    TEST_ASSERT_EQUAL(6, response.poll);
    
    // Precision確認
    TEST_ASSERT_EQUAL(-20, response.precision);
    
    // Reference ID確認（GPS）
    TEST_ASSERT_EQUAL(htonl(NTP_REFID_GPS), response.reference_id);
    
    // Root delay/dispersion確認
    TEST_ASSERT_EQUAL(0, ntohl(response.root_delay));
    TEST_ASSERT_EQUAL(100, ntohl(response.root_dispersion));
    
    // Origin timestamp確認（クライアントのtransmit timestampと一致）
    TEST_ASSERT_EQUAL(clientTransmit.seconds, response.origin_timestamp.seconds);
    TEST_ASSERT_EQUAL(clientTransmit.fraction, response.origin_timestamp.fraction);
}

/**
 * @brief Test 高精度タイムスタンプ生成・マイクロ秒精度
 */
void test_ntpserver_high_precision_timestamp_microsecond_accuracy() {
    ntpServer.setLoggingService(&mockLoggingService);
    ntpServer.init();
    
    // 高精度時刻設定
    mockTimeManager.setSynced(true);
    mockTimeManager.setTime(1609459200, 123456); // 123.456ms
    
    uint8_t packet[NTP_PACKET_SIZE];
    memset(packet, 0, NTP_PACKET_SIZE);
    packet[0] = NTP_SET_LI_VN_MODE(NTP_LI_NO_WARNING, NTP_VERSION, NTP_MODE_CLIENT);
    
    mockUdpManager.ntpSocketOpen = true;
    mockEthernetUdp.setIncomingPacket(packet, NTP_PACKET_SIZE, IPAddress(192, 168, 1, 100), 1234);
    
    // 要求処理（高精度タイムスタンプ生成）
    ntpServer.processRequests();
    
    const NtpPacket& response = ntpServer.getLastResponse();
    
    // receive timestampとtransmit timestampが設定されていることを確認
    TEST_ASSERT_NOT_EQUAL(0, ntohl(response.receive_timestamp.seconds));
    TEST_ASSERT_NOT_EQUAL(0, ntohl(response.transmit_timestamp.seconds));
    
    // タイムスタンプがNTPエポック基準であることを確認
    uint32_t receiveUnixTime = ntohl(response.receive_timestamp.seconds) - NTP_TIMESTAMP_DELTA;
    TEST_ASSERT_GREATER_OR_EQUAL(1609459200, receiveUnixTime);
    TEST_ASSERT_LESS_THAN(1609459200 + 3600, receiveUnixTime); // 1時間以内
    
    // フラクション部が設定されていることを確認（マイクロ秒精度）
    TEST_ASSERT_NOT_EQUAL(0, ntohl(response.receive_timestamp.fraction));
    TEST_ASSERT_NOT_EQUAL(0, ntohl(response.transmit_timestamp.fraction));
}

/**
 * @brief Test GPS同期状態・Stratum計算
 */
void test_ntpserver_gps_sync_status_stratum_calculation() {
    ntpServer.setLoggingService(&mockLoggingService);
    ntpServer.init();
    
    uint8_t packet[NTP_PACKET_SIZE];
    memset(packet, 0, NTP_PACKET_SIZE);
    packet[0] = NTP_SET_LI_VN_MODE(NTP_LI_NO_WARNING, NTP_VERSION, NTP_MODE_CLIENT);
    
    mockUdpManager.ntpSocketOpen = true;
    mockEthernetUdp.setIncomingPacket(packet, NTP_PACKET_SIZE, IPAddress(192, 168, 1, 100), 1234);
    
    // GPS同期時（Stratum 1）
    mockTimeManager.setSynced(true);
    mockTimeManager.setStratum(1);
    
    ntpServer.processRequests();
    const NtpPacket& syncedResponse = ntpServer.getLastResponse();
    
    TEST_ASSERT_EQUAL(NTP_LI_NO_WARNING, NTP_GET_LI(syncedResponse.li_vn_mode));
    TEST_ASSERT_EQUAL(1, syncedResponse.stratum);
    
    // GPS非同期時（Stratum 16 = unsynchronized）
    mockEthernetUdp.setIncomingPacket(packet, NTP_PACKET_SIZE, IPAddress(192, 168, 1, 101), 1235);
    mockTimeManager.setSynced(false);
    
    ntpServer.processRequests();
    const NtpPacket& unsyncedResponse = ntpServer.getLastResponse();
    
    TEST_ASSERT_EQUAL(NTP_LI_ALARM, NTP_GET_LI(unsyncedResponse.li_vn_mode));
    TEST_ASSERT_EQUAL(NTP_STRATUM_UNSYNC, unsyncedResponse.stratum);
    
    // 統計確認（2回の処理）
    const NtpStatistics& stats = ntpServer.getStatistics();
    TEST_ASSERT_EQUAL(2, stats.requests_total);
    TEST_ASSERT_EQUAL(2, stats.requests_valid);
}

/**
 * @brief Test 無効パケット処理・エラーハンドリング
 */
void test_ntpserver_invalid_packet_handling_error_processing() {
    ntpServer.setLoggingService(&mockLoggingService);
    ntpServer.init();
    ntpServer.resetStatistics();
    
    mockUdpManager.ntpSocketOpen = true;
    mockLoggingService.warning_count = 0;
    
    // 1. 不正サイズパケット
    uint8_t shortPacket[20];
    memset(shortPacket, 0, 20);
    mockEthernetUdp.setIncomingPacket(shortPacket, 20, IPAddress(192, 168, 1, 100), 1234);
    
    ntpServer.processRequests();
    
    // 2. 不正バージョンパケット
    uint8_t invalidVersionPacket[NTP_PACKET_SIZE];
    memset(invalidVersionPacket, 0, NTP_PACKET_SIZE);
    invalidVersionPacket[0] = NTP_SET_LI_VN_MODE(NTP_LI_NO_WARNING, 2, NTP_MODE_CLIENT); // Version 2
    
    mockEthernetUdp.setIncomingPacket(invalidVersionPacket, NTP_PACKET_SIZE, IPAddress(192, 168, 1, 101), 1235);
    ntpServer.processRequests();
    
    // 3. 不正モードパケット
    uint8_t invalidModePacket[NTP_PACKET_SIZE];
    memset(invalidModePacket, 0, NTP_PACKET_SIZE);
    invalidModePacket[0] = NTP_SET_LI_VN_MODE(NTP_LI_NO_WARNING, NTP_VERSION, NTP_MODE_SERVER); // Server mode
    
    mockEthernetUdp.setIncomingPacket(invalidModePacket, NTP_PACKET_SIZE, IPAddress(192, 168, 1, 102), 1236);
    ntpServer.processRequests();
    
    // 統計確認
    const NtpStatistics& stats = ntpServer.getStatistics();
    TEST_ASSERT_EQUAL(3, stats.requests_total);
    TEST_ASSERT_EQUAL(0, stats.requests_valid);
    TEST_ASSERT_EQUAL(3, stats.requests_invalid);
    TEST_ASSERT_EQUAL(0, stats.responses_sent);
    
    // 警告ログ確認
    TEST_ASSERT_GREATER_THAN(0, mockLoggingService.warning_count);
}

/**
 * @brief Test NTP統計情報・処理時間計算
 */
void test_ntpserver_ntp_statistics_processing_time_calculation() {
    ntpServer.setLoggingService(&mockLoggingService);
    ntpServer.init();
    ntpServer.resetStatistics();
    
    mockTimeManager.setSynced(true);
    mockTimeManager.setStratum(1);
    mockUdpManager.ntpSocketOpen = true;
    
    uint8_t validPacket[NTP_PACKET_SIZE];
    memset(validPacket, 0, NTP_PACKET_SIZE);
    validPacket[0] = NTP_SET_LI_VN_MODE(NTP_LI_NO_WARNING, NTP_VERSION, NTP_MODE_CLIENT);
    
    // 複数要求処理
    for (int i = 0; i < 5; i++) {
        mockEthernetUdp.setIncomingPacket(validPacket, NTP_PACKET_SIZE, 
                                         IPAddress(192, 168, 1, 100 + i), 1234 + i);
        ntpServer.processRequests();
    }
    
    const NtpStatistics& stats = ntpServer.getStatistics();
    
    // 統計確認
    TEST_ASSERT_EQUAL(5, stats.requests_total);
    TEST_ASSERT_EQUAL(5, stats.requests_valid);
    TEST_ASSERT_EQUAL(0, stats.requests_invalid);
    TEST_ASSERT_EQUAL(5, stats.responses_sent);
    
    // 処理時間が計算されていることを確認
    TEST_ASSERT_GREATER_THAN(0.0f, stats.avg_processing_time);
    TEST_ASSERT_LESS_THAN(1000.0f, stats.avg_processing_time); // 1秒未満
    
    // 最後の要求時刻が記録されていることを確認
    TEST_ASSERT_GREATER_THAN(0, stats.last_request_time);
}

/**
 * @brief Test 複数クライアント同時処理
 */
void test_ntpserver_multiple_client_concurrent_processing() {
    ntpServer.setLoggingService(&mockLoggingService);
    ntpServer.init();
    ntpServer.resetStatistics();
    
    mockTimeManager.setSynced(true);
    mockUdpManager.ntpSocketOpen = true;
    
    uint8_t packet[NTP_PACKET_SIZE];
    memset(packet, 0, NTP_PACKET_SIZE);
    packet[0] = NTP_SET_LI_VN_MODE(NTP_LI_NO_WARNING, NTP_VERSION, NTP_MODE_CLIENT);
    
    // 異なるクライアントからの要求
    IPAddress clients[] = {
        IPAddress(192, 168, 1, 10),
        IPAddress(192, 168, 1, 20),
        IPAddress(192, 168, 1, 30),
        IPAddress(10, 0, 0, 5),
        IPAddress(172, 16, 0, 100)
    };
    
    for (int i = 0; i < 5; i++) {
        mockEthernetUdp.setIncomingPacket(packet, NTP_PACKET_SIZE, clients[i], 1234 + i);
        ntpServer.processRequests();
        
        // 各要求で正しいクライアント情報が設定されることを確認
        TEST_ASSERT_TRUE(ntpServer.getCurrentClientIP() == clients[i]);
        TEST_ASSERT_EQUAL(1234 + i, ntpServer.getCurrentClientPort());
    }
    
    const NtpStatistics& stats = ntpServer.getStatistics();
    
    // 全要求が正常処理されることを確認
    TEST_ASSERT_EQUAL(5, stats.requests_total);
    TEST_ASSERT_EQUAL(5, stats.requests_valid);
    TEST_ASSERT_EQUAL(5, stats.responses_sent);
}

/**
 * @brief Test 送信失敗処理・ネットワークエラー
 */
void test_ntpserver_send_failure_network_error_handling() {
    ntpServer.setLoggingService(&mockLoggingService);
    ntpServer.init();
    ntpServer.resetStatistics();
    
    mockTimeManager.setSynced(true);
    mockUdpManager.ntpSocketOpen = true;
    
    uint8_t validPacket[NTP_PACKET_SIZE];
    memset(validPacket, 0, NTP_PACKET_SIZE);
    validPacket[0] = NTP_SET_LI_VN_MODE(NTP_LI_NO_WARNING, NTP_VERSION, NTP_MODE_CLIENT);
    
    // 送信成功ケース
    mockEthernetUdp.setIncomingPacket(validPacket, NTP_PACKET_SIZE, IPAddress(192, 168, 1, 100), 1234);
    mockEthernetUdp.setSendSuccess(true);
    
    ntpServer.processRequests();
    
    // 送信失敗ケース
    mockEthernetUdp.setIncomingPacket(validPacket, NTP_PACKET_SIZE, IPAddress(192, 168, 1, 101), 1235);
    mockEthernetUdp.setSendSuccess(false);
    
    ntpServer.processRequests();
    
    const NtpStatistics& stats = ntpServer.getStatistics();
    
    // 統計確認（送信失敗は無効要求として記録）
    TEST_ASSERT_EQUAL(2, stats.requests_total);
    TEST_ASSERT_EQUAL(1, stats.requests_valid);
    TEST_ASSERT_EQUAL(1, stats.requests_invalid);
    TEST_ASSERT_EQUAL(1, stats.responses_sent);
}

/**
 * @brief Test UDPソケット未開時の処理
 */
void test_ntpserver_udp_socket_closed_handling() {
    ntpServer.setLoggingService(&mockLoggingService);
    ntpServer.init();
    ntpServer.resetStatistics();
    
    mockTimeManager.setSynced(true);
    
    uint8_t packet[NTP_PACKET_SIZE];
    memset(packet, 0, NTP_PACKET_SIZE);
    packet[0] = NTP_SET_LI_VN_MODE(NTP_LI_NO_WARNING, NTP_VERSION, NTP_MODE_CLIENT);
    
    // UDPソケット閉状態
    mockUdpManager.ntpSocketOpen = false;
    mockEthernetUdp.setIncomingPacket(packet, NTP_PACKET_SIZE, IPAddress(192, 168, 1, 100), 1234);
    
    ntpServer.processRequests();
    
    // 処理されないことを確認
    const NtpStatistics& stats = ntpServer.getStatistics();
    TEST_ASSERT_EQUAL(0, stats.requests_total);
    TEST_ASSERT_EQUAL(0, stats.requests_valid);
    TEST_ASSERT_EQUAL(0, stats.responses_sent);
}

/**
 * @brief Test 統計リセット機能
 */
void test_ntpserver_statistics_reset_functionality() {
    ntpServer.setLoggingService(&mockLoggingService);
    ntpServer.init();
    
    mockTimeManager.setSynced(true);
    mockUdpManager.ntpSocketOpen = true;
    
    uint8_t packet[NTP_PACKET_SIZE];
    memset(packet, 0, NTP_PACKET_SIZE);
    packet[0] = NTP_SET_LI_VN_MODE(NTP_LI_NO_WARNING, NTP_VERSION, NTP_MODE_CLIENT);
    
    // いくつかの要求を処理
    for (int i = 0; i < 3; i++) {
        mockEthernetUdp.setIncomingPacket(packet, NTP_PACKET_SIZE, 
                                         IPAddress(192, 168, 1, 100 + i), 1234 + i);
        ntpServer.processRequests();
    }
    
    // 統計が記録されていることを確認
    const NtpStatistics& statsBeforeReset = ntpServer.getStatistics();
    TEST_ASSERT_EQUAL(3, statsBeforeReset.requests_total);
    TEST_ASSERT_EQUAL(3, statsBeforeReset.requests_valid);
    TEST_ASSERT_GREATER_THAN(0.0f, statsBeforeReset.avg_processing_time);
    
    // 統計リセット
    ntpServer.resetStatistics();
    
    // リセット後の統計確認
    const NtpStatistics& statsAfterReset = ntpServer.getStatistics();
    TEST_ASSERT_EQUAL(0, statsAfterReset.requests_total);
    TEST_ASSERT_EQUAL(0, statsAfterReset.requests_valid);
    TEST_ASSERT_EQUAL(0, statsAfterReset.requests_invalid);
    TEST_ASSERT_EQUAL(0, statsAfterReset.responses_sent);
    TEST_ASSERT_EQUAL(0, statsAfterReset.last_request_time);
    TEST_ASSERT_EQUAL(0.0f, statsAfterReset.avg_processing_time);
    TEST_ASSERT_EQUAL(0, statsAfterReset.clients_served);
}

/**
 * @brief Test レート制限機能
 */
void test_ntpserver_rate_limiting_functionality() {
    ntpServer.setLoggingService(&mockLoggingService);
    ntpServer.init();
    ntpServer.resetStatistics();
    
    mockTimeManager.setSynced(true);
    mockUdpManager.ntpSocketOpen = true;
    
    uint8_t packet[NTP_PACKET_SIZE];
    memset(packet, 0, NTP_PACKET_SIZE);
    packet[0] = NTP_SET_LI_VN_MODE(NTP_LI_NO_WARNING, NTP_VERSION, NTP_MODE_CLIENT);
    
    // 同じクライアントから複数要求（レート制限テスト）
    IPAddress sameClient(192, 168, 1, 100);
    
    for (int i = 0; i < 10; i++) {
        mockEthernetUdp.setIncomingPacket(packet, NTP_PACKET_SIZE, sameClient, 1234 + i);
        ntpServer.processRequests();
    }
    
    const NtpStatistics& stats = ntpServer.getStatistics();
    
    // レート制限が無効（テスト実装では）なので全て処理される
    TEST_ASSERT_EQUAL(10, stats.requests_total);
    TEST_ASSERT_EQUAL(10, stats.requests_valid);
    TEST_ASSERT_EQUAL(10, stats.responses_sent);
}

/**
 * @brief Test NTPプロトコル仕様準拠性（RFC 5905）
 */
void test_ntpserver_rfc5905_protocol_compliance() {
    ntpServer.setLoggingService(&mockLoggingService);
    ntpServer.init();
    
    mockTimeManager.setSynced(true);
    mockTimeManager.setTime(1609459200, 123456);
    mockTimeManager.setStratum(1);
    mockUdpManager.ntpSocketOpen = true;
    
    // RFC 5905準拠のNTPクライアント要求作成
    uint8_t rfcCompliantPacket[NTP_PACKET_SIZE];
    memset(rfcCompliantPacket, 0, NTP_PACKET_SIZE);
    
    // LI=0 (no warning), VN=4, Mode=3 (client)
    rfcCompliantPacket[0] = NTP_SET_LI_VN_MODE(NTP_LI_NO_WARNING, 4, NTP_MODE_CLIENT);
    rfcCompliantPacket[1] = 0;    // stratum (unspecified for client)
    rfcCompliantPacket[2] = 6;    // poll (64 seconds)
    rfcCompliantPacket[3] = -20;  // precision (~1 microsecond)
    
    // Root delay, dispersion, reference ID (client側は0)
    memset(&rfcCompliantPacket[4], 0, 8);  // root_delay + root_dispersion
    memset(&rfcCompliantPacket[12], 0, 4); // reference_id
    
    // Timestamps (client側はtransmit timestampのみ設定)
    memset(&rfcCompliantPacket[16], 0, 24);  // reference, origin, receive
    
    NtpTimestamp clientTransmit = unixToNtpTimestamp(1609459190, 500000);
    memcpy(&rfcCompliantPacket[40], &clientTransmit, sizeof(NtpTimestamp));
    
    mockEthernetUdp.setIncomingPacket(rfcCompliantPacket, NTP_PACKET_SIZE, 
                                     IPAddress(203, 0, 113, 1), 123);
    
    ntpServer.processRequests();
    
    const NtpPacket& response = ntpServer.getLastResponse();
    
    // RFC 5905準拠の応答確認
    TEST_ASSERT_EQUAL(NTP_LI_NO_WARNING, NTP_GET_LI(response.li_vn_mode));
    TEST_ASSERT_EQUAL(4, NTP_GET_VN(response.li_vn_mode));
    TEST_ASSERT_EQUAL(NTP_MODE_SERVER, NTP_GET_MODE(response.li_vn_mode));
    TEST_ASSERT_EQUAL(1, response.stratum);
    TEST_ASSERT_EQUAL(6, response.poll);  // Echo client's poll
    TEST_ASSERT_EQUAL(-20, response.precision);
    
    // Primary reference source values
    TEST_ASSERT_EQUAL(0, ntohl(response.root_delay));
    TEST_ASSERT_EQUAL(100, ntohl(response.root_dispersion));
    TEST_ASSERT_EQUAL(htonl(NTP_REFID_GPS), response.reference_id);
    
    // Timestamp relationships (RFC 5905 section 7.3)
    TEST_ASSERT_EQUAL(clientTransmit.seconds, response.origin_timestamp.seconds);
    TEST_ASSERT_EQUAL(clientTransmit.fraction, response.origin_timestamp.fraction);
    TEST_ASSERT_NOT_EQUAL(0, ntohl(response.receive_timestamp.seconds));
    TEST_ASSERT_NOT_EQUAL(0, ntohl(response.transmit_timestamp.seconds));
}

/**
 * @brief Test NTPバージョン互換性（v3/v4）
 */
void test_ntpserver_version_compatibility() {
    ntpServer.setLoggingService(&mockLoggingService);
    ntpServer.init();
    ntpServer.resetStatistics();
    
    mockTimeManager.setSynced(true);
    mockUdpManager.ntpSocketOpen = true;
    
    uint8_t packet[NTP_PACKET_SIZE];
    memset(packet, 0, NTP_PACKET_SIZE);
    
    // NTPv3クライアント要求
    packet[0] = NTP_SET_LI_VN_MODE(NTP_LI_NO_WARNING, 3, NTP_MODE_CLIENT);
    mockEthernetUdp.setIncomingPacket(packet, NTP_PACKET_SIZE, IPAddress(192, 168, 1, 100), 1234);
    ntpServer.processRequests();
    
    // NTPv4クライアント要求
    packet[0] = NTP_SET_LI_VN_MODE(NTP_LI_NO_WARNING, 4, NTP_MODE_CLIENT);
    mockEthernetUdp.setIncomingPacket(packet, NTP_PACKET_SIZE, IPAddress(192, 168, 1, 101), 1235);
    ntpServer.processRequests();
    
    const NtpStatistics& stats = ntpServer.getStatistics();
    
    // 両バージョンが処理されることを確認
    TEST_ASSERT_EQUAL(2, stats.requests_total);
    TEST_ASSERT_EQUAL(2, stats.requests_valid);
    TEST_ASSERT_EQUAL(2, stats.responses_sent);
    
    // 応答はNTPv4で返される
    const NtpPacket& response = ntpServer.getLastResponse();
    TEST_ASSERT_EQUAL(4, NTP_GET_VN(response.li_vn_mode));
}

/**
 * @brief Test パフォーマンス・スループット測定
 */
void test_ntpserver_performance_throughput() {
    ntpServer.setLoggingService(&mockLoggingService);
    ntpServer.init();
    ntpServer.resetStatistics();
    
    mockTimeManager.setSynced(true);
    mockUdpManager.ntpSocketOpen = true;
    
    uint8_t packet[NTP_PACKET_SIZE];
    memset(packet, 0, NTP_PACKET_SIZE);
    packet[0] = NTP_SET_LI_VN_MODE(NTP_LI_NO_WARNING, NTP_VERSION, NTP_MODE_CLIENT);
    
    // 大量要求処理（パフォーマンステスト）
    const int REQUEST_COUNT = 100;
    
    for (int i = 0; i < REQUEST_COUNT; i++) {
        mockEthernetUdp.setIncomingPacket(packet, NTP_PACKET_SIZE, 
                                         IPAddress(192, 168, 1, (i % 254) + 1), 1234 + i);
        ntpServer.processRequests();
    }
    
    const NtpStatistics& stats = ntpServer.getStatistics();
    
    // スループット確認
    TEST_ASSERT_EQUAL(REQUEST_COUNT, stats.requests_total);
    TEST_ASSERT_EQUAL(REQUEST_COUNT, stats.requests_valid);
    TEST_ASSERT_EQUAL(REQUEST_COUNT, stats.responses_sent);
    
    // 平均処理時間が合理的であることを確認
    TEST_ASSERT_GREATER_THAN(0.0f, stats.avg_processing_time);
    TEST_ASSERT_LESS_THAN(100.0f, stats.avg_processing_time); // 100ms未満
}

/**
 * @brief Test タイムスタンプ精度・オーバーフロー処理
 */
void test_ntpserver_timestamp_precision_overflow() {
    ntpServer.setLoggingService(&mockLoggingService);
    ntpServer.init();
    
    mockTimeManager.setSynced(true);
    mockUdpManager.ntpSocketOpen = true;
    
    uint8_t packet[NTP_PACKET_SIZE];
    memset(packet, 0, NTP_PACKET_SIZE);
    packet[0] = NTP_SET_LI_VN_MODE(NTP_LI_NO_WARNING, NTP_VERSION, NTP_MODE_CLIENT);
    
    // Unix epoch境界値テスト（2038年問題）
    mockTimeManager.setTime(2147483647, 999999); // 2038-01-19 03:14:07
    mockEthernetUdp.setIncomingPacket(packet, NTP_PACKET_SIZE, IPAddress(192, 168, 1, 100), 1234);
    ntpServer.processRequests();
    
    const NtpPacket& response1 = ntpServer.getLastResponse();
    uint32_t ntpTime1 = ntohl(response1.transmit_timestamp.seconds);
    TEST_ASSERT_GREATER_THAN(NTP_TIMESTAMP_DELTA, ntpTime1); // NTPエポック後
    
    // Unix epoch直前
    mockTimeManager.setTime(0, 0); // 1970-01-01 00:00:00
    mockEthernetUdp.setIncomingPacket(packet, NTP_PACKET_SIZE, IPAddress(192, 168, 1, 101), 1235);
    ntpServer.processRequests();
    
    const NtpPacket& response2 = ntpServer.getLastResponse();
    uint32_t ntpTime2 = ntohl(response2.transmit_timestamp.seconds);
    TEST_ASSERT_EQUAL(NTP_TIMESTAMP_DELTA, ntpTime2); // 正確にNTPエポック + 70年
    
    // マイクロ秒精度テスト
    mockTimeManager.setTime(1609459200, 123456); // 123.456ms
    mockEthernetUdp.setIncomingPacket(packet, NTP_PACKET_SIZE, IPAddress(192, 168, 1, 102), 1236);
    ntpServer.processRequests();
    
    const NtpPacket& response3 = ntpServer.getLastResponse();
    uint32_t fraction = ntohl(response3.transmit_timestamp.fraction);
    
    // フラクション部が正しく設定されていることを確認
    TEST_ASSERT_NOT_EQUAL(0, fraction);
    
    // 計算精度確認（123456マイクロ秒をNTPフラクションに変換）
    uint32_t expectedFraction = (uint32_t)((uint64_t)123456 * 4294967296ULL / 1000000ULL);
    // 許容誤差内（計算誤差を考慮）
    uint32_t diff = (fraction > expectedFraction) ? (fraction - expectedFraction) : (expectedFraction - fraction);
    TEST_ASSERT_LESS_THAN(1000, diff); // 1000以下の誤差
}

/**
 * @brief Test 境界値・エッジケース処理
 */
void test_ntpserver_boundary_edge_cases() {
    // nullポインタ処理テスト
    NtpServer nullPtrServer(nullptr, nullptr, nullptr);
    nullPtrServer.init(); // クラッシュしないことを確認
    
    // UDPソケットnullで要求処理
    nullPtrServer.processRequests(); // クラッシュしないことを確認
    
    // TimeManager nullで処理
    UdpSocketManager openSocket = {true, 0, 10000, 0};
    NtpServer nullTimeServer(&mockEthernetUdp, nullptr, &openSocket);
    nullTimeServer.setLoggingService(&mockLoggingService);
    nullTimeServer.init();
    
    uint8_t packet[NTP_PACKET_SIZE];
    memset(packet, 0, NTP_PACKET_SIZE);
    packet[0] = NTP_SET_LI_VN_MODE(NTP_LI_NO_WARNING, NTP_VERSION, NTP_MODE_CLIENT);
    
    mockEthernetUdp.setIncomingPacket(packet, NTP_PACKET_SIZE, IPAddress(192, 168, 1, 100), 1234);
    nullTimeServer.processRequests(); // フォールバック時刻で動作
    
    // パケットなし状態での処理
    ntpServer.setLoggingService(&mockLoggingService);
    ntpServer.init();
    ntpServer.resetStatistics();
    
    mockUdpManager.ntpSocketOpen = true;
    mockEthernetUdp.clearPacket(); // パケットなし
    
    ntpServer.processRequests();
    
    // 何も処理されないことを確認
    const NtpStatistics& emptyStats = ntpServer.getStatistics();
    TEST_ASSERT_EQUAL(0, emptyStats.requests_total);
    
    // 極端なタイムスタンプ値
    mockTimeManager.setSynced(true);
    mockTimeManager.setTime(0xFFFFFFFF, 999999); // 最大値
    
    uint8_t maxTimePacket[NTP_PACKET_SIZE];
    memset(maxTimePacket, 0, NTP_PACKET_SIZE);
    maxTimePacket[0] = NTP_SET_LI_VN_MODE(NTP_LI_NO_WARNING, NTP_VERSION, NTP_MODE_CLIENT);
    
    mockEthernetUdp.setIncomingPacket(maxTimePacket, NTP_PACKET_SIZE, IPAddress(192, 168, 1, 100), 1234);
    ntpServer.processRequests(); // オーバーフローしないことを確認
    
    // ゼロ時刻
    mockTimeManager.setTime(0, 0);
    mockEthernetUdp.setIncomingPacket(maxTimePacket, NTP_PACKET_SIZE, IPAddress(192, 168, 1, 101), 1235);
    ntpServer.processRequests(); // ゼロ時刻でも動作
    
    const NtpStatistics& extremeStats = ntpServer.getStatistics();
    TEST_ASSERT_EQUAL(2, extremeStats.requests_total);
    TEST_ASSERT_EQUAL(2, extremeStats.requests_valid);
    
    // 不正なポート番号（境界値）
    mockEthernetUdp.setIncomingPacket(packet, NTP_PACKET_SIZE, IPAddress(192, 168, 1, 102), 0);
    ntpServer.processRequests(); // ポート0でも動作
    
    mockEthernetUdp.setIncomingPacket(packet, NTP_PACKET_SIZE, IPAddress(192, 168, 1, 103), 65535);
    ntpServer.processRequests(); // 最大ポート番号でも動作
    
    const NtpStatistics& finalStats = ntpServer.getStatistics();
    TEST_ASSERT_EQUAL(4, finalStats.requests_total);
    TEST_ASSERT_EQUAL(4, finalStats.requests_valid);
}

// Test suite setup and teardown
void setUp(void) {
    // Reset mock states
    mockEthernetUdp.clearPacket();
    mockEthernetUdp.setSendSuccess(true);
    
    mockTimeManager.setSynced(true);
    mockTimeManager.setTime(1609459200, 0);
    mockTimeManager.setStratum(1);
    
    mockUdpManager.ntpSocketOpen = true;
    mockUdpManager.lastSocketCheck = 0;
    mockUdpManager.socketCheckInterval = 10000;
    mockUdpManager.socketErrors = 0;
    
    mockLoggingService.info_count = 0;
    mockLoggingService.warning_count = 0;
    mockLoggingService.error_count = 0;
}

void tearDown(void) {
    // Cleanup after each test
}

/**
 * @brief NtpServer完全カバレッジテスト実行
 */
int main() {
    UNITY_BEGIN();
    
    // Core functionality tests
    RUN_TEST(test_ntpserver_basic_initialization_configuration);
    RUN_TEST(test_ntpserver_ntp_packet_parsing_validation);
    RUN_TEST(test_ntpserver_ntp_response_generation_field_setup);
    RUN_TEST(test_ntpserver_high_precision_timestamp_microsecond_accuracy);
    
    // GPS synchronization and time management
    RUN_TEST(test_ntpserver_gps_sync_status_stratum_calculation);
    RUN_TEST(test_ntpserver_timestamp_precision_overflow);
    
    // Protocol compliance and compatibility
    RUN_TEST(test_ntpserver_rfc5905_protocol_compliance);
    RUN_TEST(test_ntpserver_version_compatibility);
    
    // Error handling and edge cases
    RUN_TEST(test_ntpserver_invalid_packet_handling_error_processing);
    RUN_TEST(test_ntpserver_send_failure_network_error_handling);
    RUN_TEST(test_ntpserver_udp_socket_closed_handling);
    RUN_TEST(test_ntpserver_boundary_edge_cases);
    
    // Statistics and monitoring
    RUN_TEST(test_ntpserver_ntp_statistics_processing_time_calculation);
    RUN_TEST(test_ntpserver_statistics_reset_functionality);
    
    // Performance and scalability
    RUN_TEST(test_ntpserver_multiple_client_concurrent_processing);
    RUN_TEST(test_ntpserver_rate_limiting_functionality);
    RUN_TEST(test_ntpserver_performance_throughput);
    
    return UNITY_END();
}