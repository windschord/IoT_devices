#ifndef NTP_TYPES_H
#define NTP_TYPES_H

#include <stdint.h>

// Network byte order conversion functions for Arduino/embedded environment
#ifndef ntohl
#define ntohl(x) __builtin_bswap32(x)
#endif
#ifndef htonl
#define htonl(x) __builtin_bswap32(x)
#endif
#ifndef ntohs
#define ntohs(x) __builtin_bswap16(x)
#endif
#ifndef htons
#define htons(x) __builtin_bswap16(x)
#endif

// NTP Constants
#define NTP_PACKET_SIZE 48
#define NTP_TIMESTAMP_DELTA 2208988800UL  // Difference between Unix epoch (1970) and NTP epoch (1900)

// NTP Leap Indicator values
#define NTP_LI_NO_WARNING       0x00  // No leap second warning
#define NTP_LI_LAST_MINUTE_61   0x01  // Last minute of the day has 61 seconds
#define NTP_LI_LAST_MINUTE_59   0x02  // Last minute of the day has 59 seconds
#define NTP_LI_ALARM            0x03  // Clock is unsynchronized

// NTP Version Number
#define NTP_VERSION             4     // NTP Version 4

// NTP Mode values
#define NTP_MODE_RESERVED       0     // Reserved
#define NTP_MODE_SYMMETRIC_ACTIVE 1   // Symmetric active
#define NTP_MODE_SYMMETRIC_PASSIVE 2  // Symmetric passive
#define NTP_MODE_CLIENT         3     // Client
#define NTP_MODE_SERVER         4     // Server
#define NTP_MODE_BROADCAST      5     // Broadcast
#define NTP_MODE_CONTROL        6     // NTP control message
#define NTP_MODE_PRIVATE        7     // Reserved for private use

// NTP Stratum values
#define NTP_STRATUM_UNSPECIFIED 0     // Unspecified or invalid
#define NTP_STRATUM_PRIMARY     1     // Primary reference (GPS, atomic clock, etc.)
#define NTP_STRATUM_SECONDARY_MIN 2   // Secondary reference via NTP
#define NTP_STRATUM_SECONDARY_MAX 15  // Maximum stratum
#define NTP_STRATUM_UNSYNC      16    // Unsynchronized

// NTP Timestamp structure (64-bit)
struct NtpTimestamp {
    uint32_t seconds;      // Seconds since NTP epoch (1900-01-01 00:00:00 UTC)
    uint32_t fraction;     // Fractional part (in units of 2^-32 seconds)
};

// NTP Packet structure (48 bytes) - RFC 5905
struct NtpPacket {
    uint8_t  li_vn_mode;          // Leap Indicator (2) + Version Number (3) + Mode (3)
    uint8_t  stratum;             // Stratum level (0-16)
    int8_t   poll;                // Maximum interval between successive messages (log2 seconds)
    int8_t   precision;           // Precision of the local clock (log2 seconds)
    uint32_t root_delay;          // Total round-trip delay to primary reference source
    uint32_t root_dispersion;     // Maximum error due to clock frequency tolerance
    uint32_t reference_id;        // Reference source identifier
    NtpTimestamp reference_timestamp;  // Time when local clock was last set or corrected
    NtpTimestamp origin_timestamp;     // Time at client when request departed for server
    NtpTimestamp receive_timestamp;    // Time at server when request arrived from client
    NtpTimestamp transmit_timestamp;   // Time at server when response departed for client
};

// NTP Statistics for monitoring
struct NtpStatistics {
    uint32_t requests_total;      // Total number of NTP requests processed
    uint32_t requests_valid;      // Number of valid requests
    uint32_t requests_invalid;    // Number of invalid requests
    uint32_t responses_sent;      // Number of responses sent
    uint32_t last_request_time;   // Timestamp of last request (millis)
    float avg_processing_time;    // Average processing time in milliseconds
    uint32_t clients_served;      // Number of unique clients served
};

// Helper macros for NTP packet field access
#define NTP_GET_LI(li_vn_mode)      (((li_vn_mode) >> 6) & 0x03)
#define NTP_GET_VN(li_vn_mode)      (((li_vn_mode) >> 3) & 0x07)
#define NTP_GET_MODE(li_vn_mode)    ((li_vn_mode) & 0x07)
#define NTP_SET_LI_VN_MODE(li, vn, mode) (((li & 0x03) << 6) | ((vn & 0x07) << 3) | (mode & 0x07))

// Reference identifier for GPS source
#define NTP_REFID_GPS   0x47505300    // "GPS\0" in network byte order

// Helper functions for timestamp conversion
inline NtpTimestamp unixToNtpTimestamp(uint32_t unixSeconds, uint32_t microseconds = 0) {
    NtpTimestamp ntp;
    ntp.seconds = unixSeconds + NTP_TIMESTAMP_DELTA;
    ntp.fraction = (uint32_t)((uint64_t)microseconds * 4294967296ULL / 1000000ULL);
    return ntp;
}

inline uint32_t ntpToUnixTimestamp(const NtpTimestamp& ntp) {
    return ntp.seconds - NTP_TIMESTAMP_DELTA;
}

// Convert network byte order to host byte order for NTP timestamp
inline NtpTimestamp ntohTimestamp(const NtpTimestamp& ntp) {
    NtpTimestamp result;
    result.seconds = ntohl(ntp.seconds);
    result.fraction = ntohl(ntp.fraction);
    return result;
}

// Convert host byte order to network byte order for NTP timestamp
inline NtpTimestamp htonTimestamp(const NtpTimestamp& ntp) {
    NtpTimestamp result;
    result.seconds = htonl(ntp.seconds);
    result.fraction = htonl(ntp.fraction);
    return result;
}

#endif // NTP_TYPES_H