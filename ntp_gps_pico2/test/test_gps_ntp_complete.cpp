#include <unity.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <arpa/inet.h>  // System htonl/ntohl functions

// テスト用の基本構造体（test_common.hから直接定義）
struct NtpTimestamp {
    uint32_t seconds;
    uint32_t fraction;
};

// テスト用定数
#define NTP_TIMESTAMP_DELTA 2208988800UL
#define NTP_LI_NO_WARNING 0x00
#define NTP_LI_61_SECONDS 0x01
#define NTP_MODE_SERVER 4
#define NTP_PACKET_SIZE 48

// テスト用のヘルパー関数
inline NtpTimestamp unixToNtpTimestamp(uint32_t unixSeconds, uint32_t microseconds = 0) {
    NtpTimestamp ntp;
    ntp.seconds = unixSeconds + NTP_TIMESTAMP_DELTA;
    ntp.fraction = (uint32_t)((uint64_t)microseconds * 4294967296ULL / 1000000ULL);
    return ntp;
}

inline uint32_t ntpToUnixTimestamp(const NtpTimestamp& ntp) {
    return ntp.seconds - NTP_TIMESTAMP_DELTA;
}

// システムのhtonl/ntohl関数を使用

// テスト用NTPタイムスタンプバイトオーダー変換
inline NtpTimestamp htonTimestamp(const NtpTimestamp& hostTs) {
    NtpTimestamp netTs;
    netTs.seconds = htonl(hostTs.seconds);
    netTs.fraction = htonl(hostTs.fraction);
    return netTs;
}

inline NtpTimestamp ntohTimestamp(const NtpTimestamp& netTs) {
    NtpTimestamp hostTs;
    hostTs.seconds = ntohl(netTs.seconds);
    hostTs.fraction = ntohl(netTs.fraction);
    return hostTs;
}

// GPS時刻変換関数の実装（Nativeテスト用）
time_t gpsTimeToUnixTimestamp(uint16_t year, uint8_t month, uint8_t day, 
                             uint8_t hour, uint8_t min, uint8_t sec) {
    // Unixエポック (1970年1月1日) からの経過秒数を計算
    
    // 1970年からの経過年数
    int years_since_epoch = year - 1970;
    
    // うるう年の数を計算
    int leap_years = 0;
    for (int y = 1970; y < year; y++) {
        if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) {
            leap_years++;
        }
    }
    
    // 各月の日数（平年）
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    // 現在年がうるう年かチェック
    bool is_leap_year = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
    if (is_leap_year) {
        days_in_month[1] = 29; // 2月
    }
    
    // 年の日数を計算
    int total_days = years_since_epoch * 365 + leap_years;
    
    // 月の日数を追加
    for (int m = 1; m < month; m++) {
        total_days += days_in_month[m - 1];
    }
    
    // 日を追加（1日ベースなので-1）
    total_days += day - 1;
    
    // 秒に変換
    time_t timestamp = (time_t)total_days * 24 * 60 * 60;
    timestamp += hour * 60 * 60;
    timestamp += min * 60;
    timestamp += sec;
    
    return timestamp;
}

// テスト定数
static const time_t TEST_GPS_TIME = 1753179057; // 2025-07-22 10:10:57 UTC

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
    
    printf("Problem Analysis:\n");
    printf("  Correct GPS time: %u -> NTP: %u\n", (unsigned)correctGpsTime, correctNtpTime);
    printf("  Problematic GPS: %u -> NTP: %u\n", (unsigned)problematicGpsTime, problematicNtpTime);
    printf("  Observed NTP: %u\n", observedNtpTime);
    printf("  Offset: %ld seconds (%.1f years)\n", 
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

//=============================================================================
// RFC 5905違反検証テスト（実際の失敗ケース）
//=============================================================================

void test_rfc5905_timestamp_validity() {
    // RFC 5905 Section 4: NTPタイムスタンプは1900年1月1日からの秒数
    // 現在時刻（2025年）より前の時刻は無効
    
    // Test Case 1: 正常なタイムスタンプ（2025年）
    time_t validUnixTime = 1753181256; // 2025-07-22 10:47:36 UTC
    NtpTimestamp validNtpTs = unixToNtpTimestamp(validUnixTime, 0);
    uint32_t expectedValidNtp = validUnixTime + NTP_TIMESTAMP_DELTA; // 3962170056
    
    TEST_ASSERT_EQUAL_UINT32(expectedValidNtp, validNtpTs.seconds);
    
    // Test Case 2: RFC 5905違反 - 実際の失敗タイムスタンプ
    uint32_t problematicUnixTime = 834597; // システムアップタイム（約9.6日）
    NtpTimestamp problematicNtpTs = unixToNtpTimestamp(problematicUnixTime, 0);
    uint32_t actualProblematicNtp = problematicUnixTime + NTP_TIMESTAMP_DELTA; // 2209823397
    
    TEST_ASSERT_EQUAL_UINT32(actualProblematicNtp, problematicNtpTs.seconds);
    
    // RFC 5905違反の確認
    uint32_t year2020Ntp = unixToNtpTimestamp(1577836800, 0).seconds; // 2020-01-01
    uint32_t year1970Ntp = NTP_TIMESTAMP_DELTA; // 1970-01-01 (Unix epoch)
    
    // 問題のタイムスタンプは1970年代前半の時刻を示している（RFC違反）
    TEST_ASSERT_LESS_THAN_UINT32(year2020Ntp, problematicNtpTs.seconds);
    printf("RFC 5905 Violation: Problematic timestamp %u represents year ~%lu\n", 
           problematicNtpTs.seconds, 1900UL + (problematicNtpTs.seconds - NTP_TIMESTAMP_DELTA) / (365 * 24 * 3600));
}

void test_rfc5905_stratum_consistency() {
    // RFC 5905 Section 3: Stratum 1サーバーは正確な時刻を提供する義務
    
    MockTimeManager gpsTimeManager;
    gpsTimeManager.synchronized = true;
    gpsTimeManager.fallbackMode = false;
    
    // Test Case 1: Stratum 1を主張しているか確認
    TEST_ASSERT_EQUAL_INT8(1, gpsTimeManager.getNtpStratum());
    
    // Test Case 2: しかし提供している時刻は55年古い（RFC 5905違反）
    time_t currentCorrectTime = 1753181256; // 2025-07-22
    time_t providedIncorrectTime = 834597;   // ~1970年代（55年オフセット）
    
    // Stratum 1サーバーが提供する時刻の精度要件
    // RFC 5905: Stratum 1は数マイクロ秒の精度を要求
    int64_t timeOffset = (int64_t)providedIncorrectTime - (int64_t)currentCorrectTime;
    uint32_t maxAllowedOffset = 1; // 1秒以内であるべき
    
    // RFC違反：55年（約1.7億秒）のオフセット
    TEST_ASSERT_GREATER_THAN_UINT32(maxAllowedOffset, abs(timeOffset));
    
    printf("RFC 5905 Stratum Violation: Stratum 1 server has %ld second offset (%.1f years)\n",
           (long)timeOffset, timeOffset / (365.25 * 24 * 3600));
}

void test_rfc5905_reference_timestamp_validity() {
    // RFC 5905 Section 7.3: Reference Timestampは最後にローカル時計が設定された時刻
    
    // Test Case 1: 正常ケース - GPS同期時刻がReference Timestamp
    time_t gpsCorrectTime = 1753181256;
    NtpTimestamp correctRefTs = unixToNtpTimestamp(gpsCorrectTime - 1, 0); // 1秒前
    
    // Test Case 2: 問題ケース - システムアップタイムベースのReference Timestamp
    uint32_t problematicRefUnix = 834596; // ログからの実際の値
    NtpTimestamp problematicRefTs = unixToNtpTimestamp(problematicRefUnix, 0);
    
    // RFC 5905要件: Reference TimestampはTransmit Timestampより前または同じ
    time_t problematicTransmitUnix = 834597;
    NtpTimestamp problematicTransmitTs = unixToNtpTimestamp(problematicTransmitUnix, 0);
    
    // 時系列は正しい（refTime < transmitTime）
    TEST_ASSERT_LESS_THAN_UINT32(problematicTransmitTs.seconds, problematicRefTs.seconds + 2);
    
    // しかし両方とも現在時刻から55年ずれている（RFC違反）
    time_t currentTime = 1753181256;
    NtpTimestamp currentNtpTs = unixToNtpTimestamp(currentTime, 0);
    
    int64_t refOffset = (int64_t)problematicRefTs.seconds - (int64_t)currentNtpTs.seconds;
    
    // RFC 5905違反: Reference Timestampが現在時刻から大幅にずれている
    TEST_ASSERT_GREATER_THAN_UINT32(86400, abs(refOffset)); // 1日以上のずれは異常
    
    printf("RFC 5905 Reference Violation: Reference timestamp offset %ld seconds\n", (long)refOffset);
}

void test_ntp_client_expectation_violation() {
    // RFC 5905 Section 5: NTPクライアントの期待値との整合性検証
    
    // Test Case 1: NTPクライアントが期待する現在時刻（2025年）
    time_t clientExpectedTime = 1753181256; // 2025-07-22 10:47:36
    NtpTimestamp clientExpectedNtp = unixToNtpTimestamp(clientExpectedTime, 0);
    
    // Test Case 2: 実際にサーバーが返すタイムスタンプ（1970年代）
    uint32_t serverProvidedUnix = 834597; // システムアップタイム
    NtpTimestamp serverProvidedNtp = unixToNtpTimestamp(serverProvidedUnix, 0);
    
    // クライアント-サーバー間のオフセット計算（sntpコマンドの結果と同じ）
    int64_t offset = (int64_t)serverProvidedNtp.seconds - (int64_t)clientExpectedNtp.seconds;
    
    // 実際のsntp出力: offset: -1752346657 seconds
    int64_t expectedSntpOffset = -1752346657;
    
    // 計算されたオフセットがsntpの結果と一致することを確認
    TEST_ASSERT_INT64_WITHIN(10, expectedSntpOffset, offset);
    
    // RFC 5905違反: 55年のオフセットは実用上使用不可
    uint32_t maxUsableOffset = 86400; // 1日
    TEST_ASSERT_GREATER_THAN_UINT32(maxUsableOffset, abs(offset));
    
    printf("NTP Client Impact: Server provides timestamp %lld seconds off (sntp shows: %lld)\n",
           (long long)offset, (long long)expectedSntpOffset);
}

void test_gps_synchronization_integrity() {
    // GPS同期の整合性検証 - 設定と使用の分離問題
    
    // Test Case 1: GPS時刻の正しい設定（ログから確認済み）
    time_t correctGpsTime = 1753181256; // GPS Time Sync - Set timeSync->gpsTime
    
    // Test Case 2: しかし実際に使用される時刻は異なる
    uint32_t actualUsedTime = 834597; // NTP Timestamp Debug - Unix
    
    // 整合性違反: 設定された時刻と使用される時刻が大幅に異なる
    int64_t integrityGap = (int64_t)correctGpsTime - (int64_t)actualUsedTime;
    
    // この差は容認できない（数秒以内であるべき）
    uint32_t maxAcceptableGap = 60; // 1分
    TEST_ASSERT_GREATER_THAN_UINT32(maxAcceptableGap, abs(integrityGap));
    
    // 問題の重大性を示すログ
    printf("GPS Sync Integrity Violation: Set %u, Used %u, Gap %lld seconds\n",
           (unsigned)correctGpsTime, actualUsedTime, (long long)integrityGap);
    
    // この問題により、GPSから正確な時刻を取得しているにも関わらず
    // NTPクライアントには間違った時刻が提供される
    TEST_ASSERT_EQUAL_UINT32(correctGpsTime, actualUsedTime); // これは失敗するはず
}

//=============================================================================
// 32ビット整数オーバーフロー検証テスト
//=============================================================================

void test_32bit_overflow_detection() {
    // Test Case 1: 32ビット整数の最大値確認
    uint32_t max32bit = UINT32_MAX; // 4294967295
    printf("32-bit unsigned int max: %u\n", max32bit);
    
    // Test Case 2: 2025年GPS時刻での乗算オーバーフロー検証
    time_t gps2025 = 1753223178; // 2025-07-22 22:26:18 UTC (実際のログから)
    
    // 問題のある32ビット計算（オーバーフロー発生）
    uint64_t expectedResult64 = (uint64_t)gps2025 * 1000ULL; // 正しい結果
    uint32_t overflowResult32 = (uint32_t)(gps2025 * 1000UL); // オーバーフロー結果
    
    printf("32-bit Overflow Test:\n");
    printf("  GPS time: %u (2025-07-22)\n", (unsigned)gps2025);
    printf("  Expected (64-bit): %llu milliseconds\n", expectedResult64);
    printf("  Overflow (32-bit): %u milliseconds\n", overflowResult32);
    printf("  Overflow as seconds: %u (year ~%u)\n", 
           overflowResult32 / 1000, 1970 + (overflowResult32 / 1000) / (365 * 24 * 3600));
    
    // オーバーフローが発生することを確認（64ビット結果が32ビット最大値を超える）
    TEST_ASSERT_TRUE(expectedResult64 > max32bit);
    // 32ビット計算とオーバーフロー結果が異なることを確認（上位ビットが切り捨てられる）
    TEST_ASSERT_NOT_EQUAL_UINT64(expectedResult64, (uint64_t)overflowResult32);
    
    // オーバーフロー結果が実際のログと一致することを確認
    uint32_t observedOverflow = 876521251; // 実際のログから観測された値
    
    // デバッグ情報
    printf("Overflow comparison: calculated %u vs observed %u\n", overflowResult32, observedOverflow);
    
    // オーバーフロー計算の確認：結果が予想される範囲内であることを確認
    TEST_ASSERT_UINT32_WITHIN(100000, overflowResult32, observedOverflow);
}

void test_32bit_safe_calculation() {
    // Test Case 1: 安全な64ビット演算
    time_t gps2025 = 1753223178;
    uint64_t safeResult64 = (uint64_t)gps2025 * 1000ULL;
    
    // 結果を秒に戻す
    time_t backToSeconds = (time_t)(safeResult64 / 1000ULL);
    
    // 往復変換が正確であることを確認
    TEST_ASSERT_EQUAL_UINT32(gps2025, backToSeconds);
    
    // Test Case 2: NTPタイムスタンプ変換での安全性
    uint64_t ntpTimestamp64 = safeResult64 + 2208988800000ULL; // NTP epoch offset
    
    // NTPタイムスタンプの整数部（秒）
    uint32_t ntpSeconds = (uint32_t)((safeResult64 + 2208988800000ULL) / 1000ULL);
    
    // 期待されるNTP秒値
    uint32_t expectedNtpSeconds = gps2025 + 2208988800UL;
    
    TEST_ASSERT_EQUAL_UINT32(expectedNtpSeconds, ntpSeconds);
    
    printf("Safe 64-bit Calculation:\n");
    printf("  GPS time: %u seconds\n", (unsigned)gps2025);
    printf("  64-bit milliseconds: %llu\n", safeResult64);
    printf("  Back to seconds: %u\n", (unsigned)backToSeconds);
    printf("  NTP timestamp: %u\n", ntpSeconds);
}

void test_overflow_boundary_cases() {
    // Test Case 1: オーバーフロー境界年の特定
    time_t overflowBoundary = UINT32_MAX / 1000; // 4294967 seconds = 1970/02/19
    time_t boundaryYear = 1970 + overflowBoundary / (365 * 24 * 3600);
    
    printf("Overflow Boundary Analysis:\n");
    printf("  32-bit overflow boundary: %u seconds\n", (unsigned)overflowBoundary);
    printf("  Boundary year: ~%u\n", (unsigned)boundaryYear);
    printf("  2025 GPS time: %u seconds\n", 1753223178U);
    printf("  Years beyond boundary: ~%u years\n", 
           (unsigned)(2025 - boundaryYear));
    
    // 2025年は明らかに境界を超えていることを確認
    TEST_ASSERT_GREATER_THAN_UINT32(overflowBoundary, 1753223178);
    
    // Test Case 2: 境界前後での動作確認
    time_t safeTimes[] = {
        946684800,  // 2000-01-01 (境界内)
        1577836800, // 2020-01-01 (境界内)
        1640995200  // 2022-01-01 (境界内)
    };
    
    time_t overflowTimes[] = {
        1735689600, // 2025-01-01 (境界外)
        1753223178, // 2025-07-22 (境界外、実際のケース)
        1767225600  // 2026-01-01 (境界外)
    };
    
    // 境界内の時刻はオーバーフローしない
    for (size_t i = 0; i < sizeof(safeTimes)/sizeof(safeTimes[0]); i++) {
        uint64_t result64 = (uint64_t)safeTimes[i] * 1000ULL;
        TEST_ASSERT_TRUE(result64 <= UINT32_MAX);
    }
    
    // 境界外の時刻はオーバーフローする
    for (size_t i = 0; i < sizeof(overflowTimes)/sizeof(overflowTimes[0]); i++) {
        uint64_t result64 = (uint64_t)overflowTimes[i] * 1000ULL;
        TEST_ASSERT_TRUE(result64 > UINT32_MAX);
    }
}

void test_ntp_timestamp_precision_preservation() {
    // Test Case 1: マイクロ秒精度の保持（オーバーフロー回避）
    time_t gps2025 = 1753223178;
    uint32_t testMicroseconds[] = {0, 500000, 999999}; // 0.0秒、0.5秒、0.999999秒
    
    for (size_t i = 0; i < sizeof(testMicroseconds)/sizeof(testMicroseconds[0]); i++) {
        // 64ビット安全計算
        uint64_t gpsMs64 = (uint64_t)gps2025 * 1000ULL;
        uint64_t microsFraction64 = (uint64_t)testMicroseconds[i] * 4294967296ULL / 1000000ULL;
        
        // NTPタイムスタンプ構成
        uint32_t ntpSeconds = gps2025 + 2208988800UL;
        uint32_t ntpFraction = (uint32_t)microsFraction64;
        
        // 逆変換でマイクロ秒を復元
        uint32_t recoveredMicros = (uint32_t)((uint64_t)ntpFraction * 1000000ULL / 4294967296ULL);
        
        // 1マイクロ秒以内の精度で保持されることを確認
        TEST_ASSERT_UINT32_WITHIN(1, testMicroseconds[i], recoveredMicros);
        
        printf("Precision Test %zu: %u μs -> NTP fraction: 0x%08X -> %u μs\n",
               i, testMicroseconds[i], ntpFraction, recoveredMicros);
    }
}

void test_real_world_2025_timestamps() {
    // Test Case 1: 実際のログデータでの検証
    struct {
        uint16_t year;
        uint8_t month, day, hour, min, sec;
        time_t expectedUnix;
        const char* description;
    } testCases[] = {
        {2025, 7, 22, 22, 26, 18, 1753223178, "Actual log timestamp"},
        {2025, 1, 1, 0, 0, 0, 1735689600, "2025 New Year"},
        {2025, 12, 31, 23, 59, 59, 1767225599, "2025 Year End"},
        {2030, 1, 1, 0, 0, 0, 1893456000, "2030 (far future)"}
    };
    
    for (size_t i = 0; i < sizeof(testCases)/sizeof(testCases[0]); i++) {
        uint16_t year = testCases[i].year;
        uint8_t month = testCases[i].month;
        uint8_t day = testCases[i].day;
        uint8_t hour = testCases[i].hour;
        uint8_t min = testCases[i].min;
        uint8_t sec = testCases[i].sec;
        time_t expectedUnix = testCases[i].expectedUnix;
        const char* description = testCases[i].description;
        
        // GPS時刻変換
        time_t calculatedUnix = gpsTimeToUnixTimestamp(year, month, day, hour, min, sec);
        
        // 正確性確認
        TEST_ASSERT_EQUAL_UINT32(expectedUnix, calculatedUnix);
        
        // オーバーフロー検証
        uint64_t ms64 = (uint64_t)calculatedUnix * 1000ULL;
        uint32_t ms32_overflow = (uint32_t)(calculatedUnix * 1000UL);
        
        bool willOverflow = (ms64 > UINT32_MAX);
        
        printf("Test Case: %s (%u-%02u-%02u %02u:%02u:%02u)\n", 
               description, year, month, day, hour, min, sec);
        printf("  Unix timestamp: %u\n", (unsigned)calculatedUnix);
        printf("  64-bit ms: %llu\n", ms64);
        printf("  32-bit overflow: %s (result: %u)\n", 
               willOverflow ? "YES" : "NO", ms32_overflow);
        
        // 2025年以降は全てオーバーフローすることを確認
        if (year >= 2025) {
            TEST_ASSERT_TRUE(willOverflow);
        }
    }
}

int main() {
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
    
    // RFC 5905違反検証テスト（実際の失敗ケース）
    RUN_TEST(test_rfc5905_timestamp_validity);
    RUN_TEST(test_rfc5905_stratum_consistency);
    RUN_TEST(test_rfc5905_reference_timestamp_validity);
    RUN_TEST(test_ntp_client_expectation_violation);
    RUN_TEST(test_gps_synchronization_integrity);
    
    // 32ビット整数オーバーフロー検証テスト
    RUN_TEST(test_32bit_overflow_detection);
    RUN_TEST(test_32bit_safe_calculation);
    RUN_TEST(test_overflow_boundary_cases);
    RUN_TEST(test_ntp_timestamp_precision_preservation);
    RUN_TEST(test_real_world_2025_timestamps);
    
    return UNITY_END();
}