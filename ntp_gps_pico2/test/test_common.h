#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <Arduino.h>

// テスト用の基本構造体
struct NtpTimestamp {
    uint32_t seconds;
    uint32_t fraction;
};

struct GpsSummaryData {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    bool timeValid;
    bool dateValid;
    int32_t latitude;
    int32_t longitude;
    int32_t altitude;
    uint16_t msec;
};

// テスト用定数
#define NTP_TIMESTAMP_DELTA 2208988800UL
#define NTP_LI_NO_WARNING 0x00
#define NTP_LI_61_SECONDS 0x01
#define NTP_MODE_SERVER 4
#define NTP_PACKET_SIZE 48

// テスト用のヘルパー関数（inline で重複定義を回避）
inline NtpTimestamp unixToNtpTimestamp(uint32_t unixSeconds, uint32_t microseconds = 0) {
    NtpTimestamp ntp;
    ntp.seconds = unixSeconds + NTP_TIMESTAMP_DELTA;
    ntp.fraction = (uint32_t)((uint64_t)microseconds * 4294967296ULL / 1000000ULL);
    return ntp;
}

inline uint32_t ntpToUnixTimestamp(const NtpTimestamp& ntp) {
    return ntp.seconds - NTP_TIMESTAMP_DELTA;
}

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

// GPS時刻からUnixタイムスタンプを計算する関数
inline time_t gpsTimeToUnixTimestamp(uint16_t year, uint8_t month, uint8_t day, 
                                     uint8_t hour, uint8_t min, uint8_t sec) {
    int years_since_epoch = year - 1970;
    
    int leap_years = 0;
    for (int y = 1970; y < year; y++) {
        if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) {
            leap_years++;
        }
    }
    
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    bool is_leap_year = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
    if (is_leap_year) {
        days_in_month[1] = 29;
    }
    
    int total_days = years_since_epoch * 365 + leap_years;
    
    for (int m = 1; m < month; m++) {
        total_days += days_in_month[m - 1];
    }
    
    total_days += day - 1;
    
    time_t timestamp = (time_t)total_days * 24 * 60 * 60;
    timestamp += hour * 60 * 60;
    timestamp += min * 60;
    timestamp += sec;
    
    return timestamp;
}

// テスト定数
static const time_t TEST_GPS_TIME = 1753179057; // 2025-07-22 10:10:57 UTC (ログから)
static const uint32_t EXPECTED_NTP_TIME = TEST_GPS_TIME + NTP_TIMESTAMP_DELTA;

#endif // TEST_COMMON_H