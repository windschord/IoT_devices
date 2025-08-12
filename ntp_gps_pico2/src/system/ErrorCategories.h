#ifndef ERROR_CATEGORIES_H
#define ERROR_CATEGORIES_H

#include "ErrorHandler.h"

/**
 * @brief Standardized error categories and recovery strategies
 * 
 * This file provides detailed categorization of errors and their
 * corresponding recovery strategies for consistent system behavior.
 */

namespace ErrorCategories {
    
    // Detailed error subcategories
    enum class HardwareError {
        I2C_FAILURE,          // I2C communication failure
        SPI_FAILURE,          // SPI communication failure
        GPIO_FAILURE,         // GPIO operation failure
        CLOCK_FAILURE,        // System clock failure
        POWER_FAILURE,        // Power supply issue
        SENSOR_FAILURE,       // Sensor hardware failure
        DISPLAY_FAILURE,      // Display hardware failure
        STORAGE_FAILURE       // Storage device failure
    };
    
    enum class NetworkError {
        ETHERNET_DISCONNECTED,    // Physical ethernet disconnection
        DHCP_FAILURE,            // DHCP acquisition failure
        DNS_FAILURE,             // DNS resolution failure
        TCP_CONNECTION_FAILED,   // TCP connection establishment failure
        UDP_SOCKET_ERROR,        // UDP socket operation error
        TIMEOUT_ERROR,           // Network operation timeout
        PROTOCOL_ERROR,          // Protocol-level error
        CERTIFICATE_ERROR        // SSL/TLS certificate error
    };
    
    enum class GpsError {
        NO_SATELLITES,           // No satellites visible
        PPS_SIGNAL_LOST,        // PPS signal interruption
        TIME_SYNC_FAILED,       // Time synchronization failure
        CONFIGURATION_ERROR,     // GPS configuration error
        COMMUNICATION_TIMEOUT,   // GPS communication timeout
        DATA_CORRUPTION,        // GPS data corruption
        ANTENNA_PROBLEM,        // Antenna connection issue
        COLD_START_FAILED       // Cold start procedure failed
    };
    
    enum class SystemError {
        OUT_OF_MEMORY,          // Heap allocation failure
        STACK_OVERFLOW,         // Stack overflow detected
        WATCHDOG_TIMEOUT,       // Watchdog timer triggered
        FILESYSTEM_FULL,        // Filesystem full
        CONFIG_CORRUPTED,       // Configuration data corrupted
        TASK_OVERRUN,          // Task execution overrun
        INTERRUPT_STORM,       // Too many interrupts
        THERMAL_PROTECTION     // Temperature protection triggered
    };
    
    // Recovery strategy details
    enum class RecoveryAction {
        LOG_ONLY,              // Only log the error, no action
        RETRY_OPERATION,       // Retry the failed operation
        RESET_COMPONENT,       // Reset the failing component
        FALLBACK_MODE,         // Switch to fallback/safe mode
        RESTART_SERVICE,       // Restart the affected service
        RESTART_SYSTEM,        // Full system restart
        FACTORY_RESET,         // Reset to factory defaults
        EMERGENCY_STOP         // Emergency system shutdown
    };
    
    // Recovery timing
    enum class RecoveryTiming {
        IMMEDIATE,             // Execute recovery immediately
        DELAYED_SHORT,         // Wait 1-5 seconds
        DELAYED_MEDIUM,        // Wait 30-60 seconds
        DELAYED_LONG,          // Wait 5-10 minutes
        SCHEDULED,             // Execute at next maintenance window
        MANUAL                 // Require manual intervention
    };
    
    // Error impact severity
    enum class ImpactLevel {
        MINIMAL,               // No impact on core functionality
        LOW,                   // Minor degradation
        MEDIUM,                // Significant feature impact
        HIGH,                  // Major functionality loss
        CRITICAL,              // System integrity at risk
        CATASTROPHIC           // Complete system failure imminent
    };
    
    // Error handling strategy
    struct ErrorStrategy {
        RecoveryAction primaryAction;
        RecoveryAction fallbackAction;
        RecoveryTiming timing;
        ImpactLevel impact;
        uint8_t maxRetries;
        uint32_t retryDelay;       // Delay between retries (ms)
        bool requiresReboot;       // Whether recovery requires reboot
        bool persistentLogging;    // Whether to log persistently
        const char* description;   // Human-readable description
    };
    
    // Predefined error strategies
    extern const ErrorStrategy HARDWARE_I2C_STRATEGY;
    extern const ErrorStrategy HARDWARE_SPI_STRATEGY;
    extern const ErrorStrategy NETWORK_ETHERNET_STRATEGY;
    extern const ErrorStrategy NETWORK_DHCP_STRATEGY;
    extern const ErrorStrategy GPS_PPS_STRATEGY;
    extern const ErrorStrategy GPS_SYNC_STRATEGY;
    extern const ErrorStrategy SYSTEM_MEMORY_STRATEGY;
    extern const ErrorStrategy SYSTEM_CONFIG_STRATEGY;
    
    // Error classification functions
    const ErrorStrategy& getStrategy(ErrorType type, const char* component);
    const char* getCategoryName(ErrorType type);
    const char* getSeverityName(ErrorSeverity severity);
    const char* getRecoveryActionName(RecoveryAction action);
    
    // Error reporting helpers
    void reportCategorizedError(ErrorType type, const char* component, 
                               const char* details, HardwareError subtype = HardwareError::I2C_FAILURE);
    void reportNetworkError(NetworkError subtype, const char* component, const char* details);
    void reportGpsError(GpsError subtype, const char* details);
    void reportSystemError(SystemError subtype, const char* component, const char* details);
    
    // Recovery strategy execution
    bool executeRecoveryStrategy(const ErrorStrategy& strategy, const char* component);
    void scheduleRecovery(const ErrorStrategy& strategy, const char* component, unsigned long delay);
    
    // Error analysis
    struct ErrorAnalysis {
        ErrorType primaryType;
        ImpactLevel impact;
        float probability;         // Probability of recurrence
        unsigned long mttr;        // Mean time to recovery (ms)
        bool isRecurring;         // Whether this is a recurring error
        unsigned int occurrenceCount;
        const char* rootCause;    // Suspected root cause
        const char* preventionTips; // Prevention recommendations
    };
    
    ErrorAnalysis analyzeError(const ErrorInfo& error);
    void updateErrorTrends(const ErrorInfo& error);
    
} // namespace ErrorCategories

// Convenience macros for categorized error reporting
#define REPORT_HARDWARE_ERROR(subtype, component, details) \
    ErrorCategories::reportCategorizedError(ErrorType::HARDWARE_FAILURE, component, details, ErrorCategories::HardwareError::subtype)

#define REPORT_NETWORK_ERROR_DETAILED(subtype, component, details) \
    ErrorCategories::reportNetworkError(ErrorCategories::NetworkError::subtype, component, details)

#define REPORT_GPS_ERROR_DETAILED(subtype, details) \
    ErrorCategories::reportGpsError(ErrorCategories::GpsError::subtype, details)

#define REPORT_SYSTEM_ERROR_DETAILED(subtype, component, details) \
    ErrorCategories::reportSystemError(ErrorCategories::SystemError::subtype, component, details)

#endif // ERROR_CATEGORIES_H