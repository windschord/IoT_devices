#include "ServiceContainer.h"
#include <string.h>

ServiceContainer& ServiceContainer::getInstance() {
    static ServiceContainer instance;
    return instance;
}

ServiceContainer::ServiceContainer() {
    serviceCount = 0;
    hardwareCount = 0;
    
    // 初期化
    for (int i = 0; i < MAX_SERVICES; i++) {
        clearServiceEntry(&services[i]);
    }
    
    for (int i = 0; i < MAX_HARDWARE; i++) {
        clearHardwareEntry(&hardware[i]);
    }
}

ServiceContainer::~ServiceContainer() {
    // 登録順序の逆順で停止・削除
    stopAll();
    
    // サービスインスタンスは削除しない（グローバルインスタンス）
    serviceCount = 0;
    hardwareCount = 0;
}

bool ServiceContainer::registerService(const char* name, ServiceFactory factory) {
    if (!name || !factory || serviceCount >= MAX_SERVICES) {
        return false;
    }
    
    // 重複登録チェック
    if (findService(name) >= 0) {
        Serial.printf("Warning: Service '%s' is already registered\n", name);
        return false;
    }
    
    ServiceEntry* entry = &services[serviceCount];
    clearServiceEntry(entry);
    
    if (!copyString(entry->name, name, MAX_NAME_LENGTH)) {
        return false;
    }
    
    entry->factory = factory;
    serviceCount++;
    
    return true;
}

IService* ServiceContainer::getService(const char* name) {
    if (!name) return nullptr;
    
    int index = findService(name);
    if (index < 0) return nullptr;
    
    return createServiceIfNeeded(index);
}

bool ServiceContainer::registerHardware(const char* name, HardwareFactory factory) {
    if (!name || !factory || hardwareCount >= MAX_HARDWARE) {
        return false;
    }
    
    // 重複登録チェック
    if (findHardware(name) >= 0) {
        Serial.printf("Warning: Hardware '%s' is already registered\n", name);
        return false;
    }
    
    HardwareEntry* entry = &hardware[hardwareCount];
    clearHardwareEntry(entry);
    
    if (!copyString(entry->name, name, MAX_NAME_LENGTH)) {
        return false;
    }
    
    entry->factory = factory;
    hardwareCount++;
    
    return true;
}

IHardwareInterface* ServiceContainer::getHardware(const char* name) {
    if (!name) return nullptr;
    
    int index = findHardware(name);
    if (index < 0) return nullptr;
    
    return createHardwareIfNeeded(index);
}

bool ServiceContainer::initializeAll() {
    bool allSuccess = true;
    
    // ハードウェアを最初に初期化
    for (int i = 0; i < hardwareCount; i++) {
        IHardwareInterface* hw = createHardwareIfNeeded(i);
        if (hw && !hardware[i].initialized) {
            if (hw->initialize()) {
                hardware[i].initialized = true;
                Serial.printf("✓ Hardware '%s' initialized\n", hardware[i].name);
            } else {
                Serial.printf("✗ Hardware '%s' initialization failed: %s\n", 
                             hardware[i].name, hw->getLastError() ? hw->getLastError() : "Unknown error");
                allSuccess = false;
            }
        }
    }
    
    // サービスを初期化
    for (int i = 0; i < serviceCount; i++) {
        IService* service = createServiceIfNeeded(i);
        if (service && !services[i].initialized) {
            if (service->initialize()) {
                services[i].initialized = true;
                Serial.printf("✓ Service '%s' initialized\n", services[i].name);
            } else {
                Serial.printf("✗ Service '%s' initialization failed\n", services[i].name);
                allSuccess = false;
            }
        }
    }
    
    return allSuccess;
}

bool ServiceContainer::startAll() {
    bool allSuccess = true;
    
    for (int i = 0; i < serviceCount; i++) {
        if (services[i].instance && services[i].initialized && !services[i].started) {
            if (services[i].instance->start()) {
                services[i].started = true;
                Serial.printf("✓ Service '%s' started\n", services[i].name);
            } else {
                Serial.printf("✗ Service '%s' start failed\n", services[i].name);
                allSuccess = false;
            }
        }
    }
    
    return allSuccess;
}

void ServiceContainer::stopAll() {
    // 開始順序の逆順で停止
    for (int i = serviceCount - 1; i >= 0; i--) {
        if (services[i].instance && services[i].started) {
            services[i].instance->stop();
            services[i].started = false;
            Serial.printf("✓ Service '%s' stopped\n", services[i].name);
        }
    }
}

void ServiceContainer::clear() {
    stopAll();
    
    // インスタンスは削除しない（グローバルインスタンス）
    serviceCount = 0;
    hardwareCount = 0;
}

void ServiceContainer::listServices() const {
    Serial.println("=== Registered Services ===");
    for (int i = 0; i < serviceCount; i++) {
        const ServiceEntry& entry = services[i];
        Serial.printf("- %s: %s%s%s\n", 
                     entry.name,
                     entry.instance ? "Created" : "Not created",
                     entry.initialized ? ", Initialized" : "",
                     entry.started ? ", Started" : "");
    }
}

void ServiceContainer::listHardware() const {
    Serial.println("=== Registered Hardware ===");
    for (int i = 0; i < hardwareCount; i++) {
        const HardwareEntry& entry = hardware[i];
        Serial.printf("- %s: %s%s\n", 
                     entry.name,
                     entry.instance ? "Created" : "Not created",
                     entry.initialized ? ", Initialized" : "");
    }
}

// プライベートメソッド

int ServiceContainer::findService(const char* name) {
    for (int i = 0; i < serviceCount; i++) {
        if (strcmp(services[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int ServiceContainer::findHardware(const char* name) {
    for (int i = 0; i < hardwareCount; i++) {
        if (strcmp(hardware[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

IService* ServiceContainer::createServiceIfNeeded(int index) {
    if (index < 0 || index >= serviceCount) return nullptr;
    
    ServiceEntry& entry = services[index];
    if (!entry.instance && entry.factory) {
        entry.instance = entry.factory();
    }
    
    return entry.instance;
}

IHardwareInterface* ServiceContainer::createHardwareIfNeeded(int index) {
    if (index < 0 || index >= hardwareCount) return nullptr;
    
    HardwareEntry& entry = hardware[index];
    if (!entry.instance && entry.factory) {
        entry.instance = entry.factory();
    }
    
    return entry.instance;
}

void ServiceContainer::clearServiceEntry(ServiceEntry* entry) {
    memset(entry->name, 0, MAX_NAME_LENGTH);
    entry->factory = nullptr;
    entry->instance = nullptr;
    entry->initialized = false;
    entry->started = false;
}

void ServiceContainer::clearHardwareEntry(HardwareEntry* entry) {
    memset(entry->name, 0, MAX_NAME_LENGTH);
    entry->factory = nullptr;
    entry->instance = nullptr;
    entry->initialized = false;
}

bool ServiceContainer::copyString(char* dest, const char* src, size_t maxLen) {
    if (!dest || !src || maxLen == 0) return false;
    
    size_t len = strlen(src);
    if (len >= maxLen) return false;
    
    strcpy(dest, src);
    return true;
}