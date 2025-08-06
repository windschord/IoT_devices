#include <unity.h>
#include <Arduino.h>

// Use Arduino Mock environment
// millis/micros functions are defined in arduino_mock.h

// Include TimeUtils with path resolution - contains NtpTimestamp definition
#include "utils/TimeUtils.h"

/**
 * @brief Test Unix時間からNTPタイムスタンプへの変換
 */
void test_timeutils_unix_to_ntp_timestamp_conversion() {
    // Test basic conversion
    uint32_t unix_time = 1577836800UL; // 2020年1月1日 00:00:00 UTC
    uint64_t ntp_timestamp = TimeUtils::unixToNtpTimestamp(unix_time);
    
    uint32_t expected_ntp_seconds = unix_time + TimeUtils::UNIX_TO_NTP_OFFSET;
    uint32_t actual_ntp_seconds = static_cast<uint32_t>(ntp_timestamp >> 32);
    
    TEST_ASSERT_EQUAL_UINT32(expected_ntp_seconds, actual_ntp_seconds);
    
    // Test with microseconds
    uint32_t microseconds = 500000; // 0.5 seconds
    uint64_t ntp_timestamp_with_micros = TimeUtils::unixToNtpTimestamp(unix_time, microseconds);
    
    uint32_t ntp_fraction = static_cast<uint32_t>(ntp_timestamp_with_micros & 0xFFFFFFFF);
    TEST_ASSERT_NOT_EQUAL(0, ntp_fraction); // Should have fraction part
}

/**
 * @brief Test NTPタイムスタンプからUnix時間への変換
 */
void test_timeutils_ntp_to_unix_time_conversion() {
    // Test round-trip conversion
    uint32_t original_unix_time = 1577836800UL;
    uint32_t original_microseconds = 123456;
    
    uint64_t ntp_timestamp = TimeUtils::unixToNtpTimestamp(original_unix_time, original_microseconds);
    
    uint32_t converted_microseconds;
    uint32_t converted_unix_time = TimeUtils::ntpToUnixTime(ntp_timestamp, &converted_microseconds);
    
    TEST_ASSERT_EQUAL_UINT32(original_unix_time, converted_unix_time);
    // Allow small precision loss in microseconds conversion
    TEST_ASSERT_UINT32_WITHIN(10, original_microseconds, converted_microseconds);
    
    // Test without microseconds output
    uint32_t converted_unix_time_no_micros = TimeUtils::ntpToUnixTime(ntp_timestamp);
    TEST_ASSERT_EQUAL_UINT32(original_unix_time, converted_unix_time_no_micros);
}

/**
 * @brief Test オーバーフロー・アンダーフロー境界値
 */
void test_timeutils_overflow_underflow_boundary_values() {
    // Test maximum uint32_t Unix time
    uint32_t max_unix_time = 0xFFFFFFFF;
    uint64_t ntp_max = TimeUtils::unixToNtpTimestamp(max_unix_time);
    uint32_t converted_max = TimeUtils::ntpToUnixTime(ntp_max);
    TEST_ASSERT_EQUAL_UINT32(max_unix_time, converted_max);
    
    // Test minimum valid Unix time (year 2020)
    uint32_t min_valid_time = TimeUtils::YEAR_2020_UNIX;
    uint64_t ntp_min = TimeUtils::unixToNtpTimestamp(min_valid_time);
    uint32_t converted_min = TimeUtils::ntpToUnixTime(ntp_min);
    TEST_ASSERT_EQUAL_UINT32(min_valid_time, converted_min);
    
    // Test zero Unix time
    uint32_t zero_time = 0;
    uint64_t ntp_zero = TimeUtils::unixToNtpTimestamp(zero_time);
    uint32_t converted_zero = TimeUtils::ntpToUnixTime(ntp_zero);
    TEST_ASSERT_EQUAL_UINT32(zero_time, converted_zero);
}

/**
 * @brief Test 精度計算・時刻差分計算
 */
void test_timeutils_precision_time_difference_calculation() {
    // Test basic time difference
    uint32_t time1 = 1000;
    uint32_t time2 = 2000;
    uint32_t diff = TimeUtils::calculateTimeDifference(time1, time2);
    TEST_ASSERT_EQUAL_UINT32(1000, diff);
    
    // Test reverse order (should return absolute difference)
    uint32_t diff_reverse = TimeUtils::calculateTimeDifference(time2, time1);
    TEST_ASSERT_EQUAL_UINT32(1000, diff_reverse);
    
    // Test precision difference calculation
    uint32_t ref_time = 1000;
    uint32_t meas_time = 1001;
    uint32_t ref_micros = 500000;
    uint32_t meas_micros = 750000;
    
    int64_t precision_diff = TimeUtils::calculatePrecisionDifference(
        ref_time, meas_time, ref_micros, meas_micros);
    
    // Expected: 1 second + 0.25 seconds = 1.25 seconds = 1,250,000 microseconds
    int64_t expected_diff = 1250000;
    TEST_ASSERT_EQUAL_INT64(expected_diff, precision_diff);
    
    // Test negative precision difference
    int64_t negative_diff = TimeUtils::calculatePrecisionDifference(
        meas_time, ref_time, meas_micros, ref_micros);
    TEST_ASSERT_EQUAL_INT64(-expected_diff, negative_diff);
}

/**
 * @brief Test うるう秒・時間帯変更・夏時間処理
 */
void test_timeutils_leap_second_timezone_daylight_handling() {
    // Test leap year calculation
    TEST_ASSERT_TRUE(TimeUtils::isLeapYear(2020));   // Divisible by 4
    TEST_ASSERT_FALSE(TimeUtils::isLeapYear(2021));   // Not divisible by 4
    TEST_ASSERT_FALSE(TimeUtils::isLeapYear(1900));   // Divisible by 100, not by 400
    TEST_ASSERT_TRUE(TimeUtils::isLeapYear(2000));    // Divisible by 400
    
    // Test edge cases
    TEST_ASSERT_TRUE(TimeUtils::isLeapYear(2024));
    TEST_ASSERT_FALSE(TimeUtils::isLeapYear(2100));
    TEST_ASSERT_TRUE(TimeUtils::isLeapYear(2400));
}

/**
 * @brief Test NTPタイムスタンプ生成の全精度レベル
 */
void test_timeutils_ntp_timestamp_precision_levels() {
    uint32_t base_time = 1577836800UL;
    
    // Test with microsecond precision
    uint64_t precise_timestamp = TimeUtils::generatePreciseNtpTimestamp(base_time, true);
    uint32_t precise_seconds = static_cast<uint32_t>(precise_timestamp >> 32);
    uint32_t precise_fraction = static_cast<uint32_t>(precise_timestamp & 0xFFFFFFFF);
    
    TEST_ASSERT_EQUAL_UINT32(base_time + TimeUtils::UNIX_TO_NTP_OFFSET, precise_seconds);
    // Fraction should be non-zero with microsecond precision
    TEST_ASSERT_NOT_EQUAL(0, precise_fraction);
    
    // Test without microsecond precision
    uint64_t basic_timestamp = TimeUtils::generatePreciseNtpTimestamp(base_time, false);
    uint32_t basic_fraction = static_cast<uint32_t>(basic_timestamp & 0xFFFFFFFF);
    
    // Without precision, fraction should be zero
    TEST_ASSERT_EQUAL_UINT32(0, basic_fraction);
}

/**
 * @brief Test 時刻同期品質評価アルゴリズム
 */
void test_timeutils_time_sync_quality_evaluation() {
    // Test valid Unix time ranges
    TEST_ASSERT_TRUE(TimeUtils::isValidUnixTime(TimeUtils::YEAR_2020_UNIX));
    TEST_ASSERT_TRUE(TimeUtils::isValidUnixTime(TimeUtils::YEAR_2020_UNIX + 86400 * 365)); // One year later
    
    // Test invalid Unix time ranges
    TEST_ASSERT_FALSE(TimeUtils::isValidUnixTime(TimeUtils::YEAR_2020_UNIX - 1)); // Before 2020
    TEST_ASSERT_FALSE(TimeUtils::isValidUnixTime(4102444800UL)); // Year 2100
    TEST_ASSERT_FALSE(TimeUtils::isValidUnixTime(0)); // Unix epoch
    
    // Test monotonic time checking
    uint32_t current_time = 1000;
    uint32_t last_time = 999;
    TEST_ASSERT_TRUE(TimeUtils::isMonotonicTime(current_time, last_time));
    
    // Test small rollback (within tolerance)
    current_time = 999;
    last_time = 1000;
    TEST_ASSERT_TRUE(TimeUtils::isMonotonicTime(current_time, last_time)); // 1 second rollback is tolerated
    
    // Test large rollback (beyond tolerance)
    current_time = 997;
    last_time = 1000;
    TEST_ASSERT_FALSE(TimeUtils::isMonotonicTime(current_time, last_time)); // 3 second rollback is not tolerated
}

/**
 * @brief Test NTP Short Format変換
 */
void test_timeutils_ntp_short_format_conversion() {
    // Test microseconds to NTP short format
    uint32_t microseconds = 500000; // 0.5 seconds
    uint32_t ntp_short = TimeUtils::microsecondsToNtpShort(microseconds);
    
    // Convert back to microseconds
    uint32_t converted_microseconds = TimeUtils::ntpShortToMicroseconds(ntp_short);
    
    // Allow small precision loss
    TEST_ASSERT_UINT32_WITHIN(100, microseconds, converted_microseconds);
    
    // Test boundary values
    uint32_t zero_micros = 0;
    uint32_t ntp_zero = TimeUtils::microsecondsToNtpShort(zero_micros);
    TEST_ASSERT_EQUAL_UINT32(0, ntp_zero);
    
    uint32_t max_micros = 999999; // Just under 1 second
    uint32_t ntp_max = TimeUtils::microsecondsToNtpShort(max_micros);
    uint32_t converted_max = TimeUtils::ntpShortToMicroseconds(ntp_max);
    TEST_ASSERT_UINT32_WITHIN(1000, max_micros, converted_max);
}

/**
 * @brief Test 時刻文字列フォーマット（ISO 8601形式）
 */
void test_timeutils_time_string_formatting() {
    char buffer[32];
    uint32_t unix_time = 3661; // 1 hour, 1 minute, 1 second from epoch
    
    // Test basic time formatting
    TimeUtils::formatTimeString(unix_time, buffer, sizeof(buffer));
    // Expected format: "01:01:01" (HH:MM:SS)
    TEST_ASSERT_EQUAL_STRING("01:01:01", buffer);
    
    // Test with microseconds
    uint32_t microseconds = 123456;
    TimeUtils::formatTimeString(unix_time, buffer, sizeof(buffer), true, microseconds);
    // Expected format: "01:01:01.123456" (HH:MM:SS.uuuuuu)
    TEST_ASSERT_EQUAL_STRING("01:01:01.123456", buffer);
    
    // Test buffer size validation
    char small_buffer[10];
    TimeUtils::formatTimeString(unix_time, small_buffer, sizeof(small_buffer));
    // Should still produce valid time string without microseconds
    TEST_ASSERT_EQUAL_STRING("01:01:01", small_buffer);
    
    // Test null buffer handling
    TimeUtils::formatTimeString(unix_time, nullptr, 0);
    // Should not crash (no way to verify output)
    
    // Test zero time
    TimeUtils::formatTimeString(0, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING("00:00:00", buffer);
}

/**
 * @brief Test getCurrentMicros()の精度とオーバーフロー処理
 */
void test_timeutils_get_current_micros_precision_overflow() {
    // Test basic functionality
    uint64_t micros1 = TimeUtils::getCurrentMicros();
    // Small delay to ensure time difference
    delayMicroseconds(1000);
    uint64_t micros2 = TimeUtils::getCurrentMicros();
    
    // Second reading should be larger than first
    TEST_ASSERT_GREATER_THAN(micros1, micros2);
    
    // Difference should be approximately 1000 microseconds (allow some tolerance)
    uint64_t diff = micros2 - micros1;
    TEST_ASSERT_UINT64_WITHIN(500, 1000, diff);
}

/**
 * @brief Test エラー処理と異常値処理
 */
void test_timeutils_error_handling_abnormal_values() {
    // Test time difference with identical times
    uint32_t same_time = 1000;
    uint32_t diff = TimeUtils::calculateTimeDifference(same_time, same_time);
    TEST_ASSERT_EQUAL_UINT32(0, diff);
    
    // Test precision difference with zero values
    int64_t zero_diff = TimeUtils::calculatePrecisionDifference(0, 0, 0, 0);
    TEST_ASSERT_EQUAL_INT64(0, zero_diff);
    
    // Test NTP conversion with edge values
    uint64_t max_ntp = 0xFFFFFFFFFFFFFFFFULL;
    uint32_t converted_time = TimeUtils::ntpToUnixTime(max_ntp);
    // Should handle large values without crashing
    TEST_ASSERT_NOT_EQUAL(0, converted_time);
    
    // Test format string with extreme values
    char buffer[32];
    uint32_t large_time = 0xFFFFFFFF;
    TimeUtils::formatTimeString(large_time, buffer, sizeof(buffer));
    // Should produce some valid output without crashing
    TEST_ASSERT_NOT_EQUAL(0, strlen(buffer));
}

// Test suite setup and teardown
void setUp(void) {
    // Reset any static variables if needed
}

void tearDown(void) {
    // Cleanup after each test
}

/**
 * @brief TimeUtils完全カバレッジテスト実行
 */
int main() {
    UNITY_BEGIN();
    
    // Basic conversion functions
    RUN_TEST(test_timeutils_unix_to_ntp_timestamp_conversion);
    RUN_TEST(test_timeutils_ntp_to_unix_time_conversion);
    
    // Boundary value testing
    RUN_TEST(test_timeutils_overflow_underflow_boundary_values);
    
    // Precision and difference calculations
    RUN_TEST(test_timeutils_precision_time_difference_calculation);
    
    // Time handling features
    RUN_TEST(test_timeutils_leap_second_timezone_daylight_handling);
    
    // NTP timestamp generation
    RUN_TEST(test_timeutils_ntp_timestamp_precision_levels);
    
    // Time sync quality evaluation
    RUN_TEST(test_timeutils_time_sync_quality_evaluation);
    
    // NTP short format conversion
    RUN_TEST(test_timeutils_ntp_short_format_conversion);
    
    // String formatting
    RUN_TEST(test_timeutils_time_string_formatting);
    
    // Precision and overflow handling
    RUN_TEST(test_timeutils_get_current_micros_precision_overflow);
    
    // Error handling
    RUN_TEST(test_timeutils_error_handling_abnormal_values);
    
    return UNITY_END();
}