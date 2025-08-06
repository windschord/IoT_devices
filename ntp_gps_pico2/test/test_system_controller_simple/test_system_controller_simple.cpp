#include <unity.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// Mock LoggingService for testing
class MockLoggingService {
public:
    char last_component[32] = {0};
    char last_message[256] = {0};
    int call_count = 0;
    
    void logInfo(const char* component, const char* message) {
        if (component) strncpy(last_component, component, sizeof(last_component) - 1);
        if (message) strncpy(last_message, message, sizeof(last_message) - 1);
        call_count++;
    }
    
    void reset() {
        memset(last_component, 0, sizeof(last_component));
        memset(last_message, 0, sizeof(last_message));
        call_count = 0;
    }
};

// Simple SystemController implementation for testing
enum SystemState {
    SYSTEM_INITIALIZING = 0,
    SYSTEM_STARTUP = 1,
    SYSTEM_RUNNING = 2,
    SYSTEM_DEGRADED = 3,
    SYSTEM_ERROR = 4,
    SYSTEM_RECOVERY = 5,
    SYSTEM_SHUTDOWN = 6
};

enum ServiceType {
    SERVICE_GPS = 0,
    SERVICE_NETWORK = 1,
    SERVICE_NTP = 2,
    SERVICE_DISPLAY = 3,
    SERVICE_CONFIG = 4,
    SERVICE_LOGGING = 5,
    SERVICE_METRICS = 6,
    SERVICE_HARDWARE = 7,
    SERVICE_COUNT = 8
};

struct ServiceHealth {
    bool healthy;
    float health_score;
    char status_message[64];
};

class SystemController {
private:
    SystemState current_state;
    ServiceHealth services[SERVICE_COUNT];
    MockLoggingService* logger;
    
public:
    SystemController(MockLoggingService* log_service) 
        : current_state(SYSTEM_INITIALIZING), logger(log_service) {
        // Initialize all services as unhealthy
        for (int i = 0; i < SERVICE_COUNT; i++) {
            services[i].healthy = false;
            services[i].health_score = 0.0f;
            strcpy(services[i].status_message, "Not initialized");
        }
    }
    
    void updateSystemState() {
        int healthy_services = 0;
        float total_health = 0.0f;
        
        // Calculate overall system health
        for (int i = 0; i < SERVICE_COUNT; i++) {
            if (services[i].healthy) {
                healthy_services++;
                total_health += services[i].health_score;
            }
        }
        
        float average_health = (healthy_services > 0) ? (total_health / healthy_services) : 0.0f;
        
        // Determine system state based on health
        SystemState new_state;
        if (healthy_services == SERVICE_COUNT && average_health >= 90.0f) {
            new_state = SYSTEM_RUNNING;
        } else if (healthy_services >= SERVICE_COUNT / 2 && average_health >= 50.0f) {
            new_state = SYSTEM_DEGRADED;
        } else if (healthy_services > 0) {
            new_state = SYSTEM_RECOVERY;
        } else {
            new_state = SYSTEM_ERROR;
        }
        
        if (new_state != current_state) {
            current_state = new_state;
            if (logger) {
                char msg[128];
                snprintf(msg, sizeof(msg), "System state changed to %s (Health: %.1f%%, Services: %d/%d)",
                        getStateName(current_state), average_health, healthy_services, SERVICE_COUNT);
                logger->logInfo("SYSTEM_CONTROLLER", msg);
            }
        }
    }
    
    void setServiceHealth(ServiceType service, bool healthy, float health_score, const char* message) {
        if (service >= 0 && service < SERVICE_COUNT) {
            services[service].healthy = healthy;
            services[service].health_score = health_score;
            if (message) {
                strncpy(services[service].status_message, message, 
                       sizeof(services[service].status_message) - 1);
                services[service].status_message[sizeof(services[service].status_message) - 1] = '\0';
            }
        }
    }
    
    SystemState getSystemState() const {
        return current_state;
    }
    
    float getSystemHealthScore() const {
        int healthy_services = 0;
        float total_health = 0.0f;
        
        for (int i = 0; i < SERVICE_COUNT; i++) {
            if (services[i].healthy) {
                healthy_services++;
                total_health += services[i].health_score;
            }
        }
        
        return (healthy_services > 0) ? (total_health / healthy_services) : 0.0f;
    }
    
    bool isServiceHealthy(ServiceType service) const {
        if (service >= 0 && service < SERVICE_COUNT) {
            return services[service].healthy;
        }
        return false;
    }
    
    const char* getStateName(SystemState state) const {
        switch (state) {
            case SYSTEM_INITIALIZING: return "INITIALIZING";
            case SYSTEM_STARTUP: return "STARTUP";
            case SYSTEM_RUNNING: return "RUNNING";
            case SYSTEM_DEGRADED: return "DEGRADED";
            case SYSTEM_ERROR: return "ERROR";
            case SYSTEM_RECOVERY: return "RECOVERY";
            case SYSTEM_SHUTDOWN: return "SHUTDOWN";
            default: return "UNKNOWN";
        }
    }
};

MockLoggingService mockLogger;

/**
 * @brief Test basic system controller initialization
 */
void test_system_controller_initialization() {
    mockLogger.reset();
    
    SystemController controller(&mockLogger);
    
    // Should start in INITIALIZING state
    TEST_ASSERT_EQUAL(SYSTEM_INITIALIZING, controller.getSystemState());
    
    // System health should be 0 initially
    TEST_ASSERT_EQUAL_FLOAT(0.0f, controller.getSystemHealthScore());
    
    // All services should be unhealthy initially
    for (int i = 0; i < SERVICE_COUNT; i++) {
        TEST_ASSERT_FALSE(controller.isServiceHealthy((ServiceType)i));
    }
}

/**
 * @brief Test system state transitions
 */
void test_system_controller_state_transitions() {
    mockLogger.reset();
    
    SystemController controller(&mockLogger);
    
    // Set some services as healthy
    controller.setServiceHealth(SERVICE_GPS, true, 95.0f, "GPS operational");
    controller.setServiceHealth(SERVICE_NETWORK, true, 90.0f, "Network connected");
    controller.setServiceHealth(SERVICE_NTP, true, 88.0f, "NTP synchronized");
    controller.setServiceHealth(SERVICE_CONFIG, true, 100.0f, "Config loaded");
    
    // Update system state
    controller.updateSystemState();
    
    // Should transition to DEGRADED (not all services healthy)
    TEST_ASSERT_EQUAL(SYSTEM_DEGRADED, controller.getSystemState());
    TEST_ASSERT_GREATER_THAN(50.0f, controller.getSystemHealthScore());
    
    // Set all services as healthy
    controller.setServiceHealth(SERVICE_DISPLAY, true, 92.0f, "Display active");
    controller.setServiceHealth(SERVICE_LOGGING, true, 94.0f, "Logging active");
    controller.setServiceHealth(SERVICE_METRICS, true, 96.0f, "Metrics active");
    controller.setServiceHealth(SERVICE_HARDWARE, true, 98.0f, "Hardware OK");
    
    controller.updateSystemState();
    
    // Should transition to RUNNING (all services healthy with high scores)
    TEST_ASSERT_EQUAL(SYSTEM_RUNNING, controller.getSystemState());
    TEST_ASSERT_GREATER_OR_EQUAL(90.0f, controller.getSystemHealthScore());
    
    // Should log state change
    TEST_ASSERT_GREATER_THAN(0, mockLogger.call_count);
    TEST_ASSERT_EQUAL_STRING("SYSTEM_CONTROLLER", mockLogger.last_component);
    TEST_ASSERT_TRUE(strstr(mockLogger.last_message, "RUNNING") != NULL);
}

/**
 * @brief Test service health management
 */
void test_system_controller_service_health() {
    mockLogger.reset();
    
    SystemController controller(&mockLogger);
    
    // Test individual service health setting
    controller.setServiceHealth(SERVICE_GPS, true, 85.5f, "GPS signal strong");
    TEST_ASSERT_TRUE(controller.isServiceHealthy(SERVICE_GPS));
    
    controller.setServiceHealth(SERVICE_NETWORK, false, 0.0f, "Network disconnected");
    TEST_ASSERT_FALSE(controller.isServiceHealthy(SERVICE_NETWORK));
    
    // Test health score calculation
    controller.setServiceHealth(SERVICE_NTP, true, 70.0f, "NTP syncing");
    controller.updateSystemState();
    
    // Average of 85.5 and 70.0 = 77.75
    float expected_health = (85.5f + 70.0f) / 2.0f;
    TEST_ASSERT_FLOAT_WITHIN(1.0f, expected_health, controller.getSystemHealthScore());
}

/**
 * @brief Test error state handling
 */
void test_system_controller_error_state() {
    mockLogger.reset();
    
    SystemController controller(&mockLogger);
    
    // Set all services as unhealthy
    for (int i = 0; i < SERVICE_COUNT; i++) {
        controller.setServiceHealth((ServiceType)i, false, 0.0f, "Service failed");
    }
    
    controller.updateSystemState();
    
    // Should be in ERROR state
    TEST_ASSERT_EQUAL(SYSTEM_ERROR, controller.getSystemState());
    TEST_ASSERT_EQUAL_FLOAT(0.0f, controller.getSystemHealthScore());
    
    // Should log state change
    TEST_ASSERT_GREATER_THAN(0, mockLogger.call_count);
    TEST_ASSERT_TRUE(strstr(mockLogger.last_message, "ERROR") != NULL);
}

void setUp(void) {
    mockLogger.reset();
}

void tearDown(void) {
    // Cleanup
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_system_controller_initialization);
    RUN_TEST(test_system_controller_state_transitions);
    RUN_TEST(test_system_controller_service_health);
    RUN_TEST(test_system_controller_error_state);
    
    return UNITY_END();
}