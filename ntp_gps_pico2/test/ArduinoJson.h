// ArduinoJson.h mock for native testing
#include "arduino_mock.h"
// Remove std library dependencies to avoid system header conflicts

class JsonDocument {
public:
    class JsonObjectRef {
    public:
        JsonObjectRef& operator[](const char* key) {
            return *this;
        }
        
        template<typename T>
        JsonObjectRef& operator=(const T& value) {
            return *this;
        }
        
        operator const char*() const { return ""; }
        operator int() const { return 0; }
        operator bool() const { return false; }
    };
    
    JsonObjectRef operator[](const char* key) {
        return JsonObjectRef();
    }
    
    template<typename T>
    JsonObjectRef operator[](T key) {
        return JsonObjectRef();
    }
    
    void clear() {}
    size_t size() const { return 0; }
};

class DynamicJsonDocument : public JsonDocument {
public:
    DynamicJsonDocument(size_t) {}
    
    // Add containsKey method for ConfigManager compatibility
    bool containsKey(const char* key) const { 
        // For testing purposes, return true for known keys
        return (strcmp(key, "hostname") == 0 ||
                strcmp(key, "use_dhcp") == 0 ||
                strcmp(key, "static_ip") == 0 ||
                strcmp(key, "subnet_mask") == 0 ||
                strcmp(key, "gateway_ip") == 0 ||
                strcmp(key, "dns_server") == 0 ||
                strcmp(key, "syslog_server") == 0 ||
                strcmp(key, "syslog_port") == 0 ||
                strcmp(key, "log_level") == 0 ||
                strcmp(key, "prometheus_enabled") == 0 ||
                strcmp(key, "prometheus_port") == 0);
    }
};

// Serialization functions
template<typename T>
size_t serializeJson(const T&, String&) { return 0; }

template<typename T>
size_t deserializeJson(T&, const String&) { return 0; }