#ifndef LOGGING_SERVICE_H
#define LOGGING_SERVICE_H

#include <Arduino.h>
#include <EthernetUdp.h>
#include <time.h>

// Forward declaration for TimeManager
class TimeManager;

// RFC 3164 Syslog severity levels
enum LogLevel {
    LOG_EMERG = 0,     // Emergency: system is unusable
    LOG_ALERT = 1,     // Alert: action must be taken immediately
    LOG_CRIT = 2,      // Critical: critical conditions
    LOG_ERR = 3,       // Error: error conditions
    LOG_WARNING = 4,   // Warning: warning conditions
    LOG_NOTICE = 5,    // Notice: normal but significant condition
    LOG_INFO = 6,      // Informational: informational messages
    LOG_DEBUG = 7      // Debug: debug-level messages
};

// RFC 3164 Syslog facility codes
enum LogFacility {
    FACILITY_KERNEL = 0,      // Kernel messages
    FACILITY_USER = 1,        // User-level messages
    FACILITY_MAIL = 2,        // Mail system
    FACILITY_DAEMON = 3,      // System daemons
    FACILITY_SECURITY = 4,    // Security/authorization messages
    FACILITY_SYSLOGD = 5,     // Messages generated internally by syslogd
    FACILITY_LPR = 6,         // Line printer subsystem
    FACILITY_NEWS = 7,        // Network news subsystem
    FACILITY_UUCP = 8,        // UUCP subsystem
    FACILITY_CRON = 9,        // Clock daemon
    FACILITY_AUTHPRIV = 10,   // Security/authorization messages
    FACILITY_FTP = 11,        // FTP daemon
    FACILITY_NTP = 12,        // NTP subsystem
    FACILITY_LOG_AUDIT = 13,  // Log audit
    FACILITY_LOG_ALERT = 14,  // Log alert
    FACILITY_CLOCK = 15,      // Clock daemon
    FACILITY_LOCAL0 = 16,     // Local use facilities
    FACILITY_LOCAL1 = 17,
    FACILITY_LOCAL2 = 18,
    FACILITY_LOCAL3 = 19,
    FACILITY_LOCAL4 = 20,
    FACILITY_LOCAL5 = 21,
    FACILITY_LOCAL6 = 22,
    FACILITY_LOCAL7 = 23
};

// Log entry structure for local buffering
struct LogEntry {
    unsigned long timestamp;  // millis() timestamp
    LogLevel level;
    LogFacility facility;
    char message[256];        // Log message content
    char tag[32];            // Log tag/component name
    bool transmitted;        // Whether sent to syslog server
    LogEntry* next;          // Linked list pointer
};

// Logging service configuration
struct LogConfig {
    LogLevel minLevel;           // Minimum log level to process
    char syslogServer[64];       // Syslog server hostname/IP
    uint16_t syslogPort;         // Syslog server port (default 514)
    LogFacility facility;        // Default syslog facility
    bool localBuffering;         // Enable local log buffering
    uint16_t maxBufferEntries;   // Maximum buffered log entries
    uint32_t retransmitInterval; // Retransmit interval in milliseconds
    uint16_t maxRetransmitAttempts; // Maximum retransmit attempts
};

class LoggingService {
private:
    LogConfig config;
    EthernetUDP* udp;
    TimeManager* timeManager;    // TimeManager for GPS time sync
    LogEntry* logBuffer;         // Linked list of log entries
    LogEntry* bufferTail;        // Tail of linked list
    uint16_t bufferCount;        // Current number of buffered entries
    unsigned long lastRetransmit; // Last retransmit attempt time
    
    // Private methods
    void addToBuffer(LogLevel level, LogFacility facility, const char* tag, const char* message);
    void trimBuffer();
    int calculatePriority(LogFacility facility, LogLevel level);
    void formatSyslogMessage(char* buffer, size_t bufferSize, int priority, 
                           const char* timestamp, const char* hostname, 
                           const char* tag, const char* message);
    void getCurrentTimestamp(char* buffer, size_t bufferSize);
    void getConsoleTimestamp(char* buffer, size_t bufferSize);
    void formatConsoleMessage(char* buffer, size_t bufferSize, LogLevel level, 
                            const char* tag, const char* message);
    const char* getComponentName(const char* tag) const;
    bool transmitLogEntry(const LogEntry* entry);
    void processRetransmissions();
    
public:
    LoggingService(EthernetUDP* udpInstance, TimeManager* timeManagerInstance = nullptr);
    ~LoggingService();
    
    // Configuration methods
    void init(const LogConfig& configuration);
    void setMinLevel(LogLevel level) { config.minLevel = level; }
    void setSyslogServer(const char* server, uint16_t port = 514);
    void setFacility(LogFacility facility) { config.facility = facility; }
    void setTimeManager(TimeManager* timeManagerInstance) { timeManager = timeManagerInstance; }
    LogLevel getMinLevel() const { return config.minLevel; }
    
    // Logging methods
    void log(LogLevel level, const char* tag, const char* message);
    void log(LogLevel level, LogFacility facility, const char* tag, const char* message);
    void logf(LogLevel level, const char* tag, const char* format, ...);
    void logf(LogLevel level, LogFacility facility, const char* tag, const char* format, ...);
    
    // Convenience methods for different log levels
    void emergency(const char* tag, const char* message) { log(LOG_EMERG, tag, message); }
    void alert(const char* tag, const char* message) { log(LOG_ALERT, tag, message); }
    void critical(const char* tag, const char* message) { log(LOG_CRIT, tag, message); }
    void error(const char* tag, const char* message) { log(LOG_ERR, tag, message); }
    void warning(const char* tag, const char* message) { log(LOG_WARNING, tag, message); }
    void notice(const char* tag, const char* message) { log(LOG_NOTICE, tag, message); }
    void info(const char* tag, const char* message) { log(LOG_INFO, tag, message); }
    void debug(const char* tag, const char* message) { log(LOG_DEBUG, tag, message); }
    
    // Formatted convenience methods
    void emergencyf(const char* tag, const char* format, ...) __attribute__((format(printf, 3, 4)));
    void alertf(const char* tag, const char* format, ...) __attribute__((format(printf, 3, 4)));
    void criticalf(const char* tag, const char* format, ...) __attribute__((format(printf, 3, 4)));
    void errorf(const char* tag, const char* format, ...) __attribute__((format(printf, 3, 4)));
    void warningf(const char* tag, const char* format, ...) __attribute__((format(printf, 3, 4)));
    void noticef(const char* tag, const char* format, ...) __attribute__((format(printf, 3, 4)));
    void infof(const char* tag, const char* format, ...) __attribute__((format(printf, 3, 4)));
    void debugf(const char* tag, const char* format, ...) __attribute__((format(printf, 3, 4)));
    
    // Management methods
    void processLogs();          // Process buffered logs and retransmissions
    void flushBuffers();         // Force flush all buffered logs
    void clearBuffers();         // Clear all buffered logs
    uint16_t getBufferCount() const { return bufferCount; }
    
    // Status methods
    bool isSyslogServerConfigured() const;
    const char* getLevelName(LogLevel level) const;
    const char* getFacilityName(LogFacility facility) const;
};

// Global logging macros for convenience
extern LoggingService* globalLogger;

#define LOG_EMERG_MSG(tag, msg) if (globalLogger) globalLogger->emergency(tag, msg)
#define LOG_ALERT_MSG(tag, msg) if (globalLogger) globalLogger->alert(tag, msg)
#define LOG_CRIT_MSG(tag, msg) if (globalLogger) globalLogger->critical(tag, msg)
#define LOG_ERR_MSG(tag, msg) if (globalLogger) globalLogger->error(tag, msg)
#define LOG_WARN_MSG(tag, msg) if (globalLogger) globalLogger->warning(tag, msg)
#define LOG_NOTICE_MSG(tag, msg) if (globalLogger) globalLogger->notice(tag, msg)
#define LOG_INFO_MSG(tag, msg) if (globalLogger) globalLogger->info(tag, msg)
#define LOG_DEBUG_MSG(tag, msg) if (globalLogger) globalLogger->debug(tag, msg)

#define LOG_EMERG_F(tag, fmt, ...) if (globalLogger) globalLogger->emergencyf(tag, fmt, ##__VA_ARGS__)
#define LOG_ALERT_F(tag, fmt, ...) if (globalLogger) globalLogger->alertf(tag, fmt, ##__VA_ARGS__)
#define LOG_CRIT_F(tag, fmt, ...) if (globalLogger) globalLogger->criticalf(tag, fmt, ##__VA_ARGS__)
#define LOG_ERR_F(tag, fmt, ...) if (globalLogger) globalLogger->errorf(tag, fmt, ##__VA_ARGS__)
#define LOG_WARN_F(tag, fmt, ...) if (globalLogger) globalLogger->warningf(tag, fmt, ##__VA_ARGS__)
#define LOG_NOTICE_F(tag, fmt, ...) if (globalLogger) globalLogger->noticef(tag, fmt, ##__VA_ARGS__)
#define LOG_INFO_F(tag, fmt, ...) if (globalLogger) globalLogger->infof(tag, fmt, ##__VA_ARGS__)
#define LOG_DEBUG_F(tag, fmt, ...) if (globalLogger) globalLogger->debugf(tag, fmt, ##__VA_ARGS__)

#endif // LOGGING_SERVICE_H