#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <memory>

// NTP Server test structures and constants
#define NTP_PACKET_SIZE 48
#define NTP_PORT 123
#define NTP_VERSION 4
#define NTP_MODE_CLIENT 3
#define NTP_MODE_SERVER 4

// NTP packet structure
struct NTPPacket {
    uint8_t li_vn_mode;      // Leap Indicator, Version, Mode
    uint8_t stratum;         // Stratum level
    uint8_t poll;            // Poll interval
    int8_t precision;        // Precision
    uint32_t root_delay;     // Root delay
    uint32_t root_dispersion; // Root dispersion
    uint32_t ref_id;         // Reference identifier
    uint64_t ref_timestamp;  // Reference timestamp
    uint64_t orig_timestamp; // Origin timestamp
    uint64_t recv_timestamp; // Receive timestamp
    uint64_t xmit_timestamp; // Transmit timestamp
};

struct NTPStats {
    uint32_t requests_received;
    uint32_t responses_sent;
    uint32_t invalid_requests;
    uint32_t auth_failures;
    float average_response_time_ms;
    float max_response_time_ms;
    uint32_t stratum_1_responses;
    uint32_t stratum_2_responses;
    uint32_t stratum_3_responses;
};

enum NTPServerStatus {
    NTP_SERVER_STOPPED = 0,
    NTP_SERVER_STARTING = 1,
    NTP_SERVER_RUNNING = 2,
    NTP_SERVER_ERROR = 3
};

// Mock interfaces
class MockUDPInterface {
public:
    MOCK_METHOD(bool, begin, (uint16_t port));
    MOCK_METHOD(void, stop, ());
    MOCK_METHOD(bool, parsePacket, ());
    MOCK_METHOD(size_t, read, (uint8_t* buffer, size_t len));
    MOCK_METHOD(size_t, write, (const uint8_t* buffer, size_t len));
    MOCK_METHOD(bool, beginPacket, (uint32_t ip, uint16_t port));
    MOCK_METHOD(bool, endPacket, ());
    MOCK_METHOD(uint32_t, remoteIP, ());
    MOCK_METHOD(uint16_t, remotePort, ());
};

class MockTimeSourceInterface {
public:
    MOCK_METHOD(uint64_t, getNTPTimestamp, ());
    MOCK_METHOD(uint8_t, getStratum, ());
    MOCK_METHOD(float, getAccuracy, ());
    MOCK_METHOD(bool, isTimeValid, ());
    MOCK_METHOD(uint32_t, getRefId, ());
};

// Concrete mock implementations for complex scenarios
class ConcreteMockUDPHAL {
public:
    bool server_running = false;
    uint16_t listening_port = 0;
    uint8_t receive_buffer[NTP_PACKET_SIZE];
    size_t receive_buffer_size = 0;
    uint8_t send_buffer[NTP_PACKET_SIZE];
    size_t send_buffer_size = 0;
    uint32_t client_ip = 0xC0A80102; // 192.168.1.2
    uint16_t client_port = 12345;
    bool has_packet = false;
    int error_rate = 0;
    
    bool begin(uint16_t port) {
        if (error_rate > 0 && (std::rand() % 100) < error_rate) {
            return false;
        }
        
        listening_port = port;
        server_running = true;
        return true;
    }
    
    void stop() {
        server_running = false;
        listening_port = 0;
    }
    
    bool parsePacket() {
        if (!server_running || !has_packet) {
            return false;
        }
        
        has_packet = false; // Consume the packet
        return true;
    }
    
    size_t read(uint8_t* buffer, size_t len) {
        if (!server_running || receive_buffer_size == 0) {
            return 0;
        }
        
        size_t bytes_to_read = (len < receive_buffer_size) ? len : receive_buffer_size;
        std::memcpy(buffer, receive_buffer, bytes_to_read);
        receive_buffer_size = 0; // Clear after reading
        
        return bytes_to_read;
    }
    
    size_t write(const uint8_t* buffer, size_t len) {
        if (!server_running || len > sizeof(send_buffer)) {
            return 0;
        }
        
        if (error_rate > 0 && (std::rand() % 100) < error_rate) {
            return 0; // Simulate write failure
        }
        
        std::memcpy(send_buffer, buffer, len);
        send_buffer_size = len;
        
        return len;
    }
    
    bool beginPacket(uint32_t ip, uint16_t port) {
        if (!server_running) {
            return false;
        }
        
        client_ip = ip;
        client_port = port;
        return true;
    }
    
    bool endPacket() {
        if (!server_running) {
            return false;
        }
        
        // Simulate packet sending
        return true;
    }
    
    uint32_t remoteIP() {
        return client_ip;
    }
    
    uint16_t remotePort() {
        return client_port;
    }
    
    // Test helper methods
    void simulateClientRequest(const NTPPacket& request) {
        std::memcpy(receive_buffer, &request, sizeof(request));
        receive_buffer_size = sizeof(request);
        has_packet = true;
    }
    
    void reset() {
        server_running = false;
        listening_port = 0;
        receive_buffer_size = 0;
        send_buffer_size = 0;
        client_ip = 0xC0A80102;
        client_port = 12345;
        has_packet = false;
        error_rate = 0;
        std::memset(receive_buffer, 0, sizeof(receive_buffer));
        std::memset(send_buffer, 0, sizeof(send_buffer));
    }
    
    void setErrorRate(int percentage) {
        error_rate = (percentage > 100) ? 100 : percentage;
    }
    
    const uint8_t* getSentPacket() const {
        return send_buffer;
    }
    
    size_t getSentPacketSize() const {
        return send_buffer_size;
    }
};

class ConcreteMockTimeSource {
public:
    uint64_t current_ntp_time = 0;
    uint8_t stratum_level = 1;
    float time_accuracy = 0.1f; // 0.1ms accuracy
    bool time_valid = true;
    uint32_t reference_id = 0x47505300; // "GPS\0"
    
    uint64_t getNTPTimestamp() {
        static uint64_t base_time = 3849283200ULL << 32; // 2022-01-01 in NTP epoch
        static uint32_t counter = 0;
        return base_time + (++counter);
    }
    
    uint8_t getStratum() {
        return stratum_level;
    }
    
    float getAccuracy() {
        return time_accuracy;
    }
    
    bool isTimeValid() {
        return time_valid;
    }
    
    uint32_t getRefId() {
        return reference_id;
    }
    
    void setStratum(uint8_t stratum) {
        stratum_level = stratum;
    }
    
    void setAccuracy(float accuracy_ms) {
        time_accuracy = accuracy_ms;
    }
    
    void setTimeValid(bool valid) {
        time_valid = valid;
    }
    
    void setRefId(uint32_t ref_id) {
        reference_id = ref_id;
    }
    
    void reset() {
        current_ntp_time = 0;
        stratum_level = 1;
        time_accuracy = 0.1f;
        time_valid = true;
        reference_id = 0x47505300;
    }
};

// Extended NTP Server
class ExtendedNTPServer {
private:
    ConcreteMockUDPHAL* udp;
    ConcreteMockTimeSource* time_source;
    NTPServerStatus status;
    uint16_t server_port;
    NTPStats stats;
    
    // Server configuration
    bool authentication_enabled;
    uint32_t max_clients;
    uint32_t rate_limit_requests_per_second;
    uint32_t precision;
    uint32_t poll_interval;
    
    // Rate limiting
    uint32_t last_rate_limit_reset;
    uint32_t current_request_count;
    
    // Performance monitoring
    float response_times[100];
    uint8_t response_time_index;
    
public:
    ExtendedNTPServer(ConcreteMockUDPHAL* udp_hal, ConcreteMockTimeSource* time_src)
        : udp(udp_hal), time_source(time_src) {
        status = NTP_SERVER_STOPPED;
        server_port = NTP_PORT;
        std::memset(&stats, 0, sizeof(stats));
        
        authentication_enabled = false;
        max_clients = 100;
        rate_limit_requests_per_second = 1000;
        precision = -20; // ~1 microsecond
        poll_interval = 6; // 64 seconds (2^6)
        
        last_rate_limit_reset = 0;
        current_request_count = 0;
        
        std::memset(response_times, 0, sizeof(response_times));
        response_time_index = 0;
    }
    
    uint32_t getCurrentTime() const {
        static uint32_t time_counter = 1000;
        return time_counter += 50; // Advance by 50ms each call
    }
    
    bool initialize(uint16_t port = NTP_PORT) {
        server_port = port;
        
        if (!udp->begin(server_port)) {
            status = NTP_SERVER_ERROR;
            return false;
        }
        
        status = NTP_SERVER_RUNNING;
        return true;
    }
    
    void stop() {
        udp->stop();
        status = NTP_SERVER_STOPPED;
    }
    
    void update() {
        if (status != NTP_SERVER_RUNNING) {
            return;
        }
        
        // Reset rate limiting counter periodically
        uint32_t current_time = getCurrentTime();
        if (current_time - last_rate_limit_reset >= 1000) { // 1 second
            current_request_count = 0;
            last_rate_limit_reset = current_time;
        }
        
        // Process incoming packets
        while (udp->parsePacket()) {
            handleNTPRequest();
        }
    }
    
    void handleNTPRequest() {
        uint32_t start_time = getCurrentTime();
        
        stats.requests_received++;
        
        // Check rate limiting
        if (current_request_count >= rate_limit_requests_per_second) {
            return; // Drop request due to rate limiting
        }
        current_request_count++;
        
        // Read the NTP request
        NTPPacket request_packet;
        size_t bytes_read = udp->read(reinterpret_cast<uint8_t*>(&request_packet), sizeof(request_packet));
        
        if (bytes_read != sizeof(request_packet)) {
            stats.invalid_requests++;
            return;
        }
        
        // Validate NTP packet
        uint8_t version = (request_packet.li_vn_mode >> 3) & 0x07;
        uint8_t mode = request_packet.li_vn_mode & 0x07;
        
        if (version < 3 || version > 4 || mode != NTP_MODE_CLIENT) {
            stats.invalid_requests++;
            return;
        }
        
        // Check if time source is valid
        if (!time_source->isTimeValid()) {
            stats.invalid_requests++;
            return;
        }
        
        // Create response packet
        NTPPacket response_packet = {};
        
        // Set LI, VN, Mode
        response_packet.li_vn_mode = (0 << 6) | (NTP_VERSION << 3) | NTP_MODE_SERVER;
        response_packet.stratum = time_source->getStratum();
        response_packet.poll = poll_interval;
        response_packet.precision = static_cast<int8_t>(precision);
        response_packet.root_delay = htonl(static_cast<uint32_t>(time_source->getAccuracy() * 65536));
        response_packet.root_dispersion = htonl(100 << 16); // 100ms dispersion
        response_packet.ref_id = htonl(time_source->getRefId());
        
        // Timestamps
        uint64_t current_ntp_time = time_source->getNTPTimestamp();
        response_packet.ref_timestamp = htonll(current_ntp_time - 3600); // Ref time 1 hour ago
        response_packet.orig_timestamp = request_packet.xmit_timestamp;   // Echo client's transmit
        response_packet.recv_timestamp = htonll(current_ntp_time - 1);    // Simulated receive time
        response_packet.xmit_timestamp = htonll(current_ntp_time);        // Current transmit time
        
        // Send response
        if (sendNTPResponse(response_packet)) {
            stats.responses_sent++;
            
            // Update stratum statistics
            switch (response_packet.stratum) {
                case 1: stats.stratum_1_responses++; break;
                case 2: stats.stratum_2_responses++; break;
                case 3: stats.stratum_3_responses++; break;
            }
        }
        
        // Update performance statistics
        uint32_t end_time = getCurrentTime();
        float response_time = static_cast<float>(end_time - start_time);
        updatePerformanceStats(response_time);
    }
    
    bool sendNTPResponse(const NTPPacket& response) {
        if (!udp->beginPacket(udp->remoteIP(), udp->remotePort())) {
            return false;
        }
        
        size_t bytes_written = udp->write(reinterpret_cast<const uint8_t*>(&response), sizeof(response));
        
        if (bytes_written != sizeof(response)) {
            return false;
        }
        
        return udp->endPacket();
    }
    
    void updatePerformanceStats(float response_time) {
        response_times[response_time_index] = response_time;
        response_time_index = (response_time_index + 1) % 100;
        
        // Calculate average and max
        float sum = 0.0f;
        float max_time = 0.0f;
        
        for (int i = 0; i < 100; i++) {
            sum += response_times[i];
            if (response_times[i] > max_time) {
                max_time = response_times[i];
            }
        }
        
        stats.average_response_time_ms = sum / 100.0f;
        stats.max_response_time_ms = max_time;
    }
    
    // Utility functions
    uint32_t htonl(uint32_t hostlong) {
        return ((hostlong & 0xFF000000) >> 24) |
               ((hostlong & 0x00FF0000) >> 8) |
               ((hostlong & 0x0000FF00) << 8) |
               ((hostlong & 0x000000FF) << 24);
    }
    
    uint64_t htonll(uint64_t hostlonglong) {
        uint32_t high_part = htonl(static_cast<uint32_t>(hostlonglong >> 32));
        uint32_t low_part = htonl(static_cast<uint32_t>(hostlonglong & 0xFFFFFFFFULL));
        return (static_cast<uint64_t>(low_part) << 32) | high_part;
    }
    
    // Configuration methods
    void setAuthenticationEnabled(bool enabled) {
        authentication_enabled = enabled;
    }
    
    void setMaxClients(uint32_t max_clients_count) {
        max_clients = max_clients_count;
    }
    
    void setRateLimit(uint32_t requests_per_second) {
        rate_limit_requests_per_second = requests_per_second;
    }
    
    void setPrecision(int8_t precision_exp) {
        precision = static_cast<uint32_t>(precision_exp);
    }
    
    void setPollInterval(uint8_t poll_exp) {
        poll_interval = poll_exp;
    }
    
    // Status getters
    NTPServerStatus getStatus() const {
        return status;
    }
    
    bool isRunning() const {
        return status == NTP_SERVER_RUNNING;
    }
    
    uint16_t getPort() const {
        return server_port;
    }
    
    NTPStats getStats() const {
        return stats;
    }
    
    uint32_t getRequestsReceived() const {
        return stats.requests_received;
    }
    
    uint32_t getResponsesSent() const {
        return stats.responses_sent;
    }
    
    uint32_t getInvalidRequests() const {
        return stats.invalid_requests;
    }
    
    float getAverageResponseTime() const {
        return stats.average_response_time_ms;
    }
    
    float getMaxResponseTime() const {
        return stats.max_response_time_ms;
    }
    
    bool isAuthenticationEnabled() const {
        return authentication_enabled;
    }
    
    uint32_t getMaxClients() const {
        return max_clients;
    }
    
    uint32_t getRateLimit() const {
        return rate_limit_requests_per_second;
    }
    
    void resetStats() {
        std::memset(&stats, 0, sizeof(stats));
        std::memset(response_times, 0, sizeof(response_times));
        response_time_index = 0;
    }
};

// Test fixture class
class NTPServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockUDP = std::make_unique<ConcreteMockUDPHAL>();
        mockTimeSource = std::make_unique<ConcreteMockTimeSource>();
        ntpServer = std::make_unique<ExtendedNTPServer>(mockUDP.get(), mockTimeSource.get());
    }
    
    void TearDown() override {
        ntpServer.reset();
        mockTimeSource.reset();
        mockUDP.reset();
    }
    
    NTPPacket createValidNTPRequest() {
        NTPPacket request = {};
        request.li_vn_mode = (0 << 6) | (NTP_VERSION << 3) | NTP_MODE_CLIENT;
        request.stratum = 0;  // Unspecified
        request.poll = 6;     // 64 seconds
        request.precision = -6; // ~15ms
        request.xmit_timestamp = mockTimeSource->getNTPTimestamp() - 1000; // 1 second ago
        return request;
    }
    
    std::unique_ptr<ConcreteMockUDPHAL> mockUDP;
    std::unique_ptr<ConcreteMockTimeSource> mockTimeSource;
    std::unique_ptr<ExtendedNTPServer> ntpServer;
};

// Basic NTP Server Tests
TEST_F(NTPServerTest, Initialization) {
    EXPECT_TRUE(ntpServer->initialize());
    EXPECT_TRUE(ntpServer->isRunning());
    EXPECT_EQ(NTP_PORT, ntpServer->getPort());
}

TEST_F(NTPServerTest, InitializationFailure) {
    mockUDP->setErrorRate(100);
    
    EXPECT_FALSE(ntpServer->initialize());
    EXPECT_EQ(NTP_SERVER_ERROR, ntpServer->getStatus());
}

TEST_F(NTPServerTest, CustomPortInitialization) {
    uint16_t custom_port = 1123;
    
    EXPECT_TRUE(ntpServer->initialize(custom_port));
    EXPECT_EQ(custom_port, ntpServer->getPort());
}

TEST_F(NTPServerTest, ServerStopAndStart) {
    EXPECT_TRUE(ntpServer->initialize());
    EXPECT_TRUE(ntpServer->isRunning());
    
    ntpServer->stop();
    EXPECT_FALSE(ntpServer->isRunning());
    EXPECT_EQ(NTP_SERVER_STOPPED, ntpServer->getStatus());
}

TEST_F(NTPServerTest, ValidNTPRequestHandling) {
    EXPECT_TRUE(ntpServer->initialize());
    
    NTPPacket request = createValidNTPRequest();
    mockUDP->simulateClientRequest(request);
    
    ntpServer->update();
    
    EXPECT_EQ(1U, ntpServer->getRequestsReceived());
    EXPECT_EQ(1U, ntpServer->getResponsesSent());
    EXPECT_EQ(0U, ntpServer->getInvalidRequests());
}

TEST_F(NTPServerTest, InvalidNTPRequestHandling) {
    EXPECT_TRUE(ntpServer->initialize());
    
    NTPPacket invalid_request = {};
    invalid_request.li_vn_mode = (0 << 6) | (2 << 3) | NTP_MODE_CLIENT; // Invalid version
    mockUDP->simulateClientRequest(invalid_request);
    
    ntpServer->update();
    
    EXPECT_EQ(1U, ntpServer->getRequestsReceived());
    EXPECT_EQ(0U, ntpServer->getResponsesSent());
    EXPECT_EQ(1U, ntpServer->getInvalidRequests());
}

TEST_F(NTPServerTest, InvalidTimeSourceHandling) {
    EXPECT_TRUE(ntpServer->initialize());
    
    mockTimeSource->setTimeValid(false);
    
    NTPPacket request = createValidNTPRequest();
    mockUDP->simulateClientRequest(request);
    
    ntpServer->update();
    
    EXPECT_EQ(1U, ntpServer->getRequestsReceived());
    EXPECT_EQ(0U, ntpServer->getResponsesSent());
    EXPECT_EQ(1U, ntpServer->getInvalidRequests());
}

TEST_F(NTPServerTest, StratumConfiguration) {
    EXPECT_TRUE(ntpServer->initialize());
    
    // Test different stratum levels
    for (uint8_t stratum = 1; stratum <= 3; stratum++) {
        mockTimeSource->setStratum(stratum);
        
        NTPPacket request = createValidNTPRequest();
        mockUDP->simulateClientRequest(request);
        
        ntpServer->update();
    }
    
    NTPStats stats = ntpServer->getStats();
    EXPECT_EQ(1U, stats.stratum_1_responses);
    EXPECT_EQ(1U, stats.stratum_2_responses);
    EXPECT_EQ(1U, stats.stratum_3_responses);
}

TEST_F(NTPServerTest, ResponsePacketValidation) {
    EXPECT_TRUE(ntpServer->initialize());
    
    NTPPacket request = createValidNTPRequest();
    mockUDP->simulateClientRequest(request);
    
    ntpServer->update();
    
    // Check that a response was sent
    EXPECT_GT(mockUDP->getSentPacketSize(), 0U);
    EXPECT_EQ(sizeof(NTPPacket), mockUDP->getSentPacketSize());
    
    // Validate response packet structure
    const NTPPacket* response = reinterpret_cast<const NTPPacket*>(mockUDP->getSentPacket());
    EXPECT_EQ(NTP_MODE_SERVER, response->li_vn_mode & 0x07);
    EXPECT_EQ(NTP_VERSION, (response->li_vn_mode >> 3) & 0x07);
}

TEST_F(NTPServerTest, RateLimiting) {
    EXPECT_TRUE(ntpServer->initialize());
    
    ntpServer->setRateLimit(2); // Very low rate limit for testing
    
    // Send multiple requests rapidly
    for (int i = 0; i < 5; i++) {
        NTPPacket request = createValidNTPRequest();
        mockUDP->simulateClientRequest(request);
        ntpServer->update();
    }
    
    // Should have processed more requests than responses due to rate limiting
    EXPECT_GT(ntpServer->getRequestsReceived(), ntpServer->getResponsesSent());
}

TEST_F(NTPServerTest, PerformanceStatistics) {
    EXPECT_TRUE(ntpServer->initialize());
    
    // Process several requests to generate performance data
    for (int i = 0; i < 10; i++) {
        NTPPacket request = createValidNTPRequest();
        mockUDP->simulateClientRequest(request);
        ntpServer->update();
    }
    
    EXPECT_GT(ntpServer->getAverageResponseTime(), 0.0f);
    EXPECT_GT(ntpServer->getMaxResponseTime(), 0.0f);
    EXPECT_GE(ntpServer->getMaxResponseTime(), ntpServer->getAverageResponseTime());
}

TEST_F(NTPServerTest, ConfigurationSettings) {
    ntpServer->setAuthenticationEnabled(true);
    EXPECT_TRUE(ntpServer->isAuthenticationEnabled());
    
    ntpServer->setMaxClients(50);
    EXPECT_EQ(50U, ntpServer->getMaxClients());
    
    ntpServer->setRateLimit(500);
    EXPECT_EQ(500U, ntpServer->getRateLimit());
}

TEST_F(NTPServerTest, StatisticsReset) {
    EXPECT_TRUE(ntpServer->initialize());
    
    // Generate some statistics
    NTPPacket request = createValidNTPRequest();
    mockUDP->simulateClientRequest(request);
    ntpServer->update();
    
    EXPECT_GT(ntpServer->getRequestsReceived(), 0U);
    
    // Reset statistics
    ntpServer->resetStats();
    
    NTPStats stats = ntpServer->getStats();
    EXPECT_EQ(0U, stats.requests_received);
    EXPECT_EQ(0U, stats.responses_sent);
    EXPECT_EQ(0U, stats.invalid_requests);
    EXPECT_EQ(0.0f, stats.average_response_time_ms);
    EXPECT_EQ(0.0f, stats.max_response_time_ms);
}

// Parameterized Tests for different NTP versions
class NTPVersionTest : public NTPServerTest, 
                      public ::testing::WithParamInterface<std::tuple<uint8_t, bool>> {};

TEST_P(NTPVersionTest, NTPVersionHandling) {
    auto [version, should_be_valid] = GetParam();
    
    EXPECT_TRUE(ntpServer->initialize());
    
    NTPPacket request = {};
    request.li_vn_mode = (0 << 6) | (version << 3) | NTP_MODE_CLIENT;
    request.stratum = 0;
    request.poll = 6;
    request.precision = -6;
    
    mockUDP->simulateClientRequest(request);
    ntpServer->update();
    
    if (should_be_valid) {
        EXPECT_EQ(1U, ntpServer->getResponsesSent());
        EXPECT_EQ(0U, ntpServer->getInvalidRequests());
    } else {
        EXPECT_EQ(0U, ntpServer->getResponsesSent());
        EXPECT_EQ(1U, ntpServer->getInvalidRequests());
    }
}

INSTANTIATE_TEST_SUITE_P(
    NTPVersions,
    NTPVersionTest,
    ::testing::Values(
        std::make_tuple(1, false), // Version 1 - invalid
        std::make_tuple(2, false), // Version 2 - invalid
        std::make_tuple(3, true),  // Version 3 - valid
        std::make_tuple(4, true),  // Version 4 - valid
        std::make_tuple(5, false)  // Version 5 - invalid
    )
);

TEST_F(NTPServerTest, MultipleClientHandling) {
    EXPECT_TRUE(ntpServer->initialize());
    
    // Simulate multiple clients
    for (int client = 0; client < 5; client++) {
        mockUDP->client_ip = 0xC0A80100 + client; // Different IP for each client
        mockUDP->client_port = 12345 + client;
        
        NTPPacket request = createValidNTPRequest();
        mockUDP->simulateClientRequest(request);
        ntpServer->update();
    }
    
    EXPECT_EQ(5U, ntpServer->getRequestsReceived());
    EXPECT_EQ(5U, ntpServer->getResponsesSent());
}

TEST_F(NTPServerTest, TimeSourceAccuracyReflection) {
    EXPECT_TRUE(ntpServer->initialize());
    
    // Test with different accuracy levels
    float test_accuracies[] = {0.001f, 0.1f, 1.0f, 10.0f};
    
    for (float accuracy : test_accuracies) {
        mockTimeSource->setAccuracy(accuracy);
        
        NTPPacket request = createValidNTPRequest();
        mockUDP->simulateClientRequest(request);
        ntpServer->update();
        
        // Response should be sent regardless of accuracy
        // (accuracy affects packet content, not handling success)
    }
    
    EXPECT_EQ(4U, ntpServer->getResponsesSent());
}

TEST_F(NTPServerTest, UpdateWithoutRequests) {
    EXPECT_TRUE(ntpServer->initialize());
    
    // Call update multiple times without any requests
    for (int i = 0; i < 10; i++) {
        ntpServer->update();
    }
    
    // Should remain stable without any activity
    EXPECT_EQ(0U, ntpServer->getRequestsReceived());
    EXPECT_EQ(0U, ntpServer->getResponsesSent());
    EXPECT_TRUE(ntpServer->isRunning());
}