// time.h mock for native testing
#ifndef TIME_H_MOCK
#define TIME_H_MOCK

// Simple time types for Arduino compatibility
typedef long arduino_time_t;

// Mock time functions
inline arduino_time_t time(arduino_time_t* t) { 
    arduino_time_t now = 1704067200; // 2024-01-01 00:00:00 UTC
    if (t) *t = now;
    return now;
}

// Simple mock functions without complex type definitions
inline arduino_time_t mktime(void* timeptr) {
    return 1704067200; // Mock timestamp
}

#endif // TIME_H_MOCK