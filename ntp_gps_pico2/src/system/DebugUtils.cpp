#include "DebugUtils.h"
#include "../config/LoggingService.h"
#include <stdarg.h>

namespace DebugUtils {
    
    // Global state
    static DebugLevel currentDebugLevel = DebugLevel::INFO;
    static CallStack globalCallStack;
    static bool callStackEnabled = false;
    static DebugContext* contextStack[8];
    static int contextStackDepth = 0;
    static MemoryInfo memoryStats = {0};
    static PerformanceMetrics perfMetrics = {0};
    
    #ifdef DEBUG_BUILD
    static bool errorInjectionEnabled = false;
    static unsigned long errorInjectionCounter = 0;
    #endif
    
    // CallStack implementation
    void CallStack::pushFrame(const char* function, const char* file, int line, void* context) {
        if (currentDepth < MAX_STACK_DEPTH) {
            frames[currentDepth] = {function, file, line, millis(), context};
            currentDepth++;
        }
    }
    
    void CallStack::popFrame() {
        if (currentDepth > 0) {
            currentDepth--;
        }
    }
    
    void CallStack::clear() {
        currentDepth = 0;
    }
    
    void CallStack::printStackTrace() const {
        LOG_INFO_MSG("DEBUG", "=== Call Stack Trace ===");
        for (int i = currentDepth - 1; i >= 0; i--) {
            const CallFrame& frame = frames[i];
            LOG_INFO_F("DEBUG", "#%d %s() at %s:%d [%lu ms ago]", 
                      currentDepth - i - 1, frame.function, frame.file, frame.line,
                      millis() - frame.timestamp);
        }
    }
    
    void CallStack::getStackTrace(char* buffer, size_t bufferSize) const {
        if (!buffer || bufferSize == 0) return;
        
        buffer[0] = '\0';
        size_t pos = 0;
        
        for (int i = currentDepth - 1; i >= 0 && pos < bufferSize - 50; i--) {
            const CallFrame& frame = frames[i];
            int written = snprintf(buffer + pos, bufferSize - pos, 
                                  "%s():%d\n", frame.function, frame.line);
            if (written > 0 && pos + written < bufferSize) {
                pos += written;
            }
        }
    }
    
    const CallFrame* CallStack::getFrame(int index) const {
        if (index >= 0 && index < currentDepth) {
            return &frames[index];
        }
        return nullptr;
    }
    
    // Utility functions
    void setDebugLevel(DebugLevel level) {
        currentDebugLevel = level;
        LOG_INFO_F("DEBUG", "Debug level set to %d", static_cast<int>(level));
    }
    
    DebugLevel getDebugLevel() {
        return currentDebugLevel;
    }
    
    MemoryInfo getMemoryInfo() {
        MemoryInfo info = {};
        
        #ifdef ESP32
        info.totalHeap = ESP.getHeapSize();
        info.usedHeap = info.totalHeap - ESP.getFreeHeap();
        info.freeHeap = ESP.getFreeHeap();
        info.maxBlockSize = ESP.getMaxAllocHeap();
        info.minFreeHeap = ESP.getMinFreeHeap();
        #elif defined(ARDUINO_ARCH_RP2040)
        // For Raspberry Pi Pico, we'll use approximations
        extern char __end;
        extern char *__brkval;
        info.freeHeap = (char*)malloc(4) - (char*)&__end;
        free((void*)((char*)malloc(4)));
        info.totalHeap = 264 * 1024;  // 264KB RAM on Pico 2
        info.usedHeap = info.totalHeap - info.freeHeap;
        info.maxBlockSize = info.freeHeap;
        #else
        // Generic Arduino approximation
        info.freeHeap = 1024;  // Placeholder
        info.totalHeap = 2048; // Placeholder
        info.usedHeap = info.totalHeap - info.freeHeap;
        #endif
        
        // Update tracked statistics
        info.allocations = memoryStats.allocations;
        info.deallocations = memoryStats.deallocations;
        info.allocationFailures = memoryStats.allocationFailures;
        
        return info;
    }
    
    void printMemoryInfo() {
        MemoryInfo info = getMemoryInfo();
        LOG_INFO_MSG("DEBUG", "=== Memory Information ===");
        LOG_INFO_F("DEBUG", "Total Heap: %u bytes", info.totalHeap);
        LOG_INFO_F("DEBUG", "Used Heap: %u bytes (%.1f%%)", 
                   info.usedHeap, (float)info.usedHeap * 100.0f / info.totalHeap);
        LOG_INFO_F("DEBUG", "Free Heap: %u bytes", info.freeHeap);
        LOG_INFO_F("DEBUG", "Max Block: %u bytes", info.maxBlockSize);
        LOG_INFO_F("DEBUG", "Allocations: %u, Deallocations: %u, Failures: %u",
                   info.allocations, info.deallocations, info.allocationFailures);
    }
    
    void trackMemoryAllocation(size_t size) {
        memoryStats.allocations++;
        // In a full implementation, we could track allocation sizes
    }
    
    void trackMemoryDeallocation(size_t size) {
        memoryStats.deallocations++;
    }
    
    bool checkMemoryLeaks() {
        return memoryStats.allocations != memoryStats.deallocations;
    }
    
    PerformanceMetrics getPerformanceMetrics() {
        PerformanceMetrics metrics = {};
        
        // Update memory information
        MemoryInfo memInfo = getMemoryInfo();
        metrics.freeHeapBytes = memInfo.freeHeap;
        metrics.maxAllocatedHeap = memInfo.usedHeap;
        
        // Estimate stack usage (simplified)
        metrics.stackUsageBytes = 1024; // Placeholder
        
        // CPU usage estimation (very basic)
        static unsigned long lastUpdate = 0;
        static unsigned long lastIdleTime = 0;
        unsigned long now = millis();
        
        if (lastUpdate > 0) {
            unsigned long elapsed = now - lastUpdate;
            metrics.cpuUsagePercent = 100.0f - ((float)(now - lastIdleTime) * 100.0f / elapsed);
            if (metrics.cpuUsagePercent < 0) metrics.cpuUsagePercent = 0;
            if (metrics.cpuUsagePercent > 100) metrics.cpuUsagePercent = 100;
        }
        
        lastUpdate = now;
        lastIdleTime = now; // This would be updated during idle periods
        
        // System uptime
        metrics.systemUptime = millis();
        
        // Task timing (would be updated by task scheduler)
        metrics.longestTaskTime = perfMetrics.longestTaskTime;
        metrics.averageTaskTime = perfMetrics.averageTaskTime;
        metrics.taskOverruns = perfMetrics.taskOverruns;
        
        return metrics;
    }
    
    void updatePerformanceMetrics() {
        perfMetrics = getPerformanceMetrics();
    }
    
    void printPerformanceMetrics() {
        PerformanceMetrics metrics = getPerformanceMetrics();
        LOG_INFO_MSG("DEBUG", "=== Performance Metrics ===");
        LOG_INFO_F("DEBUG", "Free Heap: %lu bytes", metrics.freeHeapBytes);
        LOG_INFO_F("DEBUG", "CPU Usage: %.1f%%", metrics.cpuUsagePercent);
        LOG_INFO_F("DEBUG", "Stack Usage: %lu bytes", metrics.stackUsageBytes);
        LOG_INFO_F("DEBUG", "Longest Task: %lu ms", metrics.longestTaskTime);
        LOG_INFO_F("DEBUG", "Average Task: %lu ms", metrics.averageTaskTime);
        LOG_INFO_F("DEBUG", "Task Overruns: %u", metrics.taskOverruns);
        LOG_INFO_F("DEBUG", "Uptime: %lu ms", metrics.systemUptime);
    }
    
    SystemSnapshot takeSystemSnapshot() {
        SystemSnapshot snapshot = {};
        snapshot.timestamp = millis();
        snapshot.performance = getPerformanceMetrics();
        snapshot.memory = getMemoryInfo();
        
        // Get error information from global error handler
        if (globalErrorHandler) {
            snapshot.errorCount = globalErrorHandler->getErrorCount();
            snapshot.highestSeverity = globalErrorHandler->getHighestSeverity();
            snapshot.systemHealthy = !globalErrorHandler->hasCriticalErrors();
            
            const ErrorInfo* lastError = globalErrorHandler->getLatestError();
            if (lastError) {
                snprintf(snapshot.lastError, sizeof(snapshot.lastError), 
                        "%s: %s", lastError->component, lastError->message);
            } else {
                strcpy(snapshot.lastError, "No errors");
            }
        } else {
            snapshot.errorCount = 0;
            snapshot.highestSeverity = ErrorSeverity::INFO;
            snapshot.systemHealthy = true;
            strcpy(snapshot.lastError, "Error handler not initialized");
        }
        
        // System status
        if (snapshot.systemHealthy && snapshot.memory.freeHeap > 1000 && 
            snapshot.performance.cpuUsagePercent < 90.0f) {
            strcpy(snapshot.systemStatus, "Healthy");
        } else if (snapshot.errorCount == 0 && snapshot.memory.freeHeap > 500) {
            strcpy(snapshot.systemStatus, "Warning");
        } else {
            strcpy(snapshot.systemStatus, "Critical");
        }
        
        return snapshot;
    }
    
    void printSystemSnapshot(const SystemSnapshot& snapshot) {
        LOG_INFO_MSG("DEBUG", "=== System Snapshot ===");
        LOG_INFO_F("DEBUG", "Timestamp: %lu ms", snapshot.timestamp);
        LOG_INFO_F("DEBUG", "Status: %s", snapshot.systemStatus);
        LOG_INFO_F("DEBUG", "Healthy: %s", snapshot.systemHealthy ? "Yes" : "No");
        LOG_INFO_F("DEBUG", "Error Count: %u", snapshot.errorCount);
        LOG_INFO_F("DEBUG", "Highest Severity: %d", static_cast<int>(snapshot.highestSeverity));
        LOG_INFO_F("DEBUG", "Last Error: %s", snapshot.lastError);
        LOG_INFO_F("DEBUG", "Free Memory: %u bytes", snapshot.memory.freeHeap);
        LOG_INFO_F("DEBUG", "CPU Usage: %.1f%%", snapshot.performance.cpuUsagePercent);
    }
    
    void saveSystemSnapshot(const SystemSnapshot& snapshot) {
        // In a full implementation, this would save to persistent storage
        LOG_INFO_F("DEBUG", "System snapshot saved at %lu ms", snapshot.timestamp);
    }
    
    CallStack* getCurrentCallStack() {
        return callStackEnabled ? &globalCallStack : nullptr;
    }
    
    void enableCallStackTracking(bool enable) {
        callStackEnabled = enable;
        if (!enable) {
            globalCallStack.clear();
        }
        LOG_INFO_F("DEBUG", "Call stack tracking %s", enable ? "enabled" : "disabled");
    }
    
    void debugPrint(DebugLevel level, const char* component, const char* format, ...) {
        if (level > currentDebugLevel) return;
        
        char message[256];
        va_list args;
        va_start(args, format);
        vsnprintf(message, sizeof(message), format, args);
        va_end(args);
        
        const char* levelNames[] = {"NONE", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"};
        LOG_INFO_F("DEBUG", "[%s][%s] %s", levelNames[static_cast<int>(level)], component, message);
    }
    
    void debugPrintHex(DebugLevel level, const char* component, const void* data, size_t length) {
        if (level > currentDebugLevel) return;
        
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        char hexStr[256];
        char* ptr = hexStr;
        size_t remaining = sizeof(hexStr);
        
        for (size_t i = 0; i < length && remaining > 3; i++) {
            int written = snprintf(ptr, remaining, "%02X ", bytes[i]);
            if (written > 0) {
                ptr += written;
                remaining -= written;
            }
        }
        
        debugPrint(level, component, "Hex dump (%u bytes): %s", length, hexStr);
    }
    
    void debugPrintBuffer(DebugLevel level, const char* component, const char* name, 
                         const void* buffer, size_t length) {
        if (level > currentDebugLevel) return;
        
        debugPrint(level, component, "Buffer '%s' (%u bytes):", name, length);
        debugPrintHex(level, component, buffer, length);
    }
    
    void pushDebugContext(const DebugContext& context) {
        if (contextStackDepth < sizeof(contextStack) / sizeof(contextStack[0])) {
            // In a full implementation, we'd allocate memory for the context
            // For simplicity, we'll just track the depth
            contextStackDepth++;
            debugPrint(DebugLevel::TRACE, context.component, 
                      "Entering context: %s (depth: %d)", context.operation, contextStackDepth);
        }
    }
    
    void popDebugContext() {
        if (contextStackDepth > 0) {
            contextStackDepth--;
            debugPrint(DebugLevel::TRACE, "DEBUG", 
                      "Exiting context (depth: %d)", contextStackDepth);
        }
    }
    
    const DebugContext* getCurrentContext() {
        if (contextStackDepth > 0 && contextStackDepth <= sizeof(contextStack) / sizeof(contextStack[0])) {
            return contextStack[contextStackDepth - 1];
        }
        return nullptr;
    }
    
    void printContextStack() {
        LOG_INFO_F("DEBUG", "Context stack depth: %d", contextStackDepth);
        // In a full implementation, we'd print the actual contexts
    }
    
    void assertionFailed(const char* expression, const char* file, int line, const char* function) {
        LOG_EMERG_F("DEBUG", "ASSERTION FAILED: %s at %s:%d in %s", expression, file, line, function);
        
        // Print call stack if available
        if (callStackEnabled) {
            globalCallStack.printStackTrace();
        }
        
        // Take system snapshot
        SystemSnapshot snapshot = takeSystemSnapshot();
        printSystemSnapshot(snapshot);
        
        // In a full implementation, this might trigger a safe shutdown
        // For now, we'll just halt
        while (true) {
            delay(1000);  // Wait for watchdog reset
        }
    }
    
    bool performSystemHealthCheck() {
        bool healthy = true;
        
        // Check memory
        MemoryInfo memInfo = getMemoryInfo();
        if (memInfo.freeHeap < 1000) {  // Less than 1KB free
            debugPrint(DebugLevel::WARN, "HEALTH", "Low memory: %u bytes free", memInfo.freeHeap);
            healthy = false;
        }
        
        // Check errors
        if (globalErrorHandler && globalErrorHandler->hasCriticalErrors()) {
            debugPrint(DebugLevel::WARN, "HEALTH", "Critical errors present");
            healthy = false;
        }
        
        // Check performance
        PerformanceMetrics perf = getPerformanceMetrics();
        if (perf.cpuUsagePercent > 95.0f) {
            debugPrint(DebugLevel::WARN, "HEALTH", "High CPU usage: %.1f%%", perf.cpuUsagePercent);
            healthy = false;
        }
        
        return healthy;
    }
    
    void printSystemHealth() {
        bool healthy = performSystemHealthCheck();
        LOG_INFO_F("DEBUG", "System Health: %s", healthy ? "HEALTHY" : "UNHEALTHY");
        
        if (!healthy) {
            printMemoryInfo();
            printPerformanceMetrics();
            
            if (globalErrorHandler) {
                globalErrorHandler->printStatistics();
            }
        }
    }
    
    #ifdef DEBUG_BUILD
    void injectError(ErrorType type, const char* component, const char* message) {
        if (!errorInjectionEnabled) return;
        
        debugPrint(DebugLevel::WARN, "DEBUG", "Injecting error: %s in %s", message, component);
        
        if (globalErrorHandler) {
            globalErrorHandler->reportError(type, ErrorSeverity::ERROR, component, 
                                          message, "DEBUG: Injected for testing");
        }
    }
    
    void enableErrorInjection(bool enable) {
        errorInjectionEnabled = enable;
        debugPrint(DebugLevel::INFO, "DEBUG", "Error injection %s", enable ? "enabled" : "disabled");
    }
    
    bool shouldInjectError(const char* component) {
        if (!errorInjectionEnabled) return false;
        
        // Simple probability-based injection
        errorInjectionCounter++;
        return (errorInjectionCounter % 1000) == 0;  // Inject every 1000th call
    }
    #endif
    
} // namespace DebugUtils