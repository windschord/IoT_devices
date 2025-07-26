// ArduinoJson.h mock for native testing
#include "arduino_mock.h"
#include <map>
#include <string>

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
};

// Serialization functions
template<typename T>
size_t serializeJson(const T&, String&) { return 0; }

template<typename T>
size_t deserializeJson(T&, const String&) { return 0; }