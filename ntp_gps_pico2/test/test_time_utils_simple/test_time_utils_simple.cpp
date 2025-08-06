#include <unity.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

// Simple TimeUtils implementation for testing (Simple Test Design Pattern)
class TimeUtils {
public:
    static const uint32_t UNIX_TO_NTP_OFFSET = 2208988800UL;
    static const uint64_t NTP_FRACTION_PER_MICROSECOND = 4294967296ULL / 1000000ULL;
    
    static uint64_t unixToNtpTimestamp(uint32_t unix_time, uint32_t microseconds = 0) {
        uint64_t ntp_seconds = (uint64_t)(unix_time + UNIX_TO_NTP_OFFSET);
        uint64_t ntp_fraction = (uint64_t)microseconds * NTP_FRACTION_PER_MICROSECOND;
        
        return (ntp_seconds << 32) | (ntp_fraction & 0xFFFFFFFFULL);
    }
    
    static uint32_t ntpToUnixTime(uint64_t ntp_timestamp, uint32_t* microseconds = nullptr) {
        uint32_t ntp_seconds = static_cast<uint32_t>(ntp_timestamp >> 32);
        uint32_t unix_time = ntp_seconds - UNIX_TO_NTP_OFFSET;
        
        if (microseconds) {
            uint32_t ntp_fraction = static_cast<uint32_t>(ntp_timestamp & 0xFFFFFFFF);
            *microseconds = static_cast<uint32_t>(ntp_fraction / NTP_FRACTION_PER_MICROSECOND);
        }
        
        return unix_time;
    }
    
    static uint32_t calculateTimeDifference(uint32_t time1, uint32_t time2) {
        if (time2 >= time1) {
            return time2 - time1;
        } else {
            // Handle overflow
            return (0xFFFFFFFFUL - time1) + time2 + 1;
        }
    }
    
    static void formatTimeString(uint32_t unix_time, char* buffer, size_t buffer_size) {
        if (!buffer || buffer_size == 0) return;
        
        uint32_t seconds = unix_time % 86400; // Seconds in a day
        uint32_t hours = seconds / 3600;
        uint32_t minutes = (seconds % 3600) / 60;
        uint32_t secs = seconds % 60;
        
        if (buffer_size >= 12) { // Full format: "HH:MM:SS.uuuuuu"
            snprintf(buffer, buffer_size, "%02u:%02u:%02u", hours, minutes, secs);
        } else if (buffer_size >= 9) { // Basic format: "HH:MM:SS"
            snprintf(buffer, buffer_size, "%02u:%02u:%02u", hours, minutes, secs);
        } else {
            buffer[0] = '\0'; // Buffer too small
        }
    }
    
    static bool isLeapYear(uint32_t year) {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }
    
    static float calculatePrecision(uint32_t time_diff_microseconds) {
        if (time_diff_microseconds == 0) return 0.0f;
        return time_diff_microseconds / 1000000.0f; // Convert to seconds
    }
    
    static uint64_t generatePreciseNtpTimestamp(uint32_t base_time, bool use_microseconds = true) {
        uint32_t microseconds = use_microseconds ? 123456 : 0; // Mock microseconds
        return unixToNtpTimestamp(base_time, microseconds);
    }
    
    static uint64_t getCurrentMicros() {
        static uint64_t mock_counter = 1000000ULL;
        return mock_counter += 1000; // Increment by 1000μs each call
    }
    
    static uint8_t evaluateTimeSyncQuality(float precision_seconds) {
        if (precision_seconds <= 0.001f) return 100; // Excellent
        if (precision_seconds <= 0.01f) return 80;   // Good
        if (precision_seconds <= 0.1f) return 60;    // Fair
        if (precision_seconds <= 1.0f) return 40;    // Poor
        return 20; // Very poor
    }
    
    static uint32_t convertToNtpShortFormat(float seconds) {
        // Convert seconds to NTP short format (16-bit seconds, 16-bit fraction)
        uint16_t int_part = static_cast<uint16_t>(seconds);
        uint16_t frac_part = static_cast<uint16_t>((seconds - int_part) * 65536.0f);
        return (static_cast<uint32_t>(int_part) << 16) | frac_part;
    }
};

/**
 * @brief Test Unix時間からNTPタイムスタンプへの変換
 */
void test_timeutils_unix_to_ntp_conversion() {
    // Test basic conversion
    uint32_t unix_time = 1577836800UL; // 2020年1月1日 00:00:00 UTC
    uint64_t ntp_timestamp = TimeUtils::unixToNtpTimestamp(unix_time);
    
    uint32_t expected_ntp_seconds = unix_time + TimeUtils::UNIX_TO_NTP_OFFSET;
    uint32_t actual_ntp_seconds = static_cast<uint32_t>(ntp_timestamp >> 32);
    
    TEST_ASSERT_EQUAL_UINT32(expected_ntp_seconds, actual_ntp_seconds);
}

/**
 * @brief Test NTPタイムスタンプからUnix時間への変換
 */
void test_timeutils_ntp_to_unix_conversion() {
    // Test round-trip conversion
    uint32_t original_unix_time = 1577836800UL;
    uint32_t original_microseconds = 123456;
    
    uint64_t ntp_timestamp = TimeUtils::unixToNtpTimestamp(original_unix_time, original_microseconds);
    
    uint32_t converted_microseconds;
    uint32_t converted_unix_time = TimeUtils::ntpToUnixTime(ntp_timestamp, &converted_microseconds);
    
    TEST_ASSERT_EQUAL_UINT32(original_unix_time, converted_unix_time);
    // Allow small precision loss in microseconds conversion
    TEST_ASSERT_UINT32_WITHIN(10, original_microseconds, converted_microseconds);
}

/**
 * @brief Test 時刻フォーマット機能
 */
void test_timeutils_time_formatting() {
    uint32_t unix_time = 3661; // 01:01:01
    char buffer[32];
    
    TimeUtils::formatTimeString(unix_time, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("01:01:01", buffer);
    
    // Test with zero time
    TimeUtils::formatTimeString(0, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("00:00:00", buffer);
}

/**
 * @brief Test 時刻差分計算
 */
void test_timeutils_time_difference_calculation() {
    // Test normal case
    uint32_t time1 = 1000;
    uint32_t time2 = 2000;
    uint32_t diff = TimeUtils::calculateTimeDifference(time1, time2);
    TEST_ASSERT_EQUAL_UINT32(1000, diff);
    
    // Test overflow case
    uint32_t time_before_overflow = 0xFFFFFFFEUL;
    uint32_t time_after_overflow = 5;
    diff = TimeUtils::calculateTimeDifference(time_before_overflow, time_after_overflow);
    TEST_ASSERT_EQUAL_UINT32(7, diff); // (0xFFFFFFFF - 0xFFFFFFFE) + 5 + 1 = 7
}

/**
 * @brief Test うるう年計算
 */
void test_timeutils_leap_year_calculation() {
    TEST_ASSERT_TRUE(TimeUtils::isLeapYear(2020));  // Divisible by 4
    TEST_ASSERT_FALSE(TimeUtils::isLeapYear(2021)); // Not divisible by 4
    TEST_ASSERT_FALSE(TimeUtils::isLeapYear(1900)); // Divisible by 100, not by 400
    TEST_ASSERT_TRUE(TimeUtils::isLeapYear(2000));  // Divisible by 400
}

/**
 * @brief Test 精度計算
 */
void test_timeutils_precision_calculation() {
    float precision = TimeUtils::calculatePrecision(1000000); // 1 second in microseconds
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, precision);
    
    precision = TimeUtils::calculatePrecision(500000); // 0.5 seconds
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, precision);
    
    precision = TimeUtils::calculatePrecision(0); // Zero time diff
    TEST_ASSERT_EQUAL_FLOAT(0.0f, precision);
}

/**
 * @brief Test NTP高精度タイムスタンプ生成
 */
void test_timeutils_precise_ntp_timestamp() {
    uint32_t base_time = 1577836800UL;
    
    // Test with microsecond precision
    uint64_t precise_timestamp = TimeUtils::generatePreciseNtpTimestamp(base_time, true);
    uint32_t precise_fraction = static_cast<uint32_t>(precise_timestamp & 0xFFFFFFFF);
    TEST_ASSERT_NOT_EQUAL(0, precise_fraction); // Should have fraction part
    
    // Test without microsecond precision
    uint64_t basic_timestamp = TimeUtils::generatePreciseNtpTimestamp(base_time, false);
    uint32_t basic_fraction = static_cast<uint32_t>(basic_timestamp & 0xFFFFFFFF);
    TEST_ASSERT_EQUAL_UINT32(0, basic_fraction); // Should have no fraction part
}

/**
 * @brief Test 時刻同期品質評価
 */
void test_timeutils_time_sync_quality() {
    TEST_ASSERT_EQUAL_UINT8(100, TimeUtils::evaluateTimeSyncQuality(0.0005f)); // Excellent
    TEST_ASSERT_EQUAL_UINT8(80, TimeUtils::evaluateTimeSyncQuality(0.005f));   // Good
    TEST_ASSERT_EQUAL_UINT8(60, TimeUtils::evaluateTimeSyncQuality(0.05f));    // Fair
    TEST_ASSERT_EQUAL_UINT8(40, TimeUtils::evaluateTimeSyncQuality(0.5f));     // Poor
    TEST_ASSERT_EQUAL_UINT8(20, TimeUtils::evaluateTimeSyncQuality(2.0f));     // Very poor
}

/**
 * @brief Test NTPショートフォーマット変換
 */
void test_timeutils_ntp_short_format() {
    uint32_t short_format = TimeUtils::convertToNtpShortFormat(1.5f);
    
    uint16_t int_part = static_cast<uint16_t>(short_format >> 16);
    uint16_t frac_part = static_cast<uint16_t>(short_format & 0xFFFF);
    
    TEST_ASSERT_EQUAL_UINT16(1, int_part);
    TEST_ASSERT_GREATER_THAN(0, frac_part); // Should have fractional part
}

/**
 * @brief Test getCurrentMicros関数
 */
void test_timeutils_get_current_micros() {
    uint64_t micros1 = TimeUtils::getCurrentMicros();
    uint64_t micros2 = TimeUtils::getCurrentMicros();
    
    // Second reading should be larger than first
    TEST_ASSERT_GREATER_THAN(micros1, micros2);
    
    // Difference should be exactly 1000 microseconds in our mock
    uint64_t diff = micros2 - micros1;
    TEST_ASSERT_EQUAL_UINT64(1000, diff);
}

/**
 * @brief Test エラーハンドリングと境界値
 */
void test_timeutils_error_handling() {
    // Test null buffer handling
    TimeUtils::formatTimeString(3661, nullptr, 10);
    // Should not crash
    
    // Test zero buffer size
    char buffer[32];
    TimeUtils::formatTimeString(3661, buffer, 0);
    // Should not crash
    
    // Test same time difference
    uint32_t diff = TimeUtils::calculateTimeDifference(1000, 1000);
    TEST_ASSERT_EQUAL_UINT32(0, diff);
}

void setUp(void) {
    // Reset any static state if needed
}

void tearDown(void) {
    // Cleanup
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_timeutils_unix_to_ntp_conversion);
    RUN_TEST(test_timeutils_ntp_to_unix_conversion);
    RUN_TEST(test_timeutils_time_formatting);
    RUN_TEST(test_timeutils_time_difference_calculation);
    RUN_TEST(test_timeutils_leap_year_calculation);
    RUN_TEST(test_timeutils_precision_calculation);
    RUN_TEST(test_timeutils_precise_ntp_timestamp);
    RUN_TEST(test_timeutils_time_sync_quality);
    RUN_TEST(test_timeutils_ntp_short_format);
    RUN_TEST(test_timeutils_get_current_micros);
    RUN_TEST(test_timeutils_error_handling);
    
    return UNITY_END();
}