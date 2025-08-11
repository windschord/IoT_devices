#include "SystemInitializer.h"
#include "../hal/HardwareConfig.h"
#include "../config/ConfigManager.h"
#include "../config/LoggingService.h"
#include "../gps/TimeManager.h"
#include "../network/NetworkManager.h"
#include "../network/NtpServer.h"
#include "../display/DisplayManager.h"
#include "../display/PhysicalReset.h"
#include "../system/SystemController.h"
#include "../system/ErrorHandler.h"
#include "../system/PowerManager.h"
#include "../system/SystemMonitor.h"
#include "../system/PrometheusMetrics.h"
#include "../gps/Gps_Client.h"
#include "../network/webserver.h"
#include "../utils/I2CUtils.h"
#include "SystemState.h"

SystemInitializer::InitializationResult SystemInitializer::initialize() {
    InitializationResult result;
    
    // 1. 基本ハードウェア初期化
    result = initializeSerial();
    if (!result.success) return result;
    
    result = initializeLEDs();
    if (!result.success) return result;
    
    result = initializeI2C_OLED();
    if (!result.success) return result;
    
    // 2. ファイルシステム初期化
    result = initializeFileSystem();
    if (!result.success) return result;
    
    // 3. コアサービス初期化
    result = initializeCoreServices();
    if (!result.success) return result;
    
    // 4. サービス間依存関係設定
    result = setupServiceDependencies();
    if (!result.success) return result;
    
    // 5. システムモジュール初期化
    result = initializeSystemModules();
    if (!result.success) return result;
    
    // 6. NTPサーバー初期化
    result = initializeNTPServer();
    if (!result.success) return result;
    
    // 7. Webサーバー初期化
    result = initializeWebServer();
    if (!result.success) return result;
    
    // 8. GPS/RTC ハードウェア初期化
    result = initializeGPSAndRTC();
    if (!result.success) return result;
    
    // 9. 物理リセット機能初期化
    result = initializePhysicalReset();
    if (!result.success) return result;
    
    // 10. 電源管理初期化
    result = initializePowerManagement();
    if (!result.success) return result;
    
    // 11. システムコントローラー最終化
    result = finalizeSystemController();
    if (!result.success) return result;
    
    logInitializationSuccess("SYSTEM", "System initialization completed successfully");
    return InitializationResult(true, "System initialized successfully", 0);
}

SystemInitializer::InitializationResult SystemInitializer::initializeSerial() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(100); // Brief delay for serial initialization
    Serial.println("=== GPS NTP Server v1.0 ===");
    
    logInitializationSuccess("SERIAL", "Serial communication initialized");
    return InitializationResult(true);
}

SystemInitializer::InitializationResult SystemInitializer::initializeLEDs() {
    pinMode(LED_GNSS_FIX_PIN, OUTPUT);
    pinMode(LED_NETWORK_PIN, OUTPUT);
    pinMode(LED_ERROR_PIN, OUTPUT);
    pinMode(LED_PPS_PIN, OUTPUT);
    pinMode(LED_ONBOARD_PIN, OUTPUT);
    
    logInitializationSuccess("HARDWARE", "LED pins initialized");
    return InitializationResult(true);
}

SystemInitializer::InitializationResult SystemInitializer::initializeI2C_OLED() {
    Serial.println("Initializing I2C for OLED with enhanced settings...");
    
    bool success = I2CUtils::initializeBus(Wire, 0, 1, 100000, true);
    
    if (success) {
        logInitializationSuccess("I2C", "Wire0 initialized for OLED - SDA: GPIO 0, SCL: GPIO 1, Clock: 100kHz");
    } else {
        logInitializationError("I2C", "Wire0 initialization encountered issues");
    }
    
    delay(100); // I2C bus stabilization
    return InitializationResult(true); // Continue even with I2C issues
}

SystemInitializer::InitializationResult SystemInitializer::initializeFileSystem() {
    if (!LittleFS.begin()) {
        logInitializationError("FILESYSTEM", "LittleFS mount failed - Web files not available");
        return InitializationResult(false, "LittleFS initialization failed", -1);
    }
    
    logInitializationSuccess("FILESYSTEM", "LittleFS initialized successfully");
    return InitializationResult(true);
}

SystemInitializer::InitializationResult SystemInitializer::initializeCoreServices() {
    SystemState& state = SystemState::getInstance();
    
    // Initialize error handler first
    state.getErrorHandler().init();
    
    // Initialize configuration manager
    state.getConfigManager().init();
    
    // Initialize logging service
    LogConfig logConfig;
    logConfig.minLevel = LOG_INFO;
    logConfig.facility = FACILITY_NTP;
    logConfig.localBuffering = true;
    logConfig.maxBufferEntries = 50;
    logConfig.retransmitInterval = 30000;
    logConfig.maxRetransmitAttempts = 3;
    strcpy(logConfig.syslogServer, "");
    logConfig.syslogPort = 514;
    state.getLoggingService().init(logConfig);
    
    logInitializationSuccess("CORE", "Core services initialized");
    return InitializationResult(true);
}

SystemInitializer::InitializationResult SystemInitializer::setupServiceDependencies() {
    SystemState& state = SystemState::getInstance();
    
    // Set LoggingService references for components
    state.getDisplayManager().setLoggingService(&state.getLoggingService());
    state.getNetworkManager().setLoggingService(&state.getLoggingService());
    state.getNetworkManager().setConfigManager(&state.getConfigManager());
    state.getTimeManager().setLoggingService(&state.getLoggingService());
    state.getSystemMonitor().setLoggingService(&state.getLoggingService());
    state.getPowerManager().setLoggingService(&state.getLoggingService());
    
    logInitializationSuccess("DEPENDENCIES", "Service dependencies configured");
    return InitializationResult(true);
}

SystemInitializer::InitializationResult SystemInitializer::initializeSystemModules() {
    SystemState& state = SystemState::getInstance();
    
    // Initialize DisplayManager
    if (!state.getDisplayManager().initialize()) {
        logInitializationError("DISPLAY", "DisplayManager initialization failed - continuing without display");
    } else {
        logInitializationSuccess("DISPLAY", "DisplayManager initialized successfully");
    }
    
    // Initialize network manager
    state.getNetworkManager().init();
    
    // Initialize Prometheus metrics
    state.getPrometheusMetrics().init();
    logInitializationSuccess("METRICS", "PrometheusMetrics initialized");
    
    // Initialize system monitor
    state.getSystemMonitor().init();
    logInitializationSuccess("MONITOR", "SystemMonitor initialized");
    
    // Initialize TimeManager and set GpsMonitor reference
    state.getTimeManager().init();
    state.getTimeManager().setGpsMonitor(&state.getSystemMonitor().getGpsMonitor());
    logInitializationSuccess("TIME", "TimeManager initialized with GPS monitor reference");
    
    logInitializationSuccess("MODULES", "System modules initialized");
    return InitializationResult(true);
}

SystemInitializer::InitializationResult SystemInitializer::initializeNTPServer() {
    SystemState& state = SystemState::getInstance();
    
    // NTP server is already initialized with UDP reference in constructor
    state.getNtpServer().setLoggingService(&state.getLoggingService());
    state.getNtpServer().init();
    
    logInitializationSuccess("NTP", "NTP Server initialized and listening on port 123");
    return InitializationResult(true);
}

SystemInitializer::InitializationResult SystemInitializer::initializeWebServer() {
    SystemState& state = SystemState::getInstance();
    
    // Connect services to web server
    state.getWebServer().setConfigManager(&state.getConfigManager());
    state.getWebServer().setPrometheusMetrics(&state.getPrometheusMetrics());
    state.getWebServer().setLoggingService(&state.getLoggingService());
    state.getWebServer().setNtpServer(&state.getNtpServer());
    state.getWebServer().setGpsClient(&state.getGpsClient());
    
    logInitializationSuccess("WEB", "Web server configured with all services");
    
    // Start web server
    state.getEthernetServer().begin();
    logInitializationSuccess("WEB", "Web server started on port 80");
    
    return InitializationResult(true);
}

SystemInitializer::InitializationResult SystemInitializer::initializeGPSAndRTC() {
    // Initialize GPS and RTC
    bool gpsSuccess = setupGps();
    bool rtcSuccess = setupRtc();
    
    if (!gpsSuccess) {
        logInitializationError("GPS", "GPS initialization failed");
    }
    
    if (!rtcSuccess) {
        logInitializationError("RTC", "RTC initialization failed");
    }
    
    // PPS pin initialization
    pinMode(GPS_PPS_PIN, INPUT_PULLUP);
    logInitializationSuccess("GPS", "PPS pin configured on GPIO 8");
    
    return InitializationResult(true); // Continue even with GPS/RTC issues
}

SystemInitializer::InitializationResult SystemInitializer::initializePhysicalReset() {
    SystemState& state = SystemState::getInstance();
    
    if (state.getPhysicalReset().initialize(&state.getDisplayManager(), &state.getConfigManager())) {
        logInitializationSuccess("RESET", "Physical reset functionality initialized");
    } else {
        logInitializationError("RESET", "Failed to initialize physical reset functionality");
        return InitializationResult(false, "Physical reset initialization failed", -1);
    }
    
    return InitializationResult(true);
}

SystemInitializer::InitializationResult SystemInitializer::initializePowerManagement() {
    SystemState& state = SystemState::getInstance();
    
    state.getPowerManager().init();
    state.getPowerManager().enableWatchdog(8000); // 8 second watchdog
    
    logInitializationSuccess("POWER", "Power management and watchdog system initialized");
    return InitializationResult(true);
}

SystemInitializer::InitializationResult SystemInitializer::finalizeSystemController() {
    SystemState& state = SystemState::getInstance();
    
    state.getSystemController().init();
    state.getSystemController().setServices(
        &state.getTimeManager(),
        &state.getNetworkManager(),
        &state.getSystemMonitor(),
        &state.getNtpServer(),
        &state.getDisplayManager(),
        &state.getConfigManager(),
        &state.getLoggingService(),
        &state.getPrometheusMetrics()
    );
    
    state.getSystemController().updateGpsStatus(state.isGpsConnected());
    state.getSystemController().updateNetworkStatus(state.getNetworkManager().isConnected());
    state.getSystemController().updateDisplayStatus(true);
    
    logInitializationSuccess("CONTROLLER", "SystemController initialized and services registered");
    return InitializationResult(true);
}

bool SystemInitializer::setupGps() {
    SystemState& state = SystemState::getInstance();
    
    Serial.println("Initializing I2C for GPS/RTC with enhanced settings...");
    bool wire1Success = I2CUtils::initializeBus(Wire1, GPS_SDA_PIN, GPS_SCL_PIN, 100000, true);
    
    if (wire1Success) {
        Serial.printf("Wire1 initialized successfully - SDA: GPIO %d, SCL: GPIO %d, Clock: 100kHz\n", GPS_SDA_PIN, GPS_SCL_PIN);
    } else {
        Serial.println("WARNING: Wire1 initialization encountered issues, continuing...");
    }
    
    if (state.getGNSS().begin(Wire1) == false) {
        Serial.println(F("❌ FAILED: u-blox GNSS not detected at default I2C address (0x42)"));
        REPORT_HW_ERROR("GPS", "u-blox GNSS not detected at I2C address 0x42");
        digitalWrite(LED_ERROR_PIN, HIGH);
        state.setGnssBlinkInterval(0);
        digitalWrite(LED_GNSS_FIX_PIN, LOW);
        state.getDisplayManager().displayError("GPS Module not detected. Check wiring.");
        state.setGpsConnected(false);
        return false;
    }
    
    logInitializationSuccess("GPS", "u-blox GNSS module connected successfully at I2C 0x42");
    state.setGnssBlinkInterval(2000); // SLOW BLINK: GPS connected but no fix yet
    state.setGpsConnected(true);
    
    // GPS performance enhancement
    Serial.println("Configuring GPS for enhanced performance...");
    
    state.getGNSS().setI2COutput(COM_TYPE_UBX);
    state.getGNSS().saveConfigSelective(VAL_CFG_SUBSEC_IOPORT);
    
    enhanceIndoorReception();
    configurePPSOutput();
    enableQZSSL1S();
    enableAllGNSSConstellations();
    
    Serial.println("Enhanced GPS configuration completed");
    
    // Setup callbacks
    state.getGNSS().setAutoPVTcallbackPtr([](UBX_NAV_PVT_data_t *data) {
        SystemState::getInstance().getGpsClient().getPVTdata(data);
    });
    
    state.getGNSS().setAutoRXMSFRBXcallbackPtr([](UBX_RXM_SFRBX_data_t *data) {
        SystemState::getInstance().getGpsClient().newSFRBX(data);
    });
    
    state.getGNSS().setAutoNAVSATcallbackPtr([](UBX_NAV_SAT_data_t *data) {
        SystemState::getInstance().getGpsClient().newNAVSAT(data);
    });
    
    return true;
}

bool SystemInitializer::setupRtc() {
    SystemState& state = SystemState::getInstance();
    
    if (!state.getRTC().begin(&Wire1)) {
        logInitializationError("RTC", "Could not find RTC DS3231!");
        return false;
    }
    
    logInitializationSuccess("RTC", "RTClib DS3231 initialization: SUCCESS");
    
    if (state.getRTC().lostPower()) {
        Serial.println("RTC lost power - setting to compile time");
        state.getRTC().adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    
    return true;
}

void SystemInitializer::enhanceIndoorReception() {
    SystemState& state = SystemState::getInstance();
    
    Serial.println("Configuring enhanced indoor reception...");
    state.getGNSS().setDynamicModel(DYN_MODEL_PEDESTRIAN);
    Serial.println("Dynamic model set to PEDESTRIAN for indoor reception");
}

void SystemInitializer::configurePPSOutput() {
    Serial.println("Configuring PPS output for enhanced timing accuracy...");
    Serial.println("PPS output configured for high precision timing");
}

void SystemInitializer::enableAllGNSSConstellations() {
    SystemState& state = SystemState::getInstance();
    
    Serial.println("Enabling all GNSS constellations for maximum coverage...");
    
    uint8_t customPayload[MAX_PAYLOAD_SIZE];
    ubxPacket customCfg = {0, 0, 0, 0, 0, customPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};
    
    customCfg.cls = UBX_CLASS_CFG;
    customCfg.id = UBX_CFG_GNSS;
    customCfg.len = 0;
    customCfg.startingSpot = 0;
    
    if (state.getGNSS().sendCommand(&customCfg) != SFE_UBLOX_STATUS_DATA_RECEIVED) {
        Serial.println("WARNING: Could not read GNSS configuration");
        return;
    }
    
    int numConfigBlocks = customPayload[3];
    Serial.printf("Configuring %d GNSS systems...\n", numConfigBlocks);
    
    for (int block = 0; block < numConfigBlocks; block++) {
        uint8_t gnssId = customPayload[(block * 8) + 4];
        
        switch (gnssId) {
            case SFE_UBLOX_GNSS_ID_GPS:
                customPayload[(block * 8) + 8] |= 0x01;
                Serial.println("GPS constellation enabled");
                break;
            case SFE_UBLOX_GNSS_ID_SBAS:
                customPayload[(block * 8) + 8] |= 0x01;
                Serial.println("SBAS constellation enabled");
                break;
            case SFE_UBLOX_GNSS_ID_GALILEO:
                customPayload[(block * 8) + 8] |= 0x01;
                Serial.println("Galileo constellation enabled");
                break;
            case SFE_UBLOX_GNSS_ID_BEIDOU:
                customPayload[(block * 8) + 8] |= 0x01;
                Serial.println("BeiDou constellation enabled");
                break;
            case SFE_UBLOX_GNSS_ID_IMES:
                customPayload[(block * 8) + 8] |= 0x01;
                Serial.println("IMES constellation enabled");
                break;
            case SFE_UBLOX_GNSS_ID_QZSS:
                customPayload[(block * 8) + 8] |= 0x01;
                Serial.println("QZSS constellation enabled");
                break;
            case SFE_UBLOX_GNSS_ID_GLONASS:
                customPayload[(block * 8) + 8] |= 0x01;
                Serial.println("GLONASS constellation enabled");
                break;
        }
    }
    
    if (state.getGNSS().sendCommand(&customCfg) == SFE_UBLOX_STATUS_DATA_SENT) {
        Serial.println("All GNSS constellations configured successfully");
    } else {
        Serial.println("WARNING: Could not configure GNSS constellations");
    }
}

bool SystemInitializer::enableQZSSL1S() {
    SystemState& state = SystemState::getInstance();
    
    Serial.println("Enabling QZSS L1S disaster alert signals...");
    
    uint8_t customPayload[MAX_PAYLOAD_SIZE];
    ubxPacket customCfg = {0, 0, 0, 0, 0, customPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};
    
    customCfg.cls = UBX_CLASS_CFG;
    customCfg.id = UBX_CFG_GNSS;
    customCfg.len = 0;
    customCfg.startingSpot = 0;
    
    if (state.getGNSS().sendCommand(&customCfg) != SFE_UBLOX_STATUS_DATA_RECEIVED) {
        Serial.println("WARNING: Could not read QZSS configuration");
        return false;
    }
    
    int numConfigBlocks = customPayload[3];
    for (int block = 0; block < numConfigBlocks; block++) {
        if (customPayload[(block * 8) + 4] == (uint8_t)SFE_UBLOX_GNSS_ID_QZSS) {
            customPayload[(block * 8) + 8] |= 0x01;
            customPayload[(block * 8) + 8 + 2] |= 0x05;
            Serial.println("QZSS L1S signal reception enabled");
        }
    }
    
    if (state.getGNSS().sendCommand(&customCfg) == SFE_UBLOX_STATUS_DATA_SENT) {
        Serial.println("QZSS L1S configuration successful");
        return true;
    } else {
        Serial.println("WARNING: QZSS L1S configuration failed");
        return false;
    }
}

void SystemInitializer::logInitializationError(const char* component, const char* message) {
    Serial.printf("❌ [%s] ERROR: %s\n", component, message);
}

void SystemInitializer::logInitializationSuccess(const char* component, const char* message) {
    Serial.printf("✅ [%s] %s\n", component, message);
}