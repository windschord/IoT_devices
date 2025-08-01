#include <unity.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Mock Arduino functions
extern "C" {
    uint32_t millis() { return 1000; }
    uint32_t micros() { return 1000000; }
    void delayMicroseconds(unsigned int us) { (void)us; }
    int snprintf(char *str, size_t size, const char *format, ...) {
        // Simple mock implementation
        if (str && size > 0) {
            strcpy(str, "01:01:01");
        }
        return 8;
    }
}

// TimeUtils implementation (simplified for testing)
class TimeUtils {
public:
    static const uint32_t UNIX_TO_NTP_OFFSET = 2208988800UL;
    static const uint64_t MICROS_PER_SECOND = 1000000UL;
    static const uint32_t YEAR_2020_UNIX = 1577836800UL;

    static uint64_t unixToNtpTimestamp(uint32_t unix_time, uint32_t microseconds = 0) {
        uint32_t ntp_seconds = unix_time + UNIX_TO_NTP_OFFSET;
        uint32_t ntp_fraction = static_cast<uint32_t>((static_cast<uint64_t>(microseconds) * 0x100000000ULL) / MICROS_PER_SECOND);
        return (static_cast<uint64_t>(ntp_seconds) << 32) | ntp_fraction;
    }

    static uint32_t ntpToUnixTime(uint64_t ntp_timestamp, uint32_t* microseconds_out = nullptr) {
        uint32_t ntp_seconds = static_cast<uint32_t>(ntp_timestamp >> 32);
        uint32_t ntp_fraction = static_cast<uint32_t>(ntp_timestamp & 0xFFFFFFFF);
        
        if (microseconds_out) {
            *microseconds_out = static_cast<uint32_t>((static_cast<uint64_t>(ntp_fraction) * MICROS_PER_SECOND) >> 32);
        }
        
        return ntp_seconds - UNIX_TO_NTP_OFFSET;
    }

    static bool isValidUnixTime(uint32_t unix_time) {
        const uint32_t YEAR_2100_UNIX = 4102444800UL;
        return (unix_time >= YEAR_2020_UNIX && unix_time < YEAR_2100_UNIX);
    }

    static uint32_t calculateTimeDifference(uint32_t time1, uint32_t time2) {
        if (time1 > time2) {
            return time1 - time2;
        } else {
            return time2 - time1;
        }
    }

    static bool isLeapYear(uint16_t year) {
        return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
    }

    static bool isMonotonicTime(uint32_t current_time, uint32_t last_time) {
        const uint32_t TOLERANCE_SECONDS = 1;
        
        if (current_time >= last_time) {
            return true;
        }
        
        uint32_t rollback = last_time - current_time;
        return (rollback <= TOLERANCE_SECONDS);
    }
};

/**
 * @brief Test Unix時間からNTPタイムスタンプへの変換
 */
void test_timeutils_unix_to_ntp_conversion() {
    uint32_t unix_time = 1577836800UL; // 2020年1月1日
    uint64_t ntp_timestamp = TimeUtils::unixToNtpTimestamp(unix_time);
    
    uint32_t expected_ntp_seconds = unix_time + TimeUtils::UNIX_TO_NTP_OFFSET;
    uint32_t actual_ntp_seconds = static_cast<uint32_t>(ntp_timestamp >> 32);
    
    TEST_ASSERT_EQUAL_UINT32(expected_ntp_seconds, actual_ntp_seconds);
}

/**
 * @brief Test NTPタイムスタンプからUnix時間への変換
 */
void test_timeutils_ntp_to_unix_conversion() {
    uint32_t original_unix_time = 1577836800UL;
    uint64_t ntp_timestamp = TimeUtils::unixToNtpTimestamp(original_unix_time);
    uint32_t converted_unix_time = TimeUtils::ntpToUnixTime(ntp_timestamp);
    
    TEST_ASSERT_EQUAL_UINT32(original_unix_time, converted_unix_time);
}

/**
 * @brief Test 境界値処理
 */
void test_timeutils_boundary_values() {
    // Test maximum uint32_t Unix time
    uint32_t max_unix_time = 0xFFFFFFFF;
    uint64_t ntp_max = TimeUtils::unixToNtpTimestamp(max_unix_time);
    uint32_t converted_max = TimeUtils::ntpToUnixTime(ntp_max);
    TEST_ASSERT_EQUAL_UINT32(max_unix_time, converted_max);
    
    // Test minimum valid Unix time
    uint32_t min_valid_time = TimeUtils::YEAR_2020_UNIX;
    uint64_t ntp_min = TimeUtils::unixToNtpTimestamp(min_valid_time);
    uint32_t converted_min = TimeUtils::ntpToUnixTime(ntp_min);
    TEST_ASSERT_EQUAL_UINT32(min_valid_time, converted_min);
}

/**
 * @brief Test 時刻差分計算
 */
void test_timeutils_time_difference() {
    uint32_t time1 = 1000;
    uint32_t time2 = 2000;
    uint32_t diff = TimeUtils::calculateTimeDifference(time1, time2);
    TEST_ASSERT_EQUAL_UINT32(1000, diff);
    
    // Test reverse order
    uint32_t diff_reverse = TimeUtils::calculateTimeDifference(time2, time1);
    TEST_ASSERT_EQUAL_UINT32(1000, diff_reverse);
}

/**
 * @brief Test うるう年判定
 */
void test_timeutils_leap_year() {
    TEST_ASSERT_TRUE(TimeUtils::isLeapYear(2020));   // Divisible by 4
    TEST_ASSERT_FALSE(TimeUtils::isLeapYear(2021));  // Not divisible by 4
    TEST_ASSERT_FALSE(TimeUtils::isLeapYear(1900));  // Divisible by 100, not by 400
    TEST_ASSERT_TRUE(TimeUtils::isLeapYear(2000));   // Divisible by 400
}

/**
 * @brief Test 時刻の妥当性チェック
 */
void test_timeutils_time_validation() {
    TEST_ASSERT_TRUE(TimeUtils::isValidUnixTime(TimeUtils::YEAR_2020_UNIX));
    TEST_ASSERT_TRUE(TimeUtils::isValidUnixTime(TimeUtils::YEAR_2020_UNIX + 86400 * 365));
    TEST_ASSERT_FALSE(TimeUtils::isValidUnixTime(TimeUtils::YEAR_2020_UNIX - 1));
    TEST_ASSERT_FALSE(TimeUtils::isValidUnixTime(4102444800UL)); // Year 2100
}

/**
 * @brief Test 単調性チェック
 */
void test_timeutils_monotonic_time() {
    uint32_t current_time = 1000;
    uint32_t last_time = 999;
    TEST_ASSERT_TRUE(TimeUtils::isMonotonicTime(current_time, last_time));
    
    // Test small rollback (within tolerance)
    current_time = 999;
    last_time = 1000;
    TEST_ASSERT_TRUE(TimeUtils::isMonotonicTime(current_time, last_time));
    
    // Test large rollback (beyond tolerance)
    current_time = 997;
    last_time = 1000;
    TEST_ASSERT_FALSE(TimeUtils::isMonotonicTime(current_time, last_time));
}

void setUp(void) {
    // Reset any state before each test
}

void tearDown(void) {
    // Cleanup after each test
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_timeutils_unix_to_ntp_conversion);
    RUN_TEST(test_timeutils_ntp_to_unix_conversion);
    RUN_TEST(test_timeutils_boundary_values);
    RUN_TEST(test_timeutils_time_difference);
    RUN_TEST(test_timeutils_leap_year);
    RUN_TEST(test_timeutils_time_validation);
    RUN_TEST(test_timeutils_monotonic_time);
    
    return UNITY_END();
}