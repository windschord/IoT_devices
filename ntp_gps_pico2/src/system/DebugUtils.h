#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include <Arduino.h>
#include "ErrorHandler.h"
#include "ErrorCategories.h"
#include "Result.h"

/**
 * @brief Debug utilities for enhanced error information and system diagnostics
 * 
 * This system provides comprehensive debugging support including:
 * - Stack trace simulation (embedded-friendly)
 * - Memory usage tracking
 * - Performance profiling
 * - System state snapshots
 * - Error context preservation
 */

namespace DebugUtils {
    
    // Debug information levels
    enum class DebugLevel {
        NONE = 0,     // No debug output
        ERROR = 1,    // Error information only
        WARN = 2,     // Warnings and errors
        INFO = 3,     // General information
        DEBUG = 4,    // Detailed debug information
        TRACE = 5     // Function trace and detailed flow
    };
    
    // System performance metrics
    struct PerformanceMetrics {
        unsigned long freeHeapBytes;      // Available heap memory
        unsigned long maxAllocatedHeap;   // Peak heap usage
        unsigned long stackUsageBytes;    // Estimated stack usage
        float cpuUsagePercent;           // CPU usage estimate
        unsigned long longestTaskTime;    // Longest task execution time
        unsigned long averageTaskTime;    // Average task execution time
        unsigned int taskOverruns;       // Number of task overruns
        unsigned long systemUptime;      // System uptime in milliseconds
    };
    
    // Call stack simulation (for embedded systems)
    struct CallFrame {
        const char* function;         // Function name
        const char* file;            // File name
        int line;                    // Line number
        unsigned long timestamp;     // When function was called
        void* context;              // Optional context data
    };
    
    class CallStack {
    private:
        static const int MAX_STACK_DEPTH = 16;
        CallFrame frames[MAX_STACK_DEPTH];
        int currentDepth;
        
    public:
        CallStack() : currentDepth(0) {}
        
        void pushFrame(const char* function, const char* file, int line, void* context = nullptr);
        void popFrame();
        void clear();
        void printStackTrace() const;
        void getStackTrace(char* buffer, size_t bufferSize) const;
        int getDepth() const { return currentDepth; }
        const CallFrame* getFrame(int index) const;
    };
    
    // Debug context for error tracking
    struct DebugContext {
        const char* operation;           // Current operation
        const char* component;          // Component being debugged
        DebugLevel level;               // Debug level
        void* userData;                 // User-specific data
        unsigned long startTime;        // Operation start time
        CallStack* callStack;          // Call stack (optional)
        
        DebugContext(const char* op, const char* comp, DebugLevel lvl = DebugLevel::INFO)
            : operation(op), component(comp), level(lvl), userData(nullptr), 
              startTime(millis()), callStack(nullptr) {}
    };
    
    // Memory tracking
    struct MemoryInfo {
        size_t totalHeap;               // Total heap size
        size_t usedHeap;               // Currently used heap
        size_t freeHeap;               // Currently free heap
        size_t maxBlockSize;           // Largest free block
        size_t minFreeHeap;            // Minimum free heap since boot
        unsigned int allocations;       // Total allocations made
        unsigned int deallocations;    // Total deallocations made
        unsigned int allocationFailures; // Failed allocation attempts
    };
    
    // System state snapshot
    struct SystemSnapshot {
        unsigned long timestamp;        // When snapshot was taken
        PerformanceMetrics performance; // Performance data
        MemoryInfo memory;             // Memory information
        unsigned int errorCount;       // Current error count
        ErrorSeverity highestSeverity; // Highest unresolved error severity
        bool systemHealthy;           // Overall system health
        char lastError[128];          // Last error message
        char systemStatus[64];        // System status summary
    };
    
    // Debug utility functions
    void setDebugLevel(DebugLevel level);
    DebugLevel getDebugLevel();
    
    // Memory debugging
    MemoryInfo getMemoryInfo();
    void printMemoryInfo();
    void trackMemoryAllocation(size_t size);
    void trackMemoryDeallocation(size_t size);
    bool checkMemoryLeaks();
    
    // Performance monitoring
    PerformanceMetrics getPerformanceMetrics();
    void updatePerformanceMetrics();
    void printPerformanceMetrics();
    
    // System state
    SystemSnapshot takeSystemSnapshot();
    void printSystemSnapshot(const SystemSnapshot& snapshot);
    void saveSystemSnapshot(const SystemSnapshot& snapshot);
    
    // Call stack management
    CallStack* getCurrentCallStack();
    void enableCallStackTracking(bool enable);
    
    // Debug output helpers
    void debugPrint(DebugLevel level, const char* component, const char* format, ...);
    void debugPrintHex(DebugLevel level, const char* component, const void* data, size_t length);
    void debugPrintBuffer(DebugLevel level, const char* component, const char* name, 
                         const void* buffer, size_t length);
    
    // Error context debugging
    void pushDebugContext(const DebugContext& context);
    void popDebugContext();
    const DebugContext* getCurrentContext();
    void printContextStack();
    
    // Result debugging
    template<typename T>
    void debugResult(const Result<T, ErrorType>& result, const char* operation, const char* component) {
        if (result.isOk()) {
            debugPrint(DebugLevel::DEBUG, component, "Operation '%s' succeeded", operation);
        } else {
            debugPrint(DebugLevel::ERROR, component, "Operation '%s' failed with error: %d", 
                      operation, static_cast<int>(result.error()));
        }
    }
    
    // Timing utilities
    class ScopedTimer {
    private:
        const char* name;
        const char* component;
        unsigned long startTime;
        
    public:
        ScopedTimer(const char* timerName, const char* comp) 
            : name(timerName), component(comp), startTime(micros()) {}
        
        ~ScopedTimer() {
            unsigned long elapsed = micros() - startTime;
            debugPrint(DebugLevel::DEBUG, component, "%s took %lu microseconds", name, elapsed);
        }
    };
    
    // Assertion system (embedded-friendly)
    void assertionFailed(const char* expression, const char* file, int line, const char* function);
    
    // System health checking
    bool performSystemHealthCheck();
    void printSystemHealth();
    
    // Error injection for testing (debug builds only)
    #ifdef DEBUG_BUILD
    void injectError(ErrorType type, const char* component, const char* message);
    void enableErrorInjection(bool enable);
    bool shouldInjectError(const char* component);
    #endif
    
} // namespace DebugUtils

// Debug macros
#ifdef DEBUG_BUILD
    #define DEBUG_ASSERT(expr) \
        do { \
            if (!(expr)) { \
                DebugUtils::assertionFailed(#expr, __FILE__, __LINE__, __FUNCTION__); \
            } \
        } while(0)
        
    #define DEBUG_PRINT(level, component, ...) \
        DebugUtils::debugPrint(level, component, __VA_ARGS__)
        
    #define DEBUG_RESULT(result, operation, component) \
        DebugUtils::debugResult(result, operation, component)
        
    #define SCOPED_TIMER(name, component) \
        DebugUtils::ScopedTimer __timer(name, component)
        
    #define PUSH_DEBUG_CONTEXT(op, comp) \
        DebugUtils::pushDebugContext(DebugUtils::DebugContext(op, comp))
        
    #define POP_DEBUG_CONTEXT() \
        DebugUtils::popDebugContext()
        
    #define CALL_STACK_PUSH(func, file, line) \
        do { \
            auto* stack = DebugUtils::getCurrentCallStack(); \
            if (stack) stack->pushFrame(func, file, line); \
        } while(0)
        
    #define CALL_STACK_POP() \
        do { \
            auto* stack = DebugUtils::getCurrentCallStack(); \
            if (stack) stack->popFrame(); \
        } while(0)
        
#else
    #define DEBUG_ASSERT(expr) ((void)0)
    #define DEBUG_PRINT(level, component, ...) ((void)0)
    #define DEBUG_RESULT(result, operation, component) ((void)0)
    #define SCOPED_TIMER(name, component) ((void)0)
    #define PUSH_DEBUG_CONTEXT(op, comp) ((void)0)
    #define POP_DEBUG_CONTEXT() ((void)0)
    #define CALL_STACK_PUSH(func, file, line) ((void)0)
    #define CALL_STACK_POP() ((void)0)
#endif

// Function entry/exit tracing
#define TRACE_FUNCTION() \
    CALL_STACK_PUSH(__FUNCTION__, __FILE__, __LINE__); \
    struct __FunctionTracer { \
        __FunctionTracer() {} \
        ~__FunctionTracer() { CALL_STACK_POP(); } \
    } __tracer

#endif // DEBUG_UTILS_H