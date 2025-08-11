#include <unity.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

// NTP Protocol Constants (RFC 5905)
#define NTP_EPOCH_OFFSET 2208988800UL  // 1900 to 1970
#define NTP_PORT 123
#define NTP_VERSION 4
#define NTP_MAX_STRATUM 15
#define NTP_MIN_POLL 4     // 16 seconds
#define NTP_MAX_POLL 10    // 1024 seconds

// NTP Packet Structure
struct NTPPacket {
    uint8_t li_vn_mode;     // Leap Indicator, Version, Mode
    uint8_t stratum;        // Stratum level
    int8_t poll;            // Poll interval
    int8_t precision;       // Clock precision
    uint32_t root_delay;    // Root delay
    uint32_t root_dispersion; // Root dispersion
    uint32_t reference_id;  // Reference identifier
    uint64_t reference_ts;  // Reference timestamp
    uint64_t origin_ts;     // Origin timestamp
    uint64_t receive_ts;    // Receive timestamp
    uint64_t transmit_ts;   // Transmit timestamp
} __attribute__((packed));

// NTP Server Statistics
struct NTPStats {
    uint32_t total_requests;
    uint32_t valid_requests;
    uint32_t invalid_requests;
    uint32_t responses_sent;
    uint32_t client_count;
    uint64_t total_bytes_sent;
    uint64_t total_bytes_received;
    float average_response_time_us;
    float max_response_time_us;
    uint32_t stratum_1_responses;
    uint32_t stratum_2_responses;
    uint32_t stratum_3_responses;
    uint32_t kiss_of_death_sent;
    uint32_t rate_limited_requests;
};

// Time Quality Information
struct TimeQuality {
    bool time_synchronized;
    uint8_t current_stratum;
    float accuracy_ms;
    float jitter_ms;
    uint64_t last_sync_time;
    uint32_t sync_source;       // 0=GPS, 1=RTC, 2=NTP, 3=LOCAL
    float frequency_offset_ppm;
    uint32_t leap_second_status; // 0=normal, 1=+leap, 2=-leap, 3=unsync
};

// Client Session Information
struct ClientSession {
    uint32_t client_ip;
    uint16_t client_port;
    uint32_t request_count;
    uint64_t last_request_time;
    uint32_t rate_limit_violations;
    bool is_authenticated;
    uint8_t poll_interval;
    float estimated_offset_ms;
    float estimated_delay_ms;
};

// Mock UDP HAL for testing
class MockUDPHAL {
public:
    struct MockPacket {
        uint32_t src_ip;
        uint16_t src_port;
        uint8_t data[48];  // NTP packet size
        size_t size;
        uint64_t timestamp;
    };
    
    MockPacket incoming_packets[10];
    MockPacket outgoing_packets[10];
    int incoming_count = 0;
    int outgoing_count = 0;
    bool server_started = false;
    uint16_t server_port = NTP_PORT;
    int error_rate = 0;
    uint32_t processing_delay_us = 100; // Microseconds
    
    bool begin(uint16_t port) {
        if (error_rate > 0 && (rand() % 100) < error_rate) {
            return false;
        }
        server_port = port;
        server_started = true;
        return true;
    }
    
    void stop() {
        server_started = false;
    }
    
    bool receivePacket(uint8_t* buffer, size_t* size, uint32_t* src_ip, uint16_t* src_port) {
        if (!server_started || incoming_count == 0) {
            return false;
        }
        
        // Get the oldest packet
        MockPacket& packet = incoming_packets[0];
        *size = packet.size;
        *src_ip = packet.src_ip;
        *src_port = packet.src_port;
        memcpy(buffer, packet.data, packet.size);
        
        // Shift remaining packets
        for (int i = 1; i < incoming_count; i++) {
            incoming_packets[i-1] = incoming_packets[i];
        }
        incoming_count--;
        
        return true;
    }
    
    bool sendPacket(const uint8_t* buffer, size_t size, uint32_t dst_ip, uint16_t dst_port) {
        if (!server_started || (error_rate > 0 && (rand() % 100) < error_rate)) {
            return false;
        }
        
        if (outgoing_count < 10) {
            MockPacket& packet = outgoing_packets[outgoing_count];
            packet.src_ip = dst_ip;  // For tracking purposes
            packet.src_port = dst_port;
            packet.size = size;
            packet.timestamp = getCurrentTime();
            memcpy(packet.data, buffer, (size < 48) ? size : 48);
            outgoing_count++;
        }
        
        return true;
    }
    
    void injectPacket(uint32_t src_ip, uint16_t src_port, const uint8_t* data, size_t size) {
        if (incoming_count < 10) {
            MockPacket& packet = incoming_packets[incoming_count];
            packet.src_ip = src_ip;
            packet.src_port = src_port;
            packet.size = size;
            packet.timestamp = getCurrentTime();
            memcpy(packet.data, data, (size < 48) ? size : 48);
            incoming_count++;
        }
    }
    
    void reset() {
        incoming_count = 0;
        outgoing_count = 0;
        server_started = false;
        server_port = NTP_PORT;
        error_rate = 0;
        processing_delay_us = 100;
        memset(incoming_packets, 0, sizeof(incoming_packets));
        memset(outgoing_packets, 0, sizeof(outgoing_packets));
    }
    
    void setErrorRate(int percentage) {
        error_rate = (percentage > 100) ? 100 : percentage;
    }
    
    void setProcessingDelay(uint32_t delay_us) {
        processing_delay_us = delay_us;
    }
    
    uint64_t getCurrentTime() const {
        static uint64_t time_base = 1640995200ULL * 1000000ULL; // 2022-01-01 in microseconds
        static uint32_t counter = 0;
        return time_base + (++counter * 1000); // Increment by 1ms each call
    }
    
    bool isRunning() const {
        return server_started;
    }
    
    int getIncomingCount() const {
        return incoming_count;
    }
    
    int getOutgoingCount() const {
        return outgoing_count;
    }
    
    const MockPacket* getLastOutgoingPacket() const {
        return (outgoing_count > 0) ? &outgoing_packets[outgoing_count - 1] : nullptr;
    }
};

// Mock Time Source for testing
class MockTimeSource {
public:
    TimeQuality time_quality;
    bool time_available = true;
    int error_rate = 0;
    
    MockTimeSource() {
        reset();
    }
    
    bool getTimeQuality(TimeQuality& quality) {
        if (!time_available || (error_rate > 0 && (rand() % 100) < error_rate)) {
            return false;
        }
        
        quality = time_quality;
        return true;
    }
    
    uint64_t getCurrentNTPTimestamp() {
        if (!time_available) {
            return 0;
        }
        
        static uint64_t base_time = (1640995200ULL + NTP_EPOCH_OFFSET) << 32; // 2022-01-01 in NTP
        static uint32_t counter = 0;
        counter++;
        
        uint32_t seconds = (base_time >> 32) + counter;
        uint32_t fraction = (counter * 1000) << 22; // Simulate sub-second precision
        
        return (static_cast<uint64_t>(seconds) << 32) | fraction;
    }
    
    void reset() {
        time_quality.time_synchronized = true;
        time_quality.current_stratum = 1;
        time_quality.accuracy_ms = 0.1f;
        time_quality.jitter_ms = 0.05f;
        time_quality.last_sync_time = getCurrentNTPTimestamp();
        time_quality.sync_source = 0; // GPS
        time_quality.frequency_offset_ppm = 0.0f;
        time_quality.leap_second_status = 0; // Normal
        
        time_available = true;
        error_rate = 0;
    }
    
    void setTimeAvailable(bool available) {
        time_available = available;
        if (!available) {
            time_quality.time_synchronized = false;
            time_quality.current_stratum = NTP_MAX_STRATUM;
        }
    }
    
    void setStratum(uint8_t stratum) {
        time_quality.current_stratum = stratum;
    }
    
    void setAccuracy(float accuracy_ms) {
        time_quality.accuracy_ms = accuracy_ms;
    }
    
    void setErrorRate(int percentage) {
        error_rate = (percentage > 100) ? 100 : percentage;
    }
};

// Extended NTP Server implementation
class ExtendedNTPServer {
private:
    MockUDPHAL* udp;
    MockTimeSource* time_source;
    NTPStats stats;
    ClientSession client_sessions[10];
    int active_sessions = 0;
    
    // Server configuration
    bool server_enabled = false;
    uint16_t server_port = NTP_PORT;
    uint8_t min_stratum = 1;
    uint8_t max_stratum = 3;
    bool rate_limiting_enabled = true;
    uint32_t max_requests_per_minute = 60;
    bool authentication_required = false;
    uint32_t kiss_of_death_threshold = 100;
    
    // Performance monitoring
    uint64_t last_stats_reset = 0;
    float response_time_samples[10];
    int response_time_index = 0;
    uint32_t processing_time_budget_us = 1000; // 1ms max processing time
    
public:
    ExtendedNTPServer(MockUDPHAL* udp_hal, MockTimeSource* time_src) 
        : udp(udp_hal), time_source(time_src) {
        memset(&stats, 0, sizeof(stats));
        memset(client_sessions, 0, sizeof(client_sessions));
        memset(response_time_samples, 0, sizeof(response_time_samples));
        last_stats_reset = getCurrentTime();
    }
    
    bool begin(uint16_t port = NTP_PORT) {
        server_port = port;
        if (!udp->begin(port)) {
            return false;
        }
        
        server_enabled = true;
        resetStats();
        return true;
    }
    
    void stop() {
        server_enabled = false;
        udp->stop();
    }
    
    void update() {
        if (!server_enabled) {
            return;
        }
        
        uint8_t buffer[48];
        size_t packet_size;
        uint32_t client_ip;
        uint16_t client_port;
        
        // Process incoming packets
        while (udp->receivePacket(buffer, &packet_size, &client_ip, &client_port)) {
            uint64_t receive_time = getCurrentTime();
            processNTPRequest(buffer, packet_size, client_ip, client_port, receive_time);
        }
        
        // Update client sessions
        updateClientSessions();
    }
    
    // Configuration methods
    void setStratumRange(uint8_t min_str, uint8_t max_str) {
        if (min_str >= 1 && max_str <= NTP_MAX_STRATUM && min_str <= max_str) {
            min_stratum = min_str;
            max_stratum = max_str;
        }
    }
    
    void setRateLimiting(bool enabled, uint32_t max_requests = 60) {
        rate_limiting_enabled = enabled;
        max_requests_per_minute = max_requests;
    }
    
    void setAuthenticationRequired(bool required) {
        authentication_required = required;
    }
    
    void setProcessingTimeBudget(uint32_t budget_us) {
        processing_time_budget_us = budget_us;
    }
    
    void setKissOfDeathThreshold(uint32_t threshold) {
        kiss_of_death_threshold = threshold;
    }
    
    // Status getters
    bool isRunning() const {
        return server_enabled && udp->isRunning();
    }
    
    uint16_t getPort() const {
        return server_port;
    }
    
    NTPStats getStats() const {
        return stats;
    }
    
    int getActiveClientCount() const {
        return active_sessions;
    }
    
    float getAverageResponseTime() const {
        return stats.average_response_time_us;
    }
    
    float getMaxResponseTime() const {
        return stats.max_response_time_us;
    }
    
    uint32_t getTotalRequests() const {
        return stats.total_requests;
    }
    
    uint32_t getValidRequests() const {
        return stats.valid_requests;
    }
    
    uint32_t getInvalidRequests() const {
        return stats.invalid_requests;
    }
    
    uint32_t getResponsesSent() const {
        return stats.responses_sent;
    }
    
    uint32_t getRateLimitedRequests() const {
        return stats.rate_limited_requests;
    }
    
    // Get current stratum from time source
    uint8_t getCurrentStratum() {
        TimeQuality quality;
        if (time_source->getTimeQuality(quality)) {
            return quality.current_stratum;
        }
        return NTP_MAX_STRATUM; // Unsynchronized
    }
    
    // Reset statistics
    void resetStats() {
        memset(&stats, 0, sizeof(stats));
        memset(response_time_samples, 0, sizeof(response_time_samples));
        response_time_index = 0;
        last_stats_reset = getCurrentTime();
    }
    
    // Client management
    ClientSession* findClientSession(uint32_t client_ip) {
        for (int i = 0; i < active_sessions; i++) {
            if (client_sessions[i].client_ip == client_ip) {
                return &client_sessions[i];
            }
        }
        return nullptr;
    }
    
    ClientSession* addClientSession(uint32_t client_ip, uint16_t client_port) {
        if (active_sessions >= 10) {
            return nullptr; // No more room
        }
        
        ClientSession* session = &client_sessions[active_sessions];
        active_sessions++;
        
        memset(session, 0, sizeof(ClientSession));
        session->client_ip = client_ip;
        session->client_port = client_port;
        session->last_request_time = getCurrentTime();
        session->poll_interval = 6; // 64 seconds default
        
        stats.client_count = active_sessions;
        return session;
    }
    
private:
    uint64_t getCurrentTime() const {
        static uint64_t time_counter = 1640995200ULL * 1000000ULL; // 2022-01-01 in microseconds
        return time_counter += 1000; // Increment by 1ms
    }
    
    void processNTPRequest(const uint8_t* buffer, size_t size, uint32_t client_ip, 
                          uint16_t client_port, uint64_t receive_time) {
        uint64_t start_time = getCurrentTime();
        
        stats.total_requests++;
        stats.total_bytes_received += size;
        
        // Validate packet size
        if (size < sizeof(NTPPacket)) {
            stats.invalid_requests++;
            return;
        }
        
        const NTPPacket* request = reinterpret_cast<const NTPPacket*>(buffer);
        
        // Validate NTP version and mode
        uint8_t version = (request->li_vn_mode >> 3) & 0x07;
        uint8_t mode = request->li_vn_mode & 0x07;
        
        if (version < 3 || version > 4 || mode != 3) { // Mode 3 = client
            stats.invalid_requests++;
            return;
        }
        
        // Find or create client session
        ClientSession* session = findClientSession(client_ip);
        if (!session) {
            session = addClientSession(client_ip, client_port);
            if (!session) {
                stats.invalid_requests++;
                return; // No session available
            }
        }
        
        // Update session info
        session->request_count++;
        session->last_request_time = receive_time;
        
        // Rate limiting check
        if (rate_limiting_enabled) {
            uint64_t time_window = 60 * 1000000ULL; // 1 minute in microseconds
            uint64_t window_start = receive_time - time_window;
            
            if (session->last_request_time > window_start && 
                session->request_count > max_requests_per_minute) {
                session->rate_limit_violations++;
                stats.rate_limited_requests++;
                
                // Send Kiss-of-Death if threshold exceeded
                if (session->rate_limit_violations > kiss_of_death_threshold) {
                    sendKissOfDeath(client_ip, client_port, request);
                    stats.kiss_of_death_sent++;
                }
                return;
            }
        }
        
        // Get current time quality
        TimeQuality time_quality;
        if (!time_source->getTimeQuality(time_quality)) {
            stats.invalid_requests++;
            return;
        }
        
        // Check if we should serve based on stratum
        uint8_t our_stratum = time_quality.current_stratum;
        if (our_stratum < min_stratum || our_stratum > max_stratum) {
            stats.invalid_requests++;
            return;
        }
        
        stats.valid_requests++;
        
        // Create response packet
        NTPPacket response;
        memset(&response, 0, sizeof(response));
        
        // Set response fields
        uint8_t leap_indicator = (time_quality.leap_second_status << 6) & 0xC0;
        response.li_vn_mode = leap_indicator | (NTP_VERSION << 3) | 0x04; // Mode 4 = server
        response.stratum = our_stratum;
        response.poll = static_cast<int8_t>(session->poll_interval);
        response.precision = -20; // ~1 microsecond precision
        
        // Calculate root delay and dispersion based on time quality
        response.root_delay = htonl(static_cast<uint32_t>(time_quality.accuracy_ms * 65536 / 1000));
        response.root_dispersion = htonl(static_cast<uint32_t>(time_quality.jitter_ms * 65536 / 1000));
        
        // Set reference identifier based on stratum
        if (our_stratum == 1) {
            response.reference_id = htonl(0x47505300); // "GPS\0"
        } else {
            response.reference_id = htonl(client_ip); // Use client IP for higher strata
        }
        
        // Timestamp handling
        uint64_t current_ntp_time = time_source->getCurrentNTPTimestamp();
        response.reference_ts = htonll(time_quality.last_sync_time);
        response.origin_ts = request->transmit_ts; // Echo client's transmit timestamp
        response.receive_ts = htonll(convertToNTPTime(receive_time));
        response.transmit_ts = htonll(current_ntp_time);
        
        // Send response
        if (udp->sendPacket(reinterpret_cast<const uint8_t*>(&response), 
                           sizeof(response), client_ip, client_port)) {
            stats.responses_sent++;
            stats.total_bytes_sent += sizeof(response);
            
            // Track response time
            uint64_t processing_time = getCurrentTime() - start_time;
            updateResponseTimeStats(static_cast<float>(processing_time));
            
            // Update stratum statistics
            switch (our_stratum) {
                case 1: stats.stratum_1_responses++; break;
                case 2: stats.stratum_2_responses++; break;
                case 3: stats.stratum_3_responses++; break;
            }
        }
    }
    
    void sendKissOfDeath(uint32_t client_ip, uint16_t client_port, const NTPPacket* request) {
        NTPPacket response;
        memset(&response, 0, sizeof(response));
        
        response.li_vn_mode = 0xC4; // LI=3 (unsync), VN=4, Mode=4
        response.stratum = 0;       // Kiss-o'-Death
        response.reference_id = htonl(0x52415445); // "RATE"
        response.origin_ts = request->transmit_ts;
        response.transmit_ts = htonll(time_source->getCurrentNTPTimestamp());
        
        udp->sendPacket(reinterpret_cast<const uint8_t*>(&response), 
                       sizeof(response), client_ip, client_port);
    }
    
    void updateClientSessions() {
        uint64_t current_time = getCurrentTime();
        uint64_t session_timeout = 300 * 1000000ULL; // 5 minutes in microseconds
        
        // Remove expired sessions
        for (int i = 0; i < active_sessions; i++) {
            if (current_time - client_sessions[i].last_request_time > session_timeout) {
                // Move last session to this position
                if (i < active_sessions - 1) {
                    client_sessions[i] = client_sessions[active_sessions - 1];
                    i--; // Check this position again
                }
                active_sessions--;
            }
        }
        
        stats.client_count = active_sessions;
    }
    
    void updateResponseTimeStats(float response_time_us) {
        response_time_samples[response_time_index] = response_time_us;
        response_time_index = (response_time_index + 1) % 10;
        
        // Calculate average
        float sum = 0.0f;
        int count = 0;
        for (int i = 0; i < 10; i++) {
            if (response_time_samples[i] > 0.0f) {
                sum += response_time_samples[i];
                count++;
            }
        }
        
        if (count > 0) {
            stats.average_response_time_us = sum / count;
        }
        
        // Update max
        if (response_time_us > stats.max_response_time_us) {
            stats.max_response_time_us = response_time_us;
        }
    }
    
    uint64_t convertToNTPTime(uint64_t microseconds) {
        uint64_t seconds = microseconds / 1000000ULL + NTP_EPOCH_OFFSET;
        uint64_t fraction = ((microseconds % 1000000ULL) << 32) / 1000000ULL;
        return (seconds << 32) | fraction;
    }
    
    uint32_t htonl(uint32_t hostlong) {
        return ((hostlong & 0xFF) << 24) |
               (((hostlong >> 8) & 0xFF) << 16) |
               (((hostlong >> 16) & 0xFF) << 8) |
               ((hostlong >> 24) & 0xFF);
    }
    
    uint64_t htonll(uint64_t hostlonglong) {
        return (static_cast<uint64_t>(htonl(hostlonglong & 0xFFFFFFFF)) << 32) |
               htonl(hostlonglong >> 32);
    }
};

// Global test instances
static MockUDPHAL* mockUDP = nullptr;
static MockTimeSource* mockTimeSource = nullptr;
static ExtendedNTPServer* ntpServer = nullptr;

void setUp(void) {
    mockUDP = new MockUDPHAL();
    mockTimeSource = new MockTimeSource();
    ntpServer = new ExtendedNTPServer(mockUDP, mockTimeSource);
}

void tearDown(void) {
    delete ntpServer;
    delete mockTimeSource;
    delete mockUDP;
    ntpServer = nullptr;
    mockTimeSource = nullptr;
    mockUDP = nullptr;
}

// Helper function to create valid NTP request packet
void createNTPRequest(NTPPacket& packet, uint32_t client_transmit_time = 0) {
    memset(&packet, 0, sizeof(packet));
    packet.li_vn_mode = (NTP_VERSION << 3) | 0x03; // Version 4, Mode 3 (client)
    packet.stratum = 0; // Unspecified
    packet.poll = 6;    // 64 seconds
    packet.precision = -6; // ~15ms
    
    if (client_transmit_time == 0) {
        client_transmit_time = static_cast<uint32_t>(mockTimeSource->getCurrentNTPTimestamp() >> 32);
    }
    packet.transmit_ts = static_cast<uint64_t>(client_transmit_time) << 32;
}

// Basic NTP Server Tests
void test_ntp_server_initialization() {
    TEST_ASSERT_TRUE(ntpServer->begin());
    TEST_ASSERT_TRUE(ntpServer->isRunning());
    TEST_ASSERT_EQUAL_UINT16(NTP_PORT, ntpServer->getPort());
}

void test_ntp_server_initialization_failure() {
    mockUDP->setErrorRate(100);
    TEST_ASSERT_FALSE(ntpServer->begin());
    TEST_ASSERT_FALSE(ntpServer->isRunning());
}

void test_ntp_server_custom_port() {
    TEST_ASSERT_TRUE(ntpServer->begin(8123));
    TEST_ASSERT_TRUE(ntpServer->isRunning());
    TEST_ASSERT_EQUAL_UINT16(8123, ntpServer->getPort());
}

// NTP Request Processing Tests
void test_ntp_server_valid_request() {
    TEST_ASSERT_TRUE(ntpServer->begin());
    
    NTPPacket request;
    createNTPRequest(request);
    
    mockUDP->injectPacket(0xC0A80101, 12345, 
                         reinterpret_cast<const uint8_t*>(&request), 
                         sizeof(request));
    
    ntpServer->update();
    
    TEST_ASSERT_EQUAL_UINT32(1, ntpServer->getTotalRequests());
    TEST_ASSERT_EQUAL_UINT32(1, ntpServer->getValidRequests());
    TEST_ASSERT_EQUAL_UINT32(1, ntpServer->getResponsesSent());
}

void test_ntp_server_invalid_packet_size() {
    TEST_ASSERT_TRUE(ntpServer->begin());
    
    uint8_t short_packet[10] = {0};
    mockUDP->injectPacket(0xC0A80101, 12345, short_packet, sizeof(short_packet));
    
    ntpServer->update();
    
    TEST_ASSERT_EQUAL_UINT32(1, ntpServer->getTotalRequests());
    TEST_ASSERT_EQUAL_UINT32(1, ntpServer->getInvalidRequests());
    TEST_ASSERT_EQUAL_UINT32(0, ntpServer->getResponsesSent());
}

void test_ntp_server_invalid_version() {
    TEST_ASSERT_TRUE(ntpServer->begin());
    
    NTPPacket request;
    createNTPRequest(request);
    request.li_vn_mode = (2 << 3) | 0x03; // Version 2 (invalid)
    
    mockUDP->injectPacket(0xC0A80101, 12345,
                         reinterpret_cast<const uint8_t*>(&request),
                         sizeof(request));
    
    ntpServer->update();
    
    TEST_ASSERT_EQUAL_UINT32(1, ntpServer->getTotalRequests());
    TEST_ASSERT_EQUAL_UINT32(1, ntpServer->getInvalidRequests());
    TEST_ASSERT_EQUAL_UINT32(0, ntpServer->getResponsesSent());
}

void test_ntp_server_invalid_mode() {
    TEST_ASSERT_TRUE(ntpServer->begin());
    
    NTPPacket request;
    createNTPRequest(request);
    request.li_vn_mode = (NTP_VERSION << 3) | 0x04; // Mode 4 (server)
    
    mockUDP->injectPacket(0xC0A80101, 12345,
                         reinterpret_cast<const uint8_t*>(&request),
                         sizeof(request));
    
    ntpServer->update();
    
    TEST_ASSERT_EQUAL_UINT32(1, ntpServer->getTotalRequests());
    TEST_ASSERT_EQUAL_UINT32(1, ntpServer->getInvalidRequests());
    TEST_ASSERT_EQUAL_UINT32(0, ntpServer->getResponsesSent());
}

// Time Source Integration Tests
void test_ntp_server_stratum_1_response() {
    mockTimeSource->setStratum(1);
    TEST_ASSERT_TRUE(ntpServer->begin());
    
    NTPPacket request;
    createNTPRequest(request);
    
    mockUDP->injectPacket(0xC0A80101, 12345,
                         reinterpret_cast<const uint8_t*>(&request),
                         sizeof(request));
    
    ntpServer->update();
    
    TEST_ASSERT_EQUAL_UINT8(1, ntpServer->getCurrentStratum());
    
    NTPStats stats = ntpServer->getStats();
    TEST_ASSERT_EQUAL_UINT32(1, stats.stratum_1_responses);
}

void test_ntp_server_time_source_unavailable() {
    mockTimeSource->setTimeAvailable(false);
    TEST_ASSERT_TRUE(ntpServer->begin());
    
    NTPPacket request;
    createNTPRequest(request);
    
    mockUDP->injectPacket(0xC0A80101, 12345,
                         reinterpret_cast<const uint8_t*>(&request),
                         sizeof(request));
    
    ntpServer->update();
    
    TEST_ASSERT_EQUAL_UINT32(1, ntpServer->getTotalRequests());
    TEST_ASSERT_EQUAL_UINT32(1, ntpServer->getInvalidRequests());
    TEST_ASSERT_EQUAL_UINT32(0, ntpServer->getResponsesSent());
}

// Client Session Management Tests
void test_ntp_server_client_session_tracking() {
    TEST_ASSERT_TRUE(ntpServer->begin());
    
    NTPPacket request;
    createNTPRequest(request);
    
    // Send requests from same client
    mockUDP->injectPacket(0xC0A80101, 12345,
                         reinterpret_cast<const uint8_t*>(&request),
                         sizeof(request));
    mockUDP->injectPacket(0xC0A80101, 12345,
                         reinterpret_cast<const uint8_t*>(&request),
                         sizeof(request));
    
    ntpServer->update();
    
    TEST_ASSERT_EQUAL_INT(1, ntpServer->getActiveClientCount());
    TEST_ASSERT_EQUAL_UINT32(2, ntpServer->getTotalRequests());
}

void test_ntp_server_multiple_clients() {
    TEST_ASSERT_TRUE(ntpServer->begin());
    
    NTPPacket request;
    createNTPRequest(request);
    
    // Send requests from different clients
    mockUDP->injectPacket(0xC0A80101, 12345,
                         reinterpret_cast<const uint8_t*>(&request),
                         sizeof(request));
    mockUDP->injectPacket(0xC0A80102, 12346,
                         reinterpret_cast<const uint8_t*>(&request),
                         sizeof(request));
    mockUDP->injectPacket(0xC0A80103, 12347,
                         reinterpret_cast<const uint8_t*>(&request),
                         sizeof(request));
    
    ntpServer->update();
    
    TEST_ASSERT_EQUAL_INT(3, ntpServer->getActiveClientCount());
    TEST_ASSERT_EQUAL_UINT32(3, ntpServer->getTotalRequests());
    TEST_ASSERT_EQUAL_UINT32(3, ntpServer->getResponsesSent());
}

// Rate Limiting Tests
void test_ntp_server_rate_limiting() {
    ntpServer->setRateLimiting(true, 2); // 2 requests per minute
    TEST_ASSERT_TRUE(ntpServer->begin());
    
    NTPPacket request;
    createNTPRequest(request);
    
    // Send 3 requests rapidly from same client
    for (int i = 0; i < 3; i++) {
        mockUDP->injectPacket(0xC0A80101, 12345,
                             reinterpret_cast<const uint8_t*>(&request),
                             sizeof(request));
        ntpServer->update();
    }
    
    TEST_ASSERT_EQUAL_UINT32(3, ntpServer->getTotalRequests());
    TEST_ASSERT_TRUE(ntpServer->getRateLimitedRequests() > 0);
}

void test_ntp_server_kiss_of_death() {
    ntpServer->setRateLimiting(true, 1);  // 1 request per minute
    ntpServer->setKissOfDeathThreshold(1); // Low threshold for testing
    TEST_ASSERT_TRUE(ntpServer->begin());
    
    NTPPacket request;
    createNTPRequest(request);
    
    // Send many requests to trigger Kiss-of-Death
    for (int i = 0; i < 5; i++) {
        mockUDP->injectPacket(0xC0A80101, 12345,
                             reinterpret_cast<const uint8_t*>(&request),
                             sizeof(request));
        ntpServer->update();
    }
    
    NTPStats stats = ntpServer->getStats();
    TEST_ASSERT_TRUE(stats.kiss_of_death_sent > 0);
}

// Configuration Tests
void test_ntp_server_stratum_range_configuration() {
    ntpServer->setStratumRange(2, 4);
    
    // Test with stratum 1 (outside range)
    mockTimeSource->setStratum(1);
    TEST_ASSERT_TRUE(ntpServer->begin());
    
    NTPPacket request;
    createNTPRequest(request);
    mockUDP->injectPacket(0xC0A80101, 12345,
                         reinterpret_cast<const uint8_t*>(&request),
                         sizeof(request));
    
    ntpServer->update();
    TEST_ASSERT_EQUAL_UINT32(1, ntpServer->getInvalidRequests());
    
    // Test with stratum 3 (within range)
    ntpServer->resetStats();
    mockTimeSource->setStratum(3);
    
    mockUDP->injectPacket(0xC0A80101, 12345,
                         reinterpret_cast<const uint8_t*>(&request),
                         sizeof(request));
    
    ntpServer->update();
    TEST_ASSERT_EQUAL_UINT32(1, ntpServer->getValidRequests());
}

void test_ntp_server_processing_time_budget() {
    ntpServer->setProcessingTimeBudget(500); // 500 microseconds
    TEST_ASSERT_TRUE(ntpServer->begin());
    
    NTPPacket request;
    createNTPRequest(request);
    
    mockUDP->injectPacket(0xC0A80101, 12345,
                         reinterpret_cast<const uint8_t*>(&request),
                         sizeof(request));
    
    ntpServer->update();
    
    // Should still process within budget
    TEST_ASSERT_EQUAL_UINT32(1, ntpServer->getValidRequests());
    TEST_ASSERT_TRUE(ntpServer->getAverageResponseTime() > 0.0f);
}

// Performance and Statistics Tests
void test_ntp_server_response_time_tracking() {
    TEST_ASSERT_TRUE(ntpServer->begin());
    
    NTPPacket request;
    createNTPRequest(request);
    
    for (int i = 0; i < 5; i++) {
        mockUDP->injectPacket(0xC0A80100 + i, 12345,
                             reinterpret_cast<const uint8_t*>(&request),
                             sizeof(request));
        ntpServer->update();
    }
    
    float avg_time = ntpServer->getAverageResponseTime();
    float max_time = ntpServer->getMaxResponseTime();
    
    TEST_ASSERT_TRUE(avg_time > 0.0f);
    TEST_ASSERT_TRUE(max_time >= avg_time);
}

void test_ntp_server_statistics_reset() {
    TEST_ASSERT_TRUE(ntpServer->begin());
    
    NTPPacket request;
    createNTPRequest(request);
    
    mockUDP->injectPacket(0xC0A80101, 12345,
                         reinterpret_cast<const uint8_t*>(&request),
                         sizeof(request));
    ntpServer->update();
    
    TEST_ASSERT_EQUAL_UINT32(1, ntpServer->getTotalRequests());
    
    ntpServer->resetStats();
    
    TEST_ASSERT_EQUAL_UINT32(0, ntpServer->getTotalRequests());
    TEST_ASSERT_EQUAL_FLOAT(0.0f, ntpServer->getAverageResponseTime());
}

// Error Handling Tests
void test_ntp_server_udp_send_failure() {
    TEST_ASSERT_TRUE(ntpServer->begin());
    mockUDP->setErrorRate(100); // Force send failures
    
    NTPPacket request;
    createNTPRequest(request);
    
    mockUDP->injectPacket(0xC0A80101, 12345,
                         reinterpret_cast<const uint8_t*>(&request),
                         sizeof(request));
    
    ntpServer->update();
    
    TEST_ASSERT_EQUAL_UINT32(1, ntpServer->getValidRequests());
    TEST_ASSERT_EQUAL_UINT32(0, ntpServer->getResponsesSent()); // Send failed
}

void test_ntp_server_time_source_error() {
    TEST_ASSERT_TRUE(ntpServer->begin());
    mockTimeSource->setErrorRate(100); // Force time source errors
    
    NTPPacket request;
    createNTPRequest(request);
    
    mockUDP->injectPacket(0xC0A80101, 12345,
                         reinterpret_cast<const uint8_t*>(&request),
                         sizeof(request));
    
    ntpServer->update();
    
    TEST_ASSERT_EQUAL_UINT32(1, ntpServer->getTotalRequests());
    TEST_ASSERT_EQUAL_UINT32(1, ntpServer->getInvalidRequests());
}

// Stress Tests
void test_ntp_server_high_load() {
    TEST_ASSERT_TRUE(ntpServer->begin());
    
    NTPPacket request;
    createNTPRequest(request);
    
    // Simulate high load with many clients
    for (int i = 0; i < 20; i++) {
        mockUDP->injectPacket(0xC0A80100 + (i % 10), 12345 + i,
                             reinterpret_cast<const uint8_t*>(&request),
                             sizeof(request));
    }
    
    ntpServer->update();
    
    // Should handle all requests (limited by client session capacity)
    TEST_ASSERT_EQUAL_UINT32(20, ntpServer->getTotalRequests());
    TEST_ASSERT_TRUE(ntpServer->getValidRequests() > 0);
    TEST_ASSERT_TRUE(ntpServer->getActiveClientCount() <= 10); // Limited by session capacity
}

void test_ntp_server_continuous_operation() {
    TEST_ASSERT_TRUE(ntpServer->begin());
    
    NTPPacket request;
    createNTPRequest(request);
    
    // Simulate continuous operation
    for (int i = 0; i < 100; i++) {
        if (i % 10 == 0) {
            mockUDP->injectPacket(0xC0A80101, 12345,
                                 reinterpret_cast<const uint8_t*>(&request),
                                 sizeof(request));
        }
        ntpServer->update();
    }
    
    // Should remain functional
    TEST_ASSERT_TRUE(ntpServer->isRunning());
    TEST_ASSERT_TRUE(ntpServer->getTotalRequests() > 0);
}

// Integration Tests
void test_ntp_server_complete_transaction() {
    TEST_ASSERT_TRUE(ntpServer->begin());
    
    NTPPacket request;
    createNTPRequest(request);
    
    mockUDP->injectPacket(0xC0A80101, 12345,
                         reinterpret_cast<const uint8_t*>(&request),
                         sizeof(request));
    
    ntpServer->update();
    
    // Verify complete transaction
    TEST_ASSERT_EQUAL_UINT32(1, ntpServer->getTotalRequests());
    TEST_ASSERT_EQUAL_UINT32(1, ntpServer->getValidRequests());
    TEST_ASSERT_EQUAL_UINT32(1, ntpServer->getResponsesSent());
    TEST_ASSERT_EQUAL_INT(1, ntpServer->getActiveClientCount());
    
    // Check that response was actually sent
    TEST_ASSERT_EQUAL_INT(1, mockUDP->getOutgoingCount());
    
    const MockUDPHAL::MockPacket* response = mockUDP->getLastOutgoingPacket();
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_EQUAL_size_t(sizeof(NTPPacket), response->size);
}

int main(void) {
    UNITY_BEGIN();
    
    // Basic NTP Server Tests
    RUN_TEST(test_ntp_server_initialization);
    RUN_TEST(test_ntp_server_initialization_failure);
    RUN_TEST(test_ntp_server_custom_port);
    
    // NTP Request Processing Tests
    RUN_TEST(test_ntp_server_valid_request);
    RUN_TEST(test_ntp_server_invalid_packet_size);
    RUN_TEST(test_ntp_server_invalid_version);
    RUN_TEST(test_ntp_server_invalid_mode);
    
    // Time Source Integration Tests
    RUN_TEST(test_ntp_server_stratum_1_response);
    RUN_TEST(test_ntp_server_time_source_unavailable);
    
    // Client Session Management Tests
    RUN_TEST(test_ntp_server_client_session_tracking);
    RUN_TEST(test_ntp_server_multiple_clients);
    
    // Rate Limiting Tests
    RUN_TEST(test_ntp_server_rate_limiting);
    RUN_TEST(test_ntp_server_kiss_of_death);
    
    // Configuration Tests
    RUN_TEST(test_ntp_server_stratum_range_configuration);
    RUN_TEST(test_ntp_server_processing_time_budget);
    
    // Performance and Statistics Tests
    RUN_TEST(test_ntp_server_response_time_tracking);
    RUN_TEST(test_ntp_server_statistics_reset);
    
    // Error Handling Tests
    RUN_TEST(test_ntp_server_udp_send_failure);
    RUN_TEST(test_ntp_server_time_source_error);
    
    // Stress Tests
    RUN_TEST(test_ntp_server_high_load);
    RUN_TEST(test_ntp_server_continuous_operation);
    
    // Integration Tests
    RUN_TEST(test_ntp_server_complete_transaction);
    
    return UNITY_END();
}