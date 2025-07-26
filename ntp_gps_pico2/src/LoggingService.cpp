#include "LoggingService.h"
#include "TimeManager.h"
#include <Ethernet.h>
#include <stdarg.h>
#include <stdio.h>

// Global logger instance pointer
LoggingService* globalLogger = nullptr;

LoggingService::LoggingService(EthernetUDP* udpInstance, TimeManager* timeManagerInstance) 
    : udp(udpInstance), timeManager(timeManagerInstance), logBuffer(nullptr), bufferTail(nullptr), 
      bufferCount(0), lastRetransmit(0) {
    
    // Initialize default configuration
    config.minLevel = LOG_INFO;
    config.syslogPort = 514;
    config.facility = FACILITY_DAEMON;
    config.localBuffering = true;
    config.maxBufferEntries = 50;
    config.retransmitInterval = 30000; // 30 seconds
    config.maxRetransmitAttempts = 3;
    strcpy(config.syslogServer, "");
}

LoggingService::~LoggingService() {
    clearBuffers();
    if (globalLogger == this) {
        globalLogger = nullptr;
    }
}

void LoggingService::init(const LogConfig& configuration) {
    config = configuration;
    clearBuffers(); // Clear any existing buffers
    
    // Set this as the global logger if none exists
    if (!globalLogger) {
        globalLogger = this;
    }
    
    Serial.print("LoggingService initialized - Min Level: ");
    Serial.print(getLevelName(config.minLevel));
    Serial.print(", Syslog Server: ");
    Serial.print(strlen(config.syslogServer) > 0 ? config.syslogServer : "Not configured");
    Serial.print(":");
    Serial.println(config.syslogPort);
}

void LoggingService::setSyslogServer(const char* server, uint16_t port) {
    strncpy(config.syslogServer, server, sizeof(config.syslogServer) - 1);
    config.syslogServer[sizeof(config.syslogServer) - 1] = '\0';
    config.syslogPort = port;
}

bool LoggingService::isSyslogServerConfigured() const {
    return strlen(config.syslogServer) > 0;
}

void LoggingService::log(LogLevel level, const char* tag, const char* message) {
    log(level, config.facility, tag, message);
}

void LoggingService::log(LogLevel level, LogFacility facility, const char* tag, const char* message) {
    // Check if log level is enabled
    if (level > config.minLevel) {
        return;
    }
    
    // Always output to serial using the console format
    char consoleBuffer[512];
    formatConsoleMessage(consoleBuffer, sizeof(consoleBuffer), level, tag, message);
    Serial.println(consoleBuffer);
    
    // Add to buffer if local buffering is enabled
    if (config.localBuffering) {
        addToBuffer(level, facility, tag, message);
    } else if (isSyslogServerConfigured()) {
        // Try immediate transmission
        LogEntry immediateEntry;
        immediateEntry.timestamp = millis();
        immediateEntry.level = level;
        immediateEntry.facility = facility;
        strncpy(immediateEntry.message, message, sizeof(immediateEntry.message) - 1);
        immediateEntry.message[sizeof(immediateEntry.message) - 1] = '\0';
        strncpy(immediateEntry.tag, tag, sizeof(immediateEntry.tag) - 1);
        immediateEntry.tag[sizeof(immediateEntry.tag) - 1] = '\0';
        immediateEntry.transmitted = false;
        immediateEntry.next = nullptr;
        
        transmitLogEntry(&immediateEntry);
    }
}

void LoggingService::logf(LogLevel level, const char* tag, const char* format, ...) {
    logf(level, config.facility, tag, format);
}

void LoggingService::logf(LogLevel level, LogFacility facility, const char* tag, const char* format, ...) {
    if (level > config.minLevel) {
        return;
    }
    
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    log(level, facility, tag, buffer);
}

// Formatted convenience method implementations
void LoggingService::emergencyf(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    emergency(tag, buffer);
}

void LoggingService::alertf(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    alert(tag, buffer);
}

void LoggingService::criticalf(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    critical(tag, buffer);
}

void LoggingService::errorf(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    error(tag, buffer);
}

void LoggingService::warningf(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    warning(tag, buffer);
}

void LoggingService::noticef(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    notice(tag, buffer);
}

void LoggingService::infof(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    info(tag, buffer);
}

void LoggingService::debugf(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    debug(tag, buffer);
}

void LoggingService::addToBuffer(LogLevel level, LogFacility facility, const char* tag, const char* message) {
    // Create new log entry
    LogEntry* newEntry = new LogEntry;
    if (!newEntry) {
        Serial.println("ERROR: Failed to allocate memory for log entry");
        return;
    }
    
    newEntry->timestamp = millis();
    newEntry->level = level;
    newEntry->facility = facility;
    strncpy(newEntry->message, message, sizeof(newEntry->message) - 1);
    newEntry->message[sizeof(newEntry->message) - 1] = '\0';
    strncpy(newEntry->tag, tag, sizeof(newEntry->tag) - 1);
    newEntry->tag[sizeof(newEntry->tag) - 1] = '\0';
    newEntry->transmitted = false;
    newEntry->next = nullptr;
    
    // Add to tail of linked list
    if (!logBuffer) {
        logBuffer = newEntry;
        bufferTail = newEntry;
    } else {
        bufferTail->next = newEntry;
        bufferTail = newEntry;
    }
    
    bufferCount++;
    
    // Trim buffer if it exceeds maximum entries
    if (bufferCount > config.maxBufferEntries) {
        trimBuffer();
    }
}

void LoggingService::trimBuffer() {
    while (bufferCount > config.maxBufferEntries && logBuffer) {
        LogEntry* toDelete = logBuffer;
        logBuffer = logBuffer->next;
        delete toDelete;
        bufferCount--;
    }
    
    // Update tail pointer if buffer is now empty
    if (!logBuffer) {
        bufferTail = nullptr;
    }
}

int LoggingService::calculatePriority(LogFacility facility, LogLevel level) {
    return (facility * 8) + level;
}

void LoggingService::getCurrentTimestamp(char* buffer, size_t bufferSize) {
    unsigned long now = millis();
    unsigned long seconds = now / 1000;
    
    // Simple timestamp format for embedded system
    // Format: MMM DD HH:MM:SS (we'll use a basic format)
    snprintf(buffer, bufferSize, "%lu", seconds);
}

void LoggingService::formatSyslogMessage(char* buffer, size_t bufferSize, int priority,
                                       const char* timestamp, const char* hostname,
                                       const char* tag, const char* message) {
    // RFC 3164 Syslog format: <Priority>Timestamp Hostname Tag: Message
    snprintf(buffer, bufferSize, "<%d>%s %s %s: %s",
             priority, timestamp, hostname, tag, message);
}

bool LoggingService::transmitLogEntry(const LogEntry* entry) {
    if (!isSyslogServerConfigured() || !udp) {
        return false;
    }
    
    char timestampBuffer[32];
    char hostnameBuffer[32];
    char syslogBuffer[512];
    
    getCurrentTimestamp(timestampBuffer, sizeof(timestampBuffer));
    
    // Get hostname (use IP address if no hostname available)
    IPAddress localIP = Ethernet.localIP();
    snprintf(hostnameBuffer, sizeof(hostnameBuffer), "%d.%d.%d.%d",
             localIP[0], localIP[1], localIP[2], localIP[3]);
    
    int priority = calculatePriority(entry->facility, entry->level);
    formatSyslogMessage(syslogBuffer, sizeof(syslogBuffer), priority,
                       timestampBuffer, hostnameBuffer, entry->tag, entry->message);
    
    // Transmit via UDP
    if (udp->beginPacket(config.syslogServer, config.syslogPort)) {
        udp->write(syslogBuffer);
        bool success = udp->endPacket();
        
        if (success) {
            Serial.print("Syslog transmitted: ");
            Serial.println(syslogBuffer);
        } else {
            Serial.print("Failed to transmit syslog: ");
            Serial.println(syslogBuffer);
        }
        
        return success;
    }
    
    return false;
}

void LoggingService::processRetransmissions() {
    if (!isSyslogServerConfigured() || 
        (millis() - lastRetransmit < config.retransmitInterval)) {
        return;
    }
    
    lastRetransmit = millis();
    
    // Retry transmission of failed entries
    LogEntry* current = logBuffer;
    while (current) {
        if (!current->transmitted) {
            if (transmitLogEntry(current)) {
                current->transmitted = true;
            }
        }
        current = current->next;
    }
}

void LoggingService::processLogs() {
    if (!config.localBuffering) {
        return;
    }
    
    // Process immediate transmissions for new entries
    LogEntry* current = logBuffer;
    while (current) {
        if (!current->transmitted && isSyslogServerConfigured()) {
            if (transmitLogEntry(current)) {
                current->transmitted = true;
            }
        }
        current = current->next;
    }
    
    // Handle retransmissions
    processRetransmissions();
}

void LoggingService::flushBuffers() {
    if (!isSyslogServerConfigured()) {
        Serial.println("Cannot flush logs: Syslog server not configured");
        return;
    }
    
    LogEntry* current = logBuffer;
    int transmitted = 0;
    int failed = 0;
    
    while (current) {
        if (transmitLogEntry(current)) {
            current->transmitted = true;
            transmitted++;
        } else {
            failed++;
        }
        current = current->next;
    }
    
    Serial.print("Log flush completed - Transmitted: ");
    Serial.print(transmitted);
    Serial.print(", Failed: ");
    Serial.println(failed);
}

void LoggingService::clearBuffers() {
    while (logBuffer) {
        LogEntry* toDelete = logBuffer;
        logBuffer = logBuffer->next;
        delete toDelete;
    }
    
    bufferTail = nullptr;
    bufferCount = 0;
}

const char* LoggingService::getLevelName(LogLevel level) const {
    switch (level) {
        case LOG_EMERG: return "EMERGENC";
        case LOG_ALERT: return "ALERT   ";
        case LOG_CRIT: return "CRITICAL";
        case LOG_ERR: return "ERROR   ";
        case LOG_WARNING: return "WARNING ";
        case LOG_NOTICE: return "NOTICE  ";
        case LOG_INFO: return "INFO    ";
        case LOG_DEBUG: return "DEBUG   ";
        default: return "UNKNOWN ";
    }
}

const char* LoggingService::getFacilityName(LogFacility facility) const {
    switch (facility) {
        case FACILITY_KERNEL: return "KERNEL";
        case FACILITY_USER: return "USER";
        case FACILITY_MAIL: return "MAIL";
        case FACILITY_DAEMON: return "DAEMON";
        case FACILITY_SECURITY: return "SECURITY";
        case FACILITY_SYSLOGD: return "SYSLOGD";
        case FACILITY_LPR: return "LPR";
        case FACILITY_NEWS: return "NEWS";
        case FACILITY_UUCP: return "UUCP";
        case FACILITY_CRON: return "CRON";
        case FACILITY_AUTHPRIV: return "AUTHPRIV";
        case FACILITY_FTP: return "FTP";
        case FACILITY_NTP: return "NTP";
        case FACILITY_LOG_AUDIT: return "LOG_AUDIT";
        case FACILITY_LOG_ALERT: return "LOG_ALERT";
        case FACILITY_CLOCK: return "CLOCK";
        case FACILITY_LOCAL0: return "LOCAL0";
        case FACILITY_LOCAL1: return "LOCAL1";
        case FACILITY_LOCAL2: return "LOCAL2";
        case FACILITY_LOCAL3: return "LOCAL3";
        case FACILITY_LOCAL4: return "LOCAL4";
        case FACILITY_LOCAL5: return "LOCAL5";
        case FACILITY_LOCAL6: return "LOCAL6";
        case FACILITY_LOCAL7: return "LOCAL7";
        default: return "UNKNOWN";
    }
}

void LoggingService::getConsoleTimestamp(char* buffer, size_t bufferSize) {
    if (timeManager) {
        // Use GPS-synchronized time when available
        time_t unixTime = timeManager->getUnixTimestamp();
        uint32_t microseconds = timeManager->getMicrosecondFraction();
        uint32_t milliseconds = microseconds / 1000;
        
        if (unixTime > 0) {
            // Convert Unix timestamp to UTC time
            struct tm* timeinfo = gmtime(&unixTime);
            if (timeinfo) {
                snprintf(buffer, bufferSize, "%04d-%02d-%02d %02d:%02d:%02d.%03lu",
                         timeinfo->tm_year + 1900,
                         timeinfo->tm_mon + 1,
                         timeinfo->tm_mday,
                         timeinfo->tm_hour,
                         timeinfo->tm_min,
                         timeinfo->tm_sec,
                         milliseconds);
                return;
            }
        }
    }
    
    // Fallback to millis() based time when GPS time is not available
    unsigned long now = millis();
    unsigned long totalSeconds = now / 1000;
    unsigned long milliseconds = now % 1000;
    
    // Convert to hours, minutes, seconds (simple calculation for demo)
    unsigned long hours = (totalSeconds / 3600) % 24;
    unsigned long minutes = (totalSeconds % 3600) / 60;
    unsigned long seconds = totalSeconds % 60;
    
    // Use placeholder date when GPS time is not synchronized
    // Format: YYYY-MM-DD HH:MM:SS.mmm
    snprintf(buffer, bufferSize, "2025-07-26 %02lu:%02lu:%02lu.%03lu", 
             hours, minutes, seconds, milliseconds);
}

const char* LoggingService::getComponentName(const char* tag) const {
    // Map tag names to 12-character fixed-width component names
    if (strcmp(tag, "SYSTEM") == 0) return "SYSTEM     ";
    if (strcmp(tag, "GPS") == 0) return "GPS        ";
    if (strcmp(tag, "NTP") == 0) return "NTP        ";
    if (strcmp(tag, "NETWORK") == 0) return "NETWORK    ";
    if (strcmp(tag, "DISPLAY") == 0) return "DISPLAY    ";
    if (strcmp(tag, "CONFIG") == 0) return "CONFIG     ";
    if (strcmp(tag, "METRICS") == 0) return "METRICS    ";
    if (strcmp(tag, "HARDWARE") == 0) return "HARDWARE   ";
    if (strcmp(tag, "BUTTON") == 0) return "BUTTON     ";
    if (strcmp(tag, "STORAGE") == 0) return "STORAGE    ";
    if (strcmp(tag, "ERROR_HDL") == 0) return "ERROR_HDL  ";
    if (strcmp(tag, "TEST") == 0) return "TEST       ";
    
    // For unknown tags, pad to 12 characters
    static char padded[13];
    snprintf(padded, sizeof(padded), "%-11s", tag);
    padded[11] = ' ';
    padded[12] = '\0';
    return padded;
}

void LoggingService::formatConsoleMessage(char* buffer, size_t bufferSize, LogLevel level, 
                                        const char* tag, const char* message) {
    char timestamp[32];
    getConsoleTimestamp(timestamp, sizeof(timestamp));
    
    const char* levelName = getLevelName(level);
    const char* componentName = getComponentName(tag);
    
    // Format: [YYYY-MM-DD HH:MM:SS.mmm] [LEVEL] [COMPONENT] Message
    snprintf(buffer, bufferSize, "[%s] [%s] [%s] %s", 
             timestamp, levelName, componentName, message);
}