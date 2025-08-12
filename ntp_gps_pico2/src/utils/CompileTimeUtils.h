#ifndef COMPILE_TIME_UTILS_H
#define COMPILE_TIME_UTILS_H

#include <Arduino.h>

/**
 * @brief Compile-time optimization utilities for embedded systems
 * 
 * Provides constexpr functions, template metaprogramming utilities,
 * and compile-time calculations to reduce runtime overhead.
 */
namespace CompileTime {

    /**
     * @brief Compile-time mathematical constants
     */
    struct Constants {
        static constexpr double PI = 3.14159265358979323846;
        static constexpr double E = 2.71828182845904523536;
        static constexpr uint32_t SECONDS_PER_DAY = 86400;
        static constexpr uint32_t SECONDS_PER_HOUR = 3600;
        static constexpr uint32_t SECONDS_PER_MINUTE = 60;
        static constexpr uint32_t MILLISECONDS_PER_SECOND = 1000;
        static constexpr uint32_t MICROSECONDS_PER_SECOND = 1000000;
        static constexpr uint32_t NANOSECONDS_PER_MICROSECOND = 1000;
    };

    /**
     * @brief Compile-time mathematical operations
     */
    struct Math {
        /**
         * @brief Compile-time power calculation
         */
        static constexpr uint64_t pow(uint64_t base, uint32_t exp) {
            return (exp == 0) ? 1 : base * pow(base, exp - 1);
        }
        
        /**
         * @brief Compile-time factorial calculation
         */
        static constexpr uint64_t factorial(uint32_t n) {
            return (n <= 1) ? 1 : n * factorial(n - 1);
        }
        
        /**
         * @brief Compile-time minimum
         */
        template<typename T>
        static constexpr T min(T a, T b) {
            return (a < b) ? a : b;
        }
        
        /**
         * @brief Compile-time maximum
         */
        template<typename T>
        static constexpr T max(T a, T b) {
            return (a > b) ? a : b;
        }
        
        /**
         * @brief Compile-time absolute value
         */
        template<typename T>
        static constexpr T abs(T value) {
            return (value < 0) ? -value : value;
        }
        
        /**
         * @brief Compile-time integer square root (Newton's method)
         */
        static constexpr uint32_t sqrt(uint32_t n) {
            return sqrt_helper(n, n);
        }
        
    private:
        static constexpr uint32_t sqrt_helper(uint32_t n, uint32_t x) {
            return (x == 0) ? 0 : 
                   ((x * x <= n && (x + 1) * (x + 1) > n) ? x : 
                    sqrt_helper(n, (x + n / x) / 2));
        }
    };

    /**
     * @brief Compile-time bit manipulation utilities
     */
    struct Bits {
        /**
         * @brief Count trailing zeros
         */
        static constexpr uint32_t countTrailingZeros(uint32_t value) {
            return (value == 0) ? 32 : countTrailingZeros_helper(value, 0);
        }
        
        /**
         * @brief Count leading zeros
         */
        static constexpr uint32_t countLeadingZeros(uint32_t value) {
            return (value == 0) ? 32 : countLeadingZeros_helper(value, 0);
        }
        
        /**
         * @brief Check if value is power of 2
         */
        static constexpr bool isPowerOf2(uint32_t value) {
            return value != 0 && (value & (value - 1)) == 0;
        }
        
        /**
         * @brief Next power of 2 greater than or equal to value
         */
        static constexpr uint32_t nextPowerOf2(uint32_t value) {
            return (value <= 1) ? 1 : 
                   (isPowerOf2(value) ? value : 
                    (1U << (32 - countLeadingZeros(value - 1))));
        }
        
        /**
         * @brief Bit reverse (32-bit)
         */
        static constexpr uint32_t reverse(uint32_t value) {
            return reverseHelper(value, 0, 32);
        }
        
        /**
         * @brief Population count (count set bits)
         */
        static constexpr uint32_t popCount(uint32_t value) {
            return (value == 0) ? 0 : (value & 1) + popCount(value >> 1);
        }
        
    private:
        static constexpr uint32_t countTrailingZeros_helper(uint32_t value, uint32_t count) {
            return ((value & 1) != 0) ? count : countTrailingZeros_helper(value >> 1, count + 1);
        }
        
        static constexpr uint32_t countLeadingZeros_helper(uint32_t value, uint32_t count) {
            return (value >= (1U << 31)) ? count : countLeadingZeros_helper(value << 1, count + 1);
        }
        
        static constexpr uint32_t reverseHelper(uint32_t value, uint32_t result, uint32_t bits) {
            return (bits == 0) ? result : 
                   reverseHelper(value >> 1, (result << 1) | (value & 1), bits - 1);
        }
    };

    /**
     * @brief Compile-time string utilities
     */
    struct String {
        /**
         * @brief Compile-time string length
         */
        static constexpr size_t length(const char* str) {
            return (*str == '\0') ? 0 : 1 + length(str + 1);
        }
        
        /**
         * @brief Compile-time string comparison
         */
        static constexpr bool equal(const char* str1, const char* str2) {
            return (*str1 == '\0' && *str2 == '\0') ? true :
                   (*str1 != *str2) ? false :
                   equal(str1 + 1, str2 + 1);
        }
        
        /**
         * @brief Compile-time character search
         */
        static constexpr const char* find(const char* str, char ch) {
            return (*str == '\0') ? nullptr :
                   (*str == ch) ? str :
                   find(str + 1, ch);
        }
        
        /**
         * @brief Compile-time hash calculation (FNV-1a)
         */
        static constexpr uint32_t hash(const char* str) {
            return hash_helper(str, 2166136261U);
        }
        
    private:
        static constexpr uint32_t hash_helper(const char* str, uint32_t hash) {
            return (*str == '\0') ? hash : 
                   hash_helper(str + 1, (hash ^ static_cast<uint32_t>(*str)) * 16777619U);
        }
    };

    /**
     * @brief Compile-time array utilities
     */
    template<typename T, size_t N>
    struct Array {
        /**
         * @brief Get array size
         */
        static constexpr size_t size() { return N; }
        
        /**
         * @brief Check if array is empty
         */
        static constexpr bool empty() { return N == 0; }
        
        /**
         * @brief Find minimum element index
         */
        static constexpr size_t minElementIndex(const T (&arr)[N]) {
            return minElementIndex_helper(arr, 0, 0, 1);
        }
        
        /**
         * @brief Find maximum element index
         */
        static constexpr size_t maxElementIndex(const T (&arr)[N]) {
            return maxElementIndex_helper(arr, 0, 0, 1);
        }
        
        /**
         * @brief Calculate sum of array elements
         */
        static constexpr T sum(const T (&arr)[N]) {
            return sum_helper(arr, 0, 0);
        }
        
    private:
        static constexpr size_t minElementIndex_helper(const T (&arr)[N], size_t minIdx, size_t currentIdx, size_t remaining) {
            return (remaining == 0) ? minIdx :
                   (arr[currentIdx] < arr[minIdx]) ? 
                   minElementIndex_helper(arr, currentIdx, currentIdx + 1, remaining - 1) :
                   minElementIndex_helper(arr, minIdx, currentIdx + 1, remaining - 1);
        }
        
        static constexpr size_t maxElementIndex_helper(const T (&arr)[N], size_t maxIdx, size_t currentIdx, size_t remaining) {
            return (remaining == 0) ? maxIdx :
                   (arr[currentIdx] > arr[maxIdx]) ? 
                   maxElementIndex_helper(arr, currentIdx, currentIdx + 1, remaining - 1) :
                   maxElementIndex_helper(arr, maxIdx, currentIdx + 1, remaining - 1);
        }
        
        static constexpr T sum_helper(const T (&arr)[N], T accumulator, size_t index) {
            return (index >= N) ? accumulator : sum_helper(arr, accumulator + arr[index], index + 1);
        }
    };

    /**
     * @brief Type traits for compile-time type checking
     */
    template<typename T>
    struct TypeTraits {
        static constexpr bool is_integral = false;
        static constexpr bool is_signed = false;
        static constexpr bool is_unsigned = false;
        static constexpr size_t size = sizeof(T);
    };

    // Specializations for common types
    template<> struct TypeTraits<uint8_t> { 
        static constexpr bool is_integral = true; 
        static constexpr bool is_signed = false; 
        static constexpr bool is_unsigned = true; 
        static constexpr size_t size = 1; 
    };
    
    template<> struct TypeTraits<int8_t> { 
        static constexpr bool is_integral = true; 
        static constexpr bool is_signed = true; 
        static constexpr bool is_unsigned = false; 
        static constexpr size_t size = 1; 
    };
    
    template<> struct TypeTraits<uint16_t> { 
        static constexpr bool is_integral = true; 
        static constexpr bool is_signed = false; 
        static constexpr bool is_unsigned = true; 
        static constexpr size_t size = 2; 
    };
    
    template<> struct TypeTraits<int16_t> { 
        static constexpr bool is_integral = true; 
        static constexpr bool is_signed = true; 
        static constexpr bool is_unsigned = false; 
        static constexpr size_t size = 2; 
    };
    
    template<> struct TypeTraits<uint32_t> { 
        static constexpr bool is_integral = true; 
        static constexpr bool is_signed = false; 
        static constexpr bool is_unsigned = true; 
        static constexpr size_t size = 4; 
    };
    
    template<> struct TypeTraits<int32_t> { 
        static constexpr bool is_integral = true; 
        static constexpr bool is_signed = true; 
        static constexpr bool is_unsigned = false; 
        static constexpr size_t size = 4; 
    };

    /**
     * @brief Compile-time buffer size calculations for common protocols
     */
    struct BufferSizes {
        // Network protocol buffer sizes
        static constexpr size_t NTP_PACKET = 48;
        static constexpr size_t ETHERNET_FRAME_MAX = 1518;
        static constexpr size_t IP_HEADER_MIN = 20;
        static constexpr size_t TCP_HEADER_MIN = 20;
        static constexpr size_t UDP_HEADER = 8;
        
        // GPS/GNSS buffer sizes
        static constexpr size_t UBX_FRAME_MAX = 65535;
        static constexpr size_t NMEA_SENTENCE_MAX = 256;
        
        // Display buffer sizes
        static constexpr size_t OLED_WIDTH = 128;
        static constexpr size_t OLED_HEIGHT = 64;
        static constexpr size_t DISPLAY_BUFFER = (OLED_WIDTH * OLED_HEIGHT) / 8;
        
        // Configuration and logging
        static constexpr size_t CONFIG_KEY_MAX = 32;
        static constexpr size_t CONFIG_VALUE_MAX = 128;
        static constexpr size_t LOG_MESSAGE_MAX = 256;
        static constexpr size_t JSON_BUFFER_MAX = 1024;
        
        // File system
        static constexpr size_t FILENAME_MAX = 64;
        static constexpr size_t PATH_MAX = 256;
        static constexpr size_t FILE_BUFFER = 512;
    };

    /**
     * @brief Compile-time unit conversions
     */
    struct Conversions {
        // Time conversions
        template<uint32_t seconds>
        static constexpr uint32_t secondsToMillis() {
            return seconds * Constants::MILLISECONDS_PER_SECOND;
        }
        
        template<uint32_t millis>
        static constexpr uint32_t millisToMicros() {
            return millis * 1000;
        }
        
        template<uint32_t seconds>
        static constexpr uint32_t secondsToMicros() {
            return seconds * Constants::MICROSECONDS_PER_SECOND;
        }
        
        // Size conversions
        template<uint32_t kb>
        static constexpr uint32_t kilobytesToBytes() {
            return kb * 1024;
        }
        
        template<uint32_t mb>
        static constexpr uint32_t megabytesToBytes() {
            return mb * 1024 * 1024;
        }
        
        // Frequency conversions
        template<uint32_t hz>
        static constexpr uint32_t hzToKhz() {
            return hz / 1000;
        }
        
        template<uint32_t khz>
        static constexpr uint32_t khzToMhz() {
            return khz / 1000;
        }
    };

    /**
     * @brief Compile-time validation utilities
     */
    template<bool condition>
    struct StaticAssert {
        static_assert(condition, "Compile-time assertion failed");
        static constexpr bool value = condition;
    };

    // Helper macros for common compile-time checks
    #define COMPILE_TIME_ASSERT(condition, message) \
        static_assert(condition, message)
        
    #define COMPILE_TIME_ARRAY_SIZE(array) \
        (sizeof(array) / sizeof((array)[0]))
        
    #define COMPILE_TIME_OFFSET(type, member) \
        ((size_t)&(((type*)0)->member))

} // namespace CompileTime

/**
 * @brief Compile-time configuration validation
 */
namespace ConfigValidation {
    // Validate buffer sizes are reasonable
    COMPILE_TIME_ASSERT(CompileTime::BufferSizes::NTP_PACKET == 48, "NTP packet must be 48 bytes");
    COMPILE_TIME_ASSERT(CompileTime::BufferSizes::LOG_MESSAGE_MAX <= 512, "Log message buffer too large");
    COMPILE_TIME_ASSERT(CompileTime::BufferSizes::JSON_BUFFER_MAX <= 2048, "JSON buffer might be too large for embedded system");
    
    // Validate time constants
    COMPILE_TIME_ASSERT(CompileTime::Constants::SECONDS_PER_DAY == 24 * 60 * 60, "Incorrect seconds per day");
    COMPILE_TIME_ASSERT(CompileTime::Constants::MICROSECONDS_PER_SECOND == 1000000, "Incorrect microseconds per second");
}

#endif // COMPILE_TIME_UTILS_H