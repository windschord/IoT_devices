// time.h mock for native testing
#ifndef TIME_H_MOCK
#define TIME_H_MOCK

// Simple time types for Arduino compatibility (avoid ctime conflicts)
typedef long arduino_time_t;

struct arduino_tm {
    int tm_sec;   // seconds after the minute [0-60]
    int tm_min;   // minutes after the hour [0-59]
    int tm_hour;  // hours since midnight [0-23]
    int tm_mday;  // day of the month [1-31]
    int tm_mon;   // months since January [0-11]
    int tm_year;  // years since 1900
    int tm_wday;  // days since Sunday [0-6]
    int tm_yday;  // days since January 1 [0-365]
    int tm_isdst; // Daylight Saving Time flag
};

// Alias for Arduino compatibility
typedef arduino_time_t time_t;
typedef arduino_tm tm;

// Mock time functions
inline time_t time(time_t* t) { 
    time_t now = 1704067200; // 2024-01-01 00:00:00 UTC
    if (t) *t = now;
    return now;
}

inline tm* gmtime(const time_t* timer) {
    static tm result = {0, 0, 12, 1, 0, 124, 1, 0, 0}; // 2024-01-01 12:00:00
    return &result;
}

inline tm* localtime(const time_t* timer) {
    return gmtime(timer);
}

inline time_t mktime(tm* timeptr) {
    return 1704067200; // Mock timestamp
}

#endif // TIME_H_MOCK