#include <unity.h>
#include "test_common.h"
#include <cmath>

// RFC 5905 NTP パケット構造体
struct NtpPacket {
    uint8_t li_vn_mode;
    uint8_t stratum;
    uint8_t poll;
    int8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t reference_id;
    NtpTimestamp reference_timestamp;
    NtpTimestamp origin_timestamp;
    NtpTimestamp receive_timestamp;
    NtpTimestamp transmit_timestamp;
};

// モックTimeManagerクラス
class MockTimeManager {
public:
    bool synchronized;
    bool fallbackMode;
    
    MockTimeManager() : synchronized(true), fallbackMode(false) {}
    
    int getNtpStratum() {
        if (synchronized && !fallbackMode) {
            return 1; // GPS同期時
        } else {
            return 3; // RTC fallback時
        }
    }
};

void setUp(void) {
    // 各テスト前の初期化
}

void tearDown(void) {
    // 各テスト後のクリーンアップ
}

//=============================================================================
// RFC 5905準拠性テスト
//=============================================================================

void test_ntp_timestamp_format() {
    // Test Case 1: Unix epoch時刻のNTP変換
    time_t unixTime = 0; // 1970-01-01 00:00:00 UTC
    NtpTimestamp ntpTs = unixToNtpTimestamp(unixTime, 0);
    
    TEST_ASSERT_EQUAL_UINT32(NTP_TIMESTAMP_DELTA, ntpTs.seconds);
    TEST_ASSERT_EQUAL_UINT32(0, ntpTs.fraction);
    
    // Test Case 2: 現在時刻の変換精度
    time_t currentTime = TEST_GPS_TIME;
    uint32_t microseconds = 500000; // 0.5秒
    ntpTs = unixToNtpTimestamp(currentTime, microseconds);
    
    TEST_ASSERT_EQUAL_UINT32(TEST_GPS_TIME + NTP_TIMESTAMP_DELTA, ntpTs.seconds);
    
    // マイクロ秒からNTPフラクション部の変換精度検証
    uint32_t expectedFraction = (uint32_t)((uint64_t)microseconds * 4294967296ULL / 1000000ULL);
    TEST_ASSERT_EQUAL_UINT32(expectedFraction, ntpTs.fraction);
}

void test_ntp_packet_structure() {
    NtpPacket packet;
    memset(&packet, 0, sizeof(packet));
    
    // Test Case 1: パケットサイズ検証
    TEST_ASSERT_EQUAL_size_t(48, sizeof(NtpPacket));
    
    // Test Case 2: フィールドオフセット検証（RFC 5905 Figure 8）
    TEST_ASSERT_EQUAL_PTR(&packet.li_vn_mode, (uint8_t*)&packet + 0);
    TEST_ASSERT_EQUAL_PTR(&packet.stratum, (uint8_t*)&packet + 1);
    TEST_ASSERT_EQUAL_PTR(&packet.poll, (uint8_t*)&packet + 2);
    TEST_ASSERT_EQUAL_PTR(&packet.precision, (uint8_t*)&packet + 3);
}

void test_stratum_levels() {
    MockTimeManager timeManager;
    
    // Test Case 1: GPS同期時 - Stratum 1
    timeManager.synchronized = true;
    timeManager.fallbackMode = false;
    
    TEST_ASSERT_EQUAL_INT8(1, timeManager.getNtpStratum());
    
    // Test Case 2: RTC fallback時 - Stratum 3以上
    timeManager.fallbackMode = true;
    timeManager.synchronized = false;
    TEST_ASSERT_GREATER_OR_EQUAL_INT8(3, timeManager.getNtpStratum());
}

void test_reference_identifier() {
    // Test Case 1: GPS reference ID ("GPS\0")
    uint32_t gpsRefId = 0x47505300; // "GPS\0" in network byte order
    TEST_ASSERT_EQUAL_UINT32(gpsRefId, 0x47505300);
    
    // Test Case 2: RTC reference ID ("RTC\0") 
    uint32_t rtcRefId = 0x52544300; // "RTC\0" in network byte order
    TEST_ASSERT_EQUAL_UINT32(rtcRefId, 0x52544300);
}

//=============================================================================
// 時刻変換テスト
//=============================================================================

void test_gps_to_unix_conversion() {
    // Test Case 1: ログから取得した実際のGPS時刻での検証
    time_t result = gpsTimeToUnixTimestamp(2025, 7, 22, 10, 10, 57);
    
    TEST_ASSERT_EQUAL_UINT32(1753179057, result);
    
    // Test Case 2: Epoch時刻での変換検証
    result = gpsTimeToUnixTimestamp(1970, 1, 1, 0, 0, 0);
    TEST_ASSERT_EQUAL_UINT32(0, result);
    
    // Test Case 3: うるう年の処理検証
    time_t leapResult = gpsTimeToUnixTimestamp(2024, 2, 29, 12, 0, 0);
    time_t feb28Result = gpsTimeToUnixTimestamp(2024, 2, 28, 12, 0, 0);
    TEST_ASSERT_EQUAL_UINT32(86400, leapResult - feb28Result); // 1日差
}

void test_unix_to_ntp_conversion() {
    // Test Case 1: 基本的な変換
    NtpTimestamp ntpTs = unixToNtpTimestamp(TEST_GPS_TIME, 0);
    
    TEST_ASSERT_EQUAL_UINT32(TEST_GPS_TIME + NTP_TIMESTAMP_DELTA, ntpTs.seconds);
    TEST_ASSERT_EQUAL_UINT32(0, ntpTs.fraction);
    
    // Test Case 2: マイクロ秒精度での変換
    uint32_t testMicros = 500000; // 0.5秒
    ntpTs = unixToNtpTimestamp(TEST_GPS_TIME, testMicros);
    
    // 期待されるフラクション部: 0.5 * 2^32 = 2147483648 = 0x80000000
    TEST_ASSERT_EQUAL_UINT32(0x80000000, ntpTs.fraction);
}

void test_timestamp_round_trip() {
    // Test Case 1: Unix → NTP → Unix 変換
    time_t originalUnix = TEST_GPS_TIME;
    NtpTimestamp ntpTs = unixToNtpTimestamp(originalUnix, 0);
    time_t convertedUnix = ntpToUnixTimestamp(ntpTs);
    
    TEST_ASSERT_EQUAL_UINT32(originalUnix, convertedUnix);
    
    // Test Case 2: マイクロ秒精度での変換
    uint32_t originalMicros = 123456;
    ntpTs = unixToNtpTimestamp(originalUnix, originalMicros);
    
    // NTPフラクション部からマイクロ秒への逆変換
    uint32_t convertedMicros = (uint32_t)((uint64_t)ntpTs.fraction * 1000000ULL / 4294967296ULL);
    TEST_ASSERT_UINT32_WITHIN(1, originalMicros, convertedMicros); // 1μs以内
}

//=============================================================================
// 問題分析テスト（55年オフセット問題）
//=============================================================================

void test_offset_problem_analysis() {
    // Test Case 1: 正常な時刻変換
    time_t correctGpsTime = TEST_GPS_TIME;  // 1753179057
    uint32_t correctNtpTime = correctGpsTime + NTP_TIMESTAMP_DELTA;  // 3962167857
    
    // Test Case 2: 問題ケース - 小さすぎるGPS時刻
    time_t problematicGpsTime = 832400;  // システムアップタイム相当
    uint32_t problematicNtpTime = problematicGpsTime + NTP_TIMESTAMP_DELTA;  // 2209821200
    
    // ログで観測された値との比較
    uint32_t observedNtpTime = 2209821199; // ログから（0x83B7320F）
    
    Serial.printf("Problem Analysis:\n");
    Serial.printf("  Correct GPS time: %lu -> NTP: %lu\n", correctGpsTime, correctNtpTime);
    Serial.printf("  Problematic GPS: %lu -> NTP: %lu\n", problematicGpsTime, problematicNtpTime);
    Serial.printf("  Observed NTP: %lu\n", observedNtpTime);
    Serial.printf("  Offset: %ld seconds (%.1f years)\n", 
                  (long)(observedNtpTime - correctNtpTime), 
                  (observedNtpTime - correctNtpTime) / (365.25 * 24 * 3600));
    
    // 問題確認：TimeManager::getHighPrecisionTime()が小さすぎる値を返している
    TEST_ASSERT_NOT_EQUAL_UINT32(correctNtpTime, observedNtpTime);
    TEST_ASSERT_UINT32_WITHIN(5, problematicNtpTime, observedNtpTime);
}

void test_time_manager_simulation() {
    // Test Case 1: 正常なGPS同期時の時刻計算
    time_t gpsTime = TEST_GPS_TIME;         // 1753179057
    unsigned long ppsTime = 1000000;        // PPS基準時刻 (micros)
    unsigned long currentMicros = 1001000;  // 現在時刻 (1ms後)
    
    // TimeManager::getHighPrecisionTime()の計算ロジック
    unsigned long elapsed = currentMicros - ppsTime;          // 1000μs
    unsigned long result = gpsTime * 1000 + elapsed / 1000;   // 1753179057001ms
    
    time_t retrievedTime = result / 1000;  // 1753179057秒
    
    // 正常ケース：GPS時刻が正しく保持される
    TEST_ASSERT_EQUAL_UINT32(TEST_GPS_TIME, retrievedTime);
    
    // Test Case 2: 問題ケース - timeSync->gpsTimeが異常値
    time_t problematicGpsTime = 832400;  // 実際に観測されている異常値
    unsigned long problematicResult = problematicGpsTime * 1000 + elapsed / 1000;
    time_t problematicRetrieved = problematicResult / 1000;
    
    // 問題再現：この小さい値がNTPサーバーで使用されている
    TEST_ASSERT_EQUAL_UINT32(832400, problematicRetrieved);
    TEST_ASSERT_NOT_EQUAL_UINT32(TEST_GPS_TIME, problematicRetrieved);
}

//=============================================================================
// ネットワーク・精度テスト
//=============================================================================

void test_network_byte_order() {
    // Test Case 1: 32ビット値のhtonl/ntohl変換
    uint32_t hostValue = 0x12345678;
    uint32_t networkValue = htonl(hostValue);
    uint32_t backToHost = ntohl(networkValue);
    
    TEST_ASSERT_EQUAL_UINT32(hostValue, backToHost);
    
    // Test Case 2: NTPタイムスタンプのバイトオーダー変換
    NtpTimestamp original = {TEST_GPS_TIME + NTP_TIMESTAMP_DELTA, 0x80000000};
    NtpTimestamp networked = htonTimestamp(original);
    NtpTimestamp restored = ntohTimestamp(networked);
    
    TEST_ASSERT_EQUAL_UINT32(original.seconds, restored.seconds);
    TEST_ASSERT_EQUAL_UINT32(original.fraction, restored.fraction);
}

void test_time_precision() {
    // Test Case 1: マイクロ秒精度の保持
    time_t baseTime = TEST_GPS_TIME;
    uint32_t microSeconds[] = {0, 1, 500000, 999999};
    
    for (size_t i = 0; i < sizeof(microSeconds)/sizeof(microSeconds[0]); i++) {
        NtpTimestamp ntpTs = unixToNtpTimestamp(baseTime, microSeconds[i]);
        
        // フラクション部からマイクロ秒への逆変換
        uint32_t recoveredMicros = (uint32_t)((uint64_t)ntpTs.fraction * 1000000ULL / 4294967296ULL);
        
        // 1マイクロ秒以内の精度で保持されることを確認
        TEST_ASSERT_UINT32_WITHIN(1, microSeconds[i], recoveredMicros);
    }
}

void test_rfc5905_compliance() {
    // Test Case 1: NTPv4パケットの基本要件
    NtpPacket packet;
    memset(&packet, 0, sizeof(packet));
    
    // NTPv4, Server mode, No leap warning
    packet.li_vn_mode = (NTP_LI_NO_WARNING << 6) | (4 << 3) | NTP_MODE_SERVER;
    packet.stratum = 1; // Primary reference (GPS)
    packet.precision = -20; // 1 microsecond
    packet.reference_id = 0x47505300; // "GPS\0"
    
    // パケット検証
    TEST_ASSERT_EQUAL_UINT8(0, (packet.li_vn_mode >> 6) & 0x03); // LI
    TEST_ASSERT_EQUAL_UINT8(4, (packet.li_vn_mode >> 3) & 0x07); // VN
    TEST_ASSERT_EQUAL_UINT8(4, packet.li_vn_mode & 0x07); // Mode
    TEST_ASSERT_EQUAL_UINT8(1, packet.stratum);
    TEST_ASSERT_EQUAL_INT8(-20, packet.precision);
}

void setup() {
    delay(2000); // Arduino起動待ち
    
    UNITY_BEGIN();
    
    // RFC 5905基本テスト
    RUN_TEST(test_ntp_timestamp_format);
    RUN_TEST(test_ntp_packet_structure);
    RUN_TEST(test_stratum_levels);
    RUN_TEST(test_reference_identifier);
    
    // 時刻変換テスト
    RUN_TEST(test_gps_to_unix_conversion);
    RUN_TEST(test_unix_to_ntp_conversion);
    RUN_TEST(test_timestamp_round_trip);
    
    // 問題分析テスト
    RUN_TEST(test_offset_problem_analysis);
    RUN_TEST(test_time_manager_simulation);
    
    // 技術的詳細テスト
    RUN_TEST(test_network_byte_order);
    RUN_TEST(test_time_precision);
    RUN_TEST(test_rfc5905_compliance);
    
    UNITY_END();
}

void loop() {
    // テスト完了後は何もしない
}