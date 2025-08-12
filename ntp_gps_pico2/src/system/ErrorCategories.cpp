#include "ErrorCategories.h"
#include "../config/LoggingService.h"
#include <Arduino.h>

namespace ErrorCategories {
    
    // Predefined error strategies
    const ErrorStrategy HARDWARE_I2C_STRATEGY = {
        RecoveryAction::RETRY_OPERATION,    // Primary: retry with different parameters
        RecoveryAction::RESET_COMPONENT,    // Fallback: reset I2C subsystem
        RecoveryTiming::DELAYED_SHORT,      // Wait a bit before retry
        ImpactLevel::MEDIUM,                // Medium impact on functionality
        3,                                  // Max 3 retries
        1000,                              // 1 second delay
        false,                             // No reboot required
        true,                              // Log persistently
        "I2C communication failure - retry with reset fallback"
    };
    
    const ErrorStrategy HARDWARE_SPI_STRATEGY = {
        RecoveryAction::RESET_COMPONENT,    // Primary: reset SPI subsystem
        RecoveryAction::RESTART_SERVICE,    // Fallback: restart affected service
        RecoveryTiming::IMMEDIATE,          // Reset immediately
        ImpactLevel::HIGH,                  // High impact (Ethernet depends on SPI)
        2,                                  // Max 2 retries
        500,                               // 0.5 second delay
        false,                             // No reboot required
        true,                              // Log persistently
        "SPI communication failure - reset subsystem"
    };
    
    const ErrorStrategy NETWORK_ETHERNET_STRATEGY = {
        RecoveryAction::RESTART_SERVICE,    // Primary: restart network service
        RecoveryAction::RESET_COMPONENT,    // Fallback: reset ethernet hardware
        RecoveryTiming::DELAYED_MEDIUM,     // Wait for network to stabilize
        ImpactLevel::HIGH,                  // High impact on NTP functionality
        5,                                  // Max 5 retries
        30000,                             // 30 second delay
        false,                             // No reboot required
        true,                              // Log persistently
        "Ethernet connection failure - restart service with hardware reset fallback"
    };
    
    const ErrorStrategy NETWORK_DHCP_STRATEGY = {
        RecoveryAction::RETRY_OPERATION,    // Primary: retry DHCP request
        RecoveryAction::FALLBACK_MODE,      // Fallback: use static IP
        RecoveryTiming::DELAYED_SHORT,      // Short delay for network
        ImpactLevel::MEDIUM,                // Medium impact
        3,                                  // Max 3 retries
        10000,                             // 10 second delay
        false,                             // No reboot required
        false,                             // Don't log persistently
        "DHCP acquisition failure - retry with static IP fallback"
    };
    
    const ErrorStrategy GPS_PPS_STRATEGY = {
        RecoveryAction::RETRY_OPERATION,    // Primary: check PPS signal again
        RecoveryAction::FALLBACK_MODE,      // Fallback: use RTC time
        RecoveryTiming::DELAYED_SHORT,      // Short delay
        ImpactLevel::HIGH,                  // High impact on time accuracy
        10,                                // Max 10 retries (PPS can be intermittent)
        5000,                              // 5 second delay
        false,                             // No reboot required
        true,                              // Log persistently
        "PPS signal lost - retry with RTC fallback"
    };
    
    const ErrorStrategy GPS_SYNC_STRATEGY = {
        RecoveryAction::RESTART_SERVICE,    // Primary: restart GPS service
        RecoveryAction::FALLBACK_MODE,      // Fallback: use RTC only
        RecoveryTiming::DELAYED_MEDIUM,     // Medium delay for GPS acquisition
        ImpactLevel::CRITICAL,              // Critical for NTP server accuracy
        3,                                  // Max 3 retries
        60000,                             // 60 second delay
        false,                             // No reboot required
        true,                              // Log persistently
        "GPS time sync failure - restart service with RTC fallback"
    };
    
    const ErrorStrategy SYSTEM_MEMORY_STRATEGY = {
        RecoveryAction::LOG_ONLY,          // Primary: log for analysis
        RecoveryAction::RESTART_SYSTEM,    // Fallback: restart if critical
        RecoveryTiming::IMMEDIATE,         // Act immediately
        ImpactLevel::CRITICAL,             // Critical system issue
        1,                                 // Only 1 retry (memory issues are serious)
        0,                                 // No delay
        true,                              // Reboot required for cleanup
        true,                              // Log persistently
        "Memory allocation failure - log and restart if critical"
    };
    
    const ErrorStrategy SYSTEM_CONFIG_STRATEGY = {
        RecoveryAction::FALLBACK_MODE,     // Primary: use factory defaults
        RecoveryAction::FACTORY_RESET,     // Fallback: full factory reset
        RecoveryTiming::IMMEDIATE,         // Fix immediately
        ImpactLevel::MEDIUM,               // Medium impact
        1,                                 // Only 1 retry
        0,                                 // No delay
        true,                              // Reboot required for config reload
        true,                              // Log persistently
        "Configuration corruption - use defaults or factory reset"
    };
    
    const ErrorStrategy& getStrategy(ErrorType type, const char* component) {
        // Component-specific strategy selection
        if (strstr(component, "I2C") || strstr(component, "i2c")) {
            return HARDWARE_I2C_STRATEGY;
        }
        if (strstr(component, "SPI") || strstr(component, "spi")) {
            return HARDWARE_SPI_STRATEGY;
        }
        if (strstr(component, "GPS") || strstr(component, "gps")) {
            if (strstr(component, "PPS") || strstr(component, "pps")) {
                return GPS_PPS_STRATEGY;
            }
            return GPS_SYNC_STRATEGY;
        }
        
        // Type-based strategy selection
        switch (type) {
            case ErrorType::HARDWARE_FAILURE:
                return HARDWARE_I2C_STRATEGY;  // Default hardware strategy
            case ErrorType::NETWORK_ERROR:
                if (strstr(component, "DHCP") || strstr(component, "dhcp")) {
                    return NETWORK_DHCP_STRATEGY;
                }
                return NETWORK_ETHERNET_STRATEGY;
            case ErrorType::GPS_ERROR:
                return GPS_SYNC_STRATEGY;
            case ErrorType::MEMORY_ERROR:
                return SYSTEM_MEMORY_STRATEGY;
            case ErrorType::CONFIGURATION_ERROR:
            case ErrorType::DATA_CORRUPTION:
                return SYSTEM_CONFIG_STRATEGY;
            default:
                return HARDWARE_I2C_STRATEGY;  // Safe default
        }
    }
    
    const char* getCategoryName(ErrorType type) {
        switch (type) {
            case ErrorType::HARDWARE_FAILURE:    return "Hardware";
            case ErrorType::COMMUNICATION_ERROR: return "Communication";
            case ErrorType::MEMORY_ERROR:        return "Memory";
            case ErrorType::CONFIGURATION_ERROR: return "Configuration";
            case ErrorType::TIMEOUT_ERROR:       return "Timeout";
            case ErrorType::DATA_CORRUPTION:     return "Data";
            case ErrorType::NETWORK_ERROR:       return "Network";
            case ErrorType::GPS_ERROR:           return "GPS";
            case ErrorType::NTP_ERROR:           return "NTP";
            case ErrorType::SYSTEM_ERROR:        return "System";
            default:                             return "Unknown";
        }
    }
    
    const char* getSeverityName(ErrorSeverity severity) {
        switch (severity) {
            case ErrorSeverity::INFO:     return "Info";
            case ErrorSeverity::WARNING:  return "Warning";
            case ErrorSeverity::ERROR:    return "Error";
            case ErrorSeverity::CRITICAL: return "Critical";
            case ErrorSeverity::FATAL:    return "Fatal";
            default:                      return "Unknown";
        }
    }
    
    const char* getRecoveryActionName(RecoveryAction action) {
        switch (action) {
            case RecoveryAction::LOG_ONLY:        return "Log Only";
            case RecoveryAction::RETRY_OPERATION: return "Retry";
            case RecoveryAction::RESET_COMPONENT: return "Reset Component";
            case RecoveryAction::FALLBACK_MODE:   return "Fallback Mode";
            case RecoveryAction::RESTART_SERVICE: return "Restart Service";
            case RecoveryAction::RESTART_SYSTEM:  return "Restart System";
            case RecoveryAction::FACTORY_RESET:   return "Factory Reset";
            case RecoveryAction::EMERGENCY_STOP:  return "Emergency Stop";
            default:                              return "Unknown";
        }
    }
    
    void reportCategorizedError(ErrorType type, const char* component, 
                               const char* details, HardwareError subtype) {
        if (globalErrorHandler) {
            const ErrorStrategy& strategy = getStrategy(type, component);
            
            // Enhanced error message with strategy information
            char enhancedMessage[256];
            snprintf(enhancedMessage, sizeof(enhancedMessage), 
                    "%s - Recovery: %s", details, 
                    getRecoveryActionName(strategy.primaryAction));
            
            ErrorSeverity severity;
            switch (strategy.impact) {
                case ImpactLevel::MINIMAL:
                case ImpactLevel::LOW:      severity = ErrorSeverity::WARNING; break;
                case ImpactLevel::MEDIUM:   severity = ErrorSeverity::ERROR; break;
                case ImpactLevel::HIGH:     severity = ErrorSeverity::CRITICAL; break;
                case ImpactLevel::CRITICAL:
                case ImpactLevel::CATASTROPHIC: severity = ErrorSeverity::FATAL; break;
                default:                    severity = ErrorSeverity::ERROR; break;
            }
            
            globalErrorHandler->reportError(type, severity, component, enhancedMessage, details);
            
            // Schedule recovery if appropriate
            if (strategy.timing != RecoveryTiming::MANUAL) {
                scheduleRecovery(strategy, component, strategy.retryDelay);
            }
        }
    }
    
    void reportNetworkError(NetworkError subtype, const char* component, const char* details) {
        const char* subtypeNames[] = {
            "Ethernet Disconnected", "DHCP Failure", "DNS Failure", 
            "TCP Connection Failed", "UDP Socket Error", "Timeout Error",
            "Protocol Error", "Certificate Error"
        };
        
        char message[256];
        snprintf(message, sizeof(message), "%s: %s", 
                subtypeNames[static_cast<int>(subtype)], details);
        
        reportCategorizedError(ErrorType::NETWORK_ERROR, component, message);
    }
    
    void reportGpsError(GpsError subtype, const char* details) {
        const char* subtypeNames[] = {
            "No Satellites", "PPS Signal Lost", "Time Sync Failed",
            "Configuration Error", "Communication Timeout", "Data Corruption",
            "Antenna Problem", "Cold Start Failed"
        };
        
        char message[256];
        snprintf(message, sizeof(message), "%s: %s", 
                subtypeNames[static_cast<int>(subtype)], details);
        
        reportCategorizedError(ErrorType::GPS_ERROR, "GPS", message);
    }
    
    void reportSystemError(SystemError subtype, const char* component, const char* details) {
        const char* subtypeNames[] = {
            "Out of Memory", "Stack Overflow", "Watchdog Timeout",
            "Filesystem Full", "Config Corrupted", "Task Overrun",
            "Interrupt Storm", "Thermal Protection"
        };
        
        char message[256];
        snprintf(message, sizeof(message), "%s: %s", 
                subtypeNames[static_cast<int>(subtype)], details);
        
        reportCategorizedError(ErrorType::SYSTEM_ERROR, component, message);
    }
    
    bool executeRecoveryStrategy(const ErrorStrategy& strategy, const char* component) {
        LOG_INFO_F("RECOVERY", "Executing recovery strategy for %s: %s", 
                   component, strategy.description);
        
        // Implementation note: This is a simplified version
        // In a full implementation, each recovery action would have
        // specific code to execute the recovery
        
        switch (strategy.primaryAction) {
            case RecoveryAction::LOG_ONLY:
                LOG_INFO_F("RECOVERY", "Logged error for %s", component);
                return true;
                
            case RecoveryAction::RETRY_OPERATION:
                LOG_INFO_F("RECOVERY", "Scheduled retry for %s", component);
                return true;
                
            case RecoveryAction::RESET_COMPONENT:
                LOG_WARN_F("RECOVERY", "Component reset required for %s", component);
                // Specific component reset logic would go here
                return true;
                
            case RecoveryAction::FALLBACK_MODE:
                LOG_WARN_F("RECOVERY", "Entering fallback mode for %s", component);
                return true;
                
            case RecoveryAction::RESTART_SERVICE:
                LOG_WARN_F("RECOVERY", "Service restart required for %s", component);
                return true;
                
            case RecoveryAction::RESTART_SYSTEM:
                LOG_ERR_F("RECOVERY", "System restart required due to %s", component);
                return false;
                
            case RecoveryAction::FACTORY_RESET:
                LOG_ERR_F("RECOVERY", "Factory reset required due to %s", component);
                return false;
                
            case RecoveryAction::EMERGENCY_STOP:
                LOG_EMERG_F("RECOVERY", "Emergency stop triggered by %s", component);
                return false;
                
            default:
                LOG_WARN_F("RECOVERY", "Unknown recovery action for %s", component);
                return false;
        }
    }
    
    void scheduleRecovery(const ErrorStrategy& strategy, const char* component, unsigned long delay) {
        // In a full implementation, this would add the recovery to a scheduler
        LOG_INFO_F("RECOVERY", "Scheduled recovery for %s in %lu ms", component, delay);
    }
    
    ErrorAnalysis analyzeError(const ErrorInfo& error) {
        ErrorAnalysis analysis = {};
        analysis.primaryType = error.type;
        
        // Simple heuristics for error analysis
        const ErrorStrategy& strategy = getStrategy(error.type, error.component);
        analysis.impact = strategy.impact;
        
        // Estimate probability based on error type
        switch (error.type) {
            case ErrorType::NETWORK_ERROR:
                analysis.probability = 0.3f;  // Network issues are common
                analysis.mttr = 60000;        // 1 minute typical recovery
                break;
            case ErrorType::GPS_ERROR:
                analysis.probability = 0.2f;  // GPS issues are less common
                analysis.mttr = 120000;       // 2 minutes typical recovery
                break;
            case ErrorType::HARDWARE_FAILURE:
                analysis.probability = 0.1f;  // Hardware issues are rare
                analysis.mttr = 300000;       // 5 minutes typical recovery
                break;
            default:
                analysis.probability = 0.15f;
                analysis.mttr = 90000;
                break;
        }
        
        // Check if it's recurring (simplified)
        analysis.isRecurring = (error.retryCount > 1);
        analysis.occurrenceCount = error.retryCount + 1;
        
        // Provide basic root cause and prevention tips
        switch (error.type) {
            case ErrorType::NETWORK_ERROR:
                analysis.rootCause = "Network connectivity or configuration issue";
                analysis.preventionTips = "Check cable connections, verify network settings";
                break;
            case ErrorType::GPS_ERROR:
                analysis.rootCause = "GPS signal reception or hardware issue";
                analysis.preventionTips = "Check antenna placement, verify GPS module connection";
                break;
            case ErrorType::HARDWARE_FAILURE:
                analysis.rootCause = "Hardware component malfunction";
                analysis.preventionTips = "Check connections, verify power supply stability";
                break;
            default:
                analysis.rootCause = "System operational issue";
                analysis.preventionTips = "Monitor system logs, perform regular maintenance";
                break;
        }
        
        return analysis;
    }
    
    void updateErrorTrends(const ErrorInfo& error) {
        // In a full implementation, this would maintain error trend statistics
        // For now, just log the trend update
        LOG_DEBUG_F("ANALYSIS", "Updated error trends for %s: %s", 
                   error.component, getCategoryName(error.type));
    }
    
} // namespace ErrorCategories