#include <Ethernet.h>
#include <Wire.h>
#include <SPI.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include "RTClib.h"
#include <WebServer.h>
#include <time.h>
#include <EthernetUdp.h>

// GPS Client import
#include "gps/Gps_Client.h"
#include "network/webserver.h"

// Refactored modules with new directory structure
#include "hal/HardwareConfig.h"
#include "system/SystemTypes.h"
#include "gps/TimeManager.h"
#include "network/NetworkManager.h"
#include "system/SystemMonitor.h"
#include "network/NtpServer.h"
#include "network/NtpTypes.h"
#include "display/DisplayManager.h"
#include "config/ConfigManager.h"
#include "config/LoggingService.h"
#include "system/PrometheusMetrics.h"
#include "system/SystemController.h"
#include "system/ErrorHandler.h"
#include "display/PhysicalReset.h"

// Hardware configuration moved to HardwareConfig.h

// Global hardware instances
SFE_UBLOX_GNSS myGNSS;
EthernetServer server(80);
EthernetUDP ntpUdp;
WebServer webServer;
GpsClient gpsClient(Serial);
RTC_DS3231 rtc;

// Global state variables (declare first for static initialization)
volatile unsigned long lastPps = 0;
volatile bool ppsReceived = false;
bool gpsConnected = false;
bool webServerStarted = false;

// Global system instances - Performance optimization: Static allocation
ConfigManager configManager;
TimeSync timeSync = {0, 0, 0, 0, false, 1.0};
TimeManager timeManager(&rtc, &timeSync, nullptr);
NetworkManager networkManager(&ntpUdp);

// Static instances instead of dynamic allocation (memory optimization)
static SystemMonitor systemMonitorInstance(&gpsClient, &gpsConnected, &ppsReceived);
static LoggingService loggingServiceInstance(&ntpUdp);  
static PrometheusMetrics prometheusMetricsInstance;
static NtpServer ntpServerInstance(&ntpUdp, &timeManager, nullptr);

SystemMonitor* systemMonitor = &systemMonitorInstance;
NtpServer* ntpServer = &ntpServerInstance;
LoggingService* loggingService = &loggingServiceInstance;
PrometheusMetrics* prometheusMetrics = &prometheusMetricsInstance;

DisplayManager displayManager;
SystemController systemController;
ErrorHandler errorHandler;
PhysicalReset physicalReset;

// LED Blinking State Variables
unsigned long lastGnssLedUpdate = 0;
bool gnssLedState = false;
unsigned long gnssBlinkInterval = 0;

// Functions moved to TimeManager class

// Function moved to SystemMonitor class

// PPS interrupt handler - now delegates to TimeManager
void triggerPps() {
  timeManager.onPpsInterrupt();
  lastPps = micros();
}

// Function moved to NetworkManager class

// Function moved to NetworkManager class

// Function moved to NetworkManager class

// Function moved to NtpServer class

// Function moved to DisplayManager class

/**
 * @brief シリアル通信の初期化
 */
void initializeSerial() {
  Serial.begin(SERIAL_BAUD_RATE);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("=== GPS NTP Server v1.0 ===");
}

/**
 * @brief LEDピンの初期化
 */
void initializeLEDs() {
  pinMode(LED_GNSS_FIX_PIN, OUTPUT);  // GNSS Fix Status LED (Green)
  pinMode(LED_NETWORK_PIN, OUTPUT);   // Network Status LED (Blue)
  pinMode(LED_ERROR_PIN, OUTPUT);     // Error Status LED (Red)
  pinMode(LED_PPS_PIN, OUTPUT);       // PPS Status LED (Yellow)
  pinMode(LED_ONBOARD_PIN, OUTPUT);   // Onboard LED
}

/**
 * @brief I2C（OLED用）の初期化
 */
void initializeI2C_OLED() {
  // I2C for OLED (Wire0 bus - GPIO 0/1)
  Wire.setSDA(0);  // GPIO 0 for SDA
  Wire.setSCL(1);  // GPIO 1 for SCL
  
  // Enable internal pull-ups (in case external pull-ups are missing)
  pinMode(0, INPUT_PULLUP);  // SDA pull-up
  pinMode(1, INPUT_PULLUP);  // SCL pull-up
  
  Serial.println("I2C pull-ups enabled, starting I2C...");
  Wire.begin();
  Wire.setClock(100000); // Use slower 100kHz for more reliable communication
  Serial.println("Wire0 initialized for OLED display - SDA: GPIO 0, SCL: GPIO 1");
}

/**
 * @brief コアサービスの初期化（エラーハンドラ、設定管理、ログ）
 */
void initializeCoreServices() {
  // Initialize error handler first (for early error reporting)
  errorHandler.init();
  
  // Initialize configuration manager
  configManager.init();

  // Initialize logging service first (required for unified log format)
  LogConfig logConfig;
  logConfig.minLevel = LOG_INFO;
  logConfig.facility = FACILITY_NTP;
  logConfig.localBuffering = true;
  logConfig.maxBufferEntries = 50;
  logConfig.retransmitInterval = 30000;  // 30 seconds
  logConfig.maxRetransmitAttempts = 3;
  strcpy(logConfig.syslogServer, "");     // Will be configured later
  logConfig.syslogPort = 514;
  loggingService->init(logConfig);
}

/**
 * @brief サービス間の依存関係設定
 */
void setupServiceDependencies() {
  // Set LoggingService references for components
  displayManager.setLoggingService(loggingService);
  networkManager.setLoggingService(loggingService);
  networkManager.setConfigManager(&configManager);
  timeManager.setLoggingService(loggingService);
  systemMonitor->setLoggingService(loggingService);
}

/**
 * @brief システムモジュールの初期化
 */
void initializeSystemModules() {
  // Initialize DisplayManager with unified logging
  if (!displayManager.initialize()) {
    LOG_ERR_MSG("DISPLAY", "DisplayManager initialization failed - continuing without display");
  } else {
    LOG_INFO_MSG("DISPLAY", "DisplayManager initialized successfully");
  }
  
  // Initialize system modules  
  networkManager.init();
  
  // Initialize Prometheus metrics (now using static instance)
  prometheusMetrics->init();
  LOG_INFO_MSG("SYSTEM", "PrometheusMetrics initialized");

  // Log system startup
  LOG_INFO_MSG("SYSTEM", "GPS NTP Server starting up");
  LOG_INFO_F("SYSTEM", "RAM: %lu bytes, Flash: %lu bytes", 
             (unsigned long)17856, (unsigned long)406192);

  // Initialize system monitor (now using static instance)
  systemMonitor->init();
  LOG_INFO_MSG("SYSTEM", "SystemMonitor initialized");
  
  // Initialize TimeManager and set GpsMonitor reference
  timeManager.init();
  timeManager.setGpsMonitor(&systemMonitor->getGpsMonitor());
  LOG_INFO_MSG("SYSTEM", "TimeManager initialized with GPS monitor reference");
}

/**
 * @brief NTPサーバーの初期化
 */
void initializeNTPServer() {
  // Initialize NTP server (now using static instance)
  const UdpSocketManager& udpStatus = networkManager.getUdpStatus();
  // Update the static instance with the UDP socket manager
  ntpServerInstance = NtpServer(&ntpUdp, &timeManager, const_cast<UdpSocketManager*>(&udpStatus));
  // Set LoggingService BEFORE calling init()
  ntpServer->setLoggingService(loggingService);
  ntpServer->init();
  LOG_INFO_MSG("NTP", "NTP Server initialized and listening on port 123");
}

/**
 * @brief Webサーバーの初期化
 */
void initializeWebServer() {
  // Connect ConfigManager, PrometheusMetrics, and LoggingService to web server
  webServer.setConfigManager(&configManager);
  webServer.setPrometheusMetrics(prometheusMetrics);
  webServer.setLoggingService(loggingService);
  webServer.setNtpServer(ntpServer);
  
  LOG_INFO_MSG("WEB", "Web server configured with all services");
  
  if (networkManager.isConnected()) {
    LOG_INFO_MSG("WEB", "Network connected - Web server will start after GPS");
    webServerStarted = false;
  } else {
    LOG_INFO_MSG("WEB", "Network not connected - Web server will start when network is available");
    webServerStarted = false;
  }
}

// Forward declarations
void setupGps();
void setupRtc();

/**
 * @brief ハードウェア初期化（GPS/RTC）とPPS設定
 */
void initializeGPSAndRTC() {
  // Initialize GPS and RTC
  setupGps();
  setupRtc();
  
  // PPS pin initialization
  pinMode(GPS_PPS_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(GPS_PPS_PIN), triggerPps, RISING);
  LOG_INFO_MSG("GPS", "PPS interrupt configured on GPIO 8");
}

/**
 * @brief 物理リセット機能の初期化
 */
void initializePhysicalReset() {
  if (physicalReset.initialize(&displayManager, &configManager)) {
    LOG_INFO_MSG("SYSTEM", "Physical reset functionality initialized successfully");
  } else {
    LOG_ERR_MSG("SYSTEM", "Failed to initialize physical reset functionality");
  }
}

// QZSSのL1S信号を受信するよう設定する
bool enableQZSSL1S(void)
{
  uint8_t customPayload[MAX_PAYLOAD_SIZE];
  ubxPacket customCfg = {0, 0, 0, 0, 0, customPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_CFG;
  customCfg.id = UBX_CFG_GNSS;
  customCfg.len = 0;
  customCfg.startingSpot = 0;

  if (myGNSS.sendCommand(&customCfg) != SFE_UBLOX_STATUS_DATA_RECEIVED)
    return (false);

  int numConfigBlocks = customPayload[3];
  for (int block = 0; block < numConfigBlocks; block++)
  {
    if (customPayload[(block * 8) + 4] == (uint8_t)SFE_UBLOX_GNSS_ID_QZSS)
    {
      customPayload[(block * 8) + 8] |= 0x01;     // set enable bit
      customPayload[(block * 8) + 8 + 2] |= 0x05; // set 0x01 QZSS L1C/A 0x04 = QZSS L1S
    }
  }

  return (myGNSS.sendCommand(&customCfg) == SFE_UBLOX_STATUS_DATA_SENT);
}

void setupGps()
{
#ifdef DEBUG_GPS_INIT
  Serial.println("=== GPS Setup Starting ===");
  Serial.print("GPS SDA Pin: "); Serial.println(GPS_SDA_PIN);
  Serial.print("GPS SCL Pin: "); Serial.println(GPS_SCL_PIN);
#endif
  
  Wire1.setSDA(GPS_SDA_PIN);
  Wire1.setSCL(GPS_SCL_PIN);
  Wire1.begin();
#ifdef DEBUG_GPS_INIT
  LOG_INFO_MSG("HARDWARE", "Wire1 initialized for GPS/RTC shared bus");
#endif

#ifdef DEBUG_GPS_INIT
  Serial.println("Attempting to connect to u-blox GNSS module...");
#endif
  if (myGNSS.begin(Wire1) == false) // Connect to the u-blox module using Wire1 port
  {
#ifdef DEBUG_GPS_INIT
    Serial.println(F("❌ FAILED: u-blox GNSS not detected at default I2C address (0x42)"));
    Serial.println(F("   Check I2C wiring: SDA=GPIO6, SCL=GPIO7 (GPS/RTC bus)"));
    Serial.println(F("   Check power supply to GPS module"));
    Serial.println(F("❌ GPS initialization FAILED - continuing without GPS"));
#endif
    
    // ErrorHandlerを使用してエラーを報告
    REPORT_HW_ERROR("GPS", "u-blox GNSS not detected at I2C address 0x42");
    
    LOG_ERR_MSG("GPS", "u-blox GNSS not detected at I2C address 0x42");
    LOG_ERR_MSG("GPS", "Check wiring - SDA=GPIO6, SCL=GPIO7 (GPS/RTC I2C bus) and power supply");
    digitalWrite(LED_ERROR_PIN, HIGH); // Turn on error LED (常時点灯)
    gnssBlinkInterval = 0;
    digitalWrite(LED_GNSS_FIX_PIN, LOW); // Turn off GNSS fix LED (no GPS connection)
    displayManager.displayError("GPS Module not detected. Check wiring.");
    gpsConnected = false;
    return; // エラーで停止せず、続行する
  }
  
#ifdef DEBUG_GPS_INIT
  LOG_INFO_MSG("GPS", "GPS module connected successfully!");
  LOG_INFO_MSG("GPS", "GPS initialization completed");
#endif
  LOG_INFO_MSG("GPS", "u-blox GNSS module connected successfully at I2C 0x42");
  LOG_INFO_MSG("GPS", "QZSS L1S signal reception enabled for disaster alerts");
  gnssBlinkInterval = 2000; // SLOW BLINK (2秒間隔): GPS connected but no fix yet
  lastGnssLedUpdate = millis();
  gnssLedState = false;
  gpsConnected = true;
  
  myGNSS.setI2COutput(COM_TYPE_UBX);                 // Set the I2C port to output both NMEA and UBX messages
  myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT); // Save (only) the communications port settings to flash and BBR
  enableQZSSL1S();                                   // QZSS L1S信号の受信を有効にする

  myGNSS.setAutoPVTcallbackPtr([](UBX_NAV_PVT_data_t *data)
                               { gpsClient.getPVTdata(data); });

  myGNSS.setAutoRXMSFRBXcallbackPtr([](UBX_RXM_SFRBX_data_t *data)
                                    { gpsClient.newSFRBX(data); }); // UBX-RXM-SFRBXメッセージ受信コールバック関数を登録
  myGNSS.setAutoNAVSATcallbackPtr([](UBX_NAV_SAT_data_t *data)
                                  { gpsClient.newNAVSAT(data); }); // UBX-NAV-SATメッセージ受信コールバック関数を登録
}

void setupRtc()
{
  // Note: Wire1 is already initialized by GPS setup
#ifdef DEBUG_RTC_INIT
  LOG_INFO_MSG("RTC", "Starting RTC initialization on Wire1 bus");
#endif
  
#ifdef DEBUG_RTC_INIT
  // I2C scan on Wire1 bus to verify device presence
  Serial.println("Scanning I2C devices on Wire1 bus:");
  int deviceCount = 0;
  for (byte address = 1; address < 127; address++) {
    Wire1.beginTransmission(address);
    byte error = Wire1.endTransmission();
    if (error == 0) {
      Serial.printf("  Device found at address 0x%02X\n", address);
      deviceCount++;
    }
  }
  Serial.printf("Total I2C devices found: %d\n", deviceCount);
#endif
  
  // Initialize RTClib with Wire1
  if (!rtc.begin(&Wire1)) {
#ifdef DEBUG_RTC_INIT
    LOG_ERR_MSG("RTC", "Could not find RTC DS3231!");
#endif
    return;
  }
  
#ifdef DEBUG_RTC_INIT
  LOG_INFO_MSG("RTC", "RTClib DS3231 initialization: SUCCESS");
#endif
  
  // Check if RTC lost power and needs initialization
  if (rtc.lostPower()) {
#ifdef DEBUG_RTC_INIT
    Serial.println("RTC lost power - setting to compile time");
#endif
    // Set to compile time as initial value
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
#ifdef DEBUG_RTC_INIT
  // Display current RTC time
  DateTime now = rtc.now();
  Serial.printf("Current RTC time: %04d/%02d/%02d %02d:%02d:%02d\n",
                now.year(), now.month(), now.day(),
                now.hour(), now.minute(), now.second());
  
  // Display temperature from DS3231
  Serial.printf("DS3231 temperature: %.2f°C\n", rtc.getTemperature());
#endif
  
#ifdef DEBUG_RTC_INIT
  // Manual DS3231 register verification
  Serial.println("Manual DS3231 register read test:");
  Wire1.beginTransmission(0x68);
  Wire1.write(0x00); // Point to seconds register
  byte ds3231Error = Wire1.endTransmission();
  if (ds3231Error == 0) {
    Wire1.requestFrom(0x68, 7); // Request 7 bytes
    if (Wire1.available() >= 7) {
      byte seconds = Wire1.read();
      byte minutes = Wire1.read();
      byte hours = Wire1.read();
      byte dayOfWeek = Wire1.read();
      byte date = Wire1.read();
      byte month = Wire1.read();
      byte year = Wire1.read();
      
      Serial.printf("Raw DS3231 registers: %02X %02X %02X %02X %02X %02X %02X\n",
                   seconds, minutes, hours, dayOfWeek, date, month, year);
      
      // Convert BCD to decimal
      byte secDec = ((seconds >> 4) * 10) + (seconds & 0x0F);
      byte minDec = ((minutes >> 4) * 10) + (minutes & 0x0F);
      byte hourDec = ((hours >> 4) * 10) + (hours & 0x0F);
      byte dateDec = ((date >> 4) * 10) + (date & 0x0F);
      byte monthDec = ((month >> 4) * 10) + (month & 0x0F);
      byte yearDec = ((year >> 4) * 10) + (year & 0x0F);
      
      Serial.printf("Manual BCD decode: 20%02d/%02d/%02d %02d:%02d:%02d\n",
                   yearDec, monthDec, dateDec, hourDec, minDec, secDec);
    } else {
      Serial.println("DS3231 register read: No data available");
    }
  } else {
    Serial.printf("DS3231 register read failed: error %d\n", ds3231Error);
  }
#endif
}

/**
 * @brief システム初期化のメイン関数
 * 
 * システム初期化を機能別に分割し、可読性と保守性を向上。
 * 各初期化段階は独立した関数として実装され、
 * 初期化順序の依存関係が明確になっている。
 */
void setup()
{
  // 1. 基本ハードウェア初期化
  initializeSerial();
  initializeLEDs();
  initializeI2C_OLED();

  // 2. コアサービス初期化（エラーハンドラ、設定、ログ）
  initializeCoreServices();

  // 3. サービス間依存関係の設定
  setupServiceDependencies();

  // 4. システムモジュール初期化
  initializeSystemModules();

  // 5. NTPサーバー初期化
  initializeNTPServer();

  // 6. Webサーバー初期化と起動
  initializeWebServer();
  server.begin();
  LOG_INFO_F("WEB", "Web server started on port 80 - Network connected: %s", 
             networkManager.isConnected() ? "YES" : "NO");

  // 7. GPS/RTC ハードウェア初期化
  LOG_INFO_MSG("GPS", "Starting GPS initialization");
  setupGps();
  LOG_INFO_MSG("RTC", "Starting RTC initialization on Wire1 bus");
  setupRtc();

  // 8. PPS割り込み設定
  attachInterrupt(digitalPinToInterrupt(GPS_PPS_PIN), triggerPps, FALLING);
  LOG_INFO_MSG("GPS", "PPS interrupt attached to GPIO pin");

  // 9. システムコントローラ初期化
  systemController.init();
  systemController.setServices(&timeManager, &networkManager, systemMonitor,
                               ntpServer, &displayManager, &configManager,
                               loggingService, prometheusMetrics);
  
  systemController.updateGpsStatus(gpsConnected);
  systemController.updateNetworkStatus(networkManager.isConnected());
  systemController.updateDisplayStatus(true);
  LOG_INFO_MSG("SYSTEM", "SystemController initialized and services registered");

  // 10. 物理リセット機能初期化
  initializePhysicalReset();

  LOG_INFO_MSG("SYSTEM", "System initialization completed successfully");
}

unsigned long ledOffTime = 0;

void loop()
{
  // Performance optimization: 3-tier timing control
  static unsigned long lastLowPriorityUpdate = 0;
  static unsigned long lastMediumPriorityUpdate = 0;
  unsigned long currentTime = millis();
  
  // ====== HIGH PRIORITY (every loop) ======
  // Critical operations that must run immediately
  errorHandler.update();
  physicalReset.update();
  
  // ====== MEDIUM PRIORITY (100ms interval) ======
  // Operations that need regular updates but not every loop
  if (currentTime - lastMediumPriorityUpdate >= 100) {
    displayManager.update();
    systemController.update();
    
    // GPS signal monitoring and fallback management
    if (systemMonitor) {
      systemMonitor->monitorGpsSignal();
    }
    
    lastMediumPriorityUpdate = currentTime;
  }
  
  // ====== LOW PRIORITY (1000ms interval) ======
  // Status updates and non-critical monitoring
  if (currentTime - lastLowPriorityUpdate >= 1000) {
    // Update hardware status in SystemController
    systemController.updateGpsStatus(gpsConnected);
    systemController.updateNetworkStatus(networkManager.isConnected());
    
    // Network monitoring and recovery
    networkManager.monitorConnection();
    networkManager.attemptReconnection();
    
    // Update Prometheus metrics (moved to low priority for performance)
    if (prometheusMetrics && ntpServer && systemMonitor) {
      GpsSummaryData gpsData = gpsClient.getGpsSummaryData();
      const NtpStatistics& ntpStats = ntpServer->getStatistics();
      const GpsMonitor& gpsMonitor = systemMonitor->getGpsMonitor();
      extern TimeManager timeManager;
      unsigned long ppsCount = timeManager.getPpsCount();
      
      prometheusMetrics->update(&ntpStats, &gpsData, &gpsMonitor, ppsCount);
    }
    
    lastLowPriorityUpdate = currentTime;
  }
  
  if (gpsConnected) {
    myGNSS.checkUblox();     // Check for the arrival of new data and process it.
    myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.
    
    // PPS signal processing
    GpsSummaryData gpsData = gpsClient.getGpsSummaryData();
    timeManager.processPpsSync(gpsData);
    
    // GNSS Fix Status LED control based on GPS fix quality
    if (gpsData.fixType >= 3) { // 3D fix or better
      gnssBlinkInterval = 0; // ON (常時点灯): 3D fix or better
      digitalWrite(LED_GNSS_FIX_PIN, HIGH);
    } else if (gpsData.fixType >= 2) { // 2D fix
      gnssBlinkInterval = 500; // FAST BLINK (0.5秒間隔): 2D fix available
    } else { // GPS connected but no fix
      gnssBlinkInterval = 2000; // SLOW BLINK (2秒間隔): GPS connected but no fix
    }
  } else {
    // GPS not connected - turn off GNSS fix LED
    gnssBlinkInterval = 0;
    digitalWrite(LED_GNSS_FIX_PIN, LOW);
  }
  
  // Handle GNSS LED blinking pattern
  if (gnssBlinkInterval > 0) {
    unsigned long currentTime = millis();
    if (currentTime - lastGnssLedUpdate >= gnssBlinkInterval) {
      gnssLedState = !gnssLedState;
      digitalWrite(LED_GNSS_FIX_PIN, gnssLedState ? HIGH : LOW);
      lastGnssLedUpdate = currentTime;
    }
  }
  
  // LED management (non-blocking)
  if (ledOffTime == 0 && digitalRead(LED_PPS_PIN)) {
    ledOffTime = millis() + 50; // Turn off LED after 50ms
  }
  if (ledOffTime > 0 && millis() > ledOffTime) {
    analogWrite(LED_PPS_PIN, 0);
    ledOffTime = 0;
  }

  // Webserver handling (always check, regardless of network status)
  webServer.handleClient(Serial, server, gpsClient.getUbxNavSatData_t(), gpsClient.getGpsSummaryData());

  // Display management
  // Note: Button handling is now managed by PhysicalReset class
  // displayManager.update() moved to medium priority (100ms interval)
  
  if (displayManager.shouldDisplay()) {
    GpsSummaryData gpsData = gpsClient.getGpsSummaryData();
    
    // Display content based on current mode
    // Note: Removed verbose main loop logging for cleaner output
    switch (displayManager.getCurrentMode()) {
      case DISPLAY_GPS_TIME:
      case DISPLAY_GPS_SATS:
        displayManager.displayInfo(gpsData);
        break;
        
      case DISPLAY_NTP_STATS:
        if (ntpServer) {
          displayManager.displayNtpStats(ntpServer->getStatistics());
        }
        break;
        
      case DISPLAY_SYSTEM_STATUS:
        displayManager.displaySystemStatus(
          gpsConnected, 
          networkManager.isConnected(), 
          millis() / 1000
        );
        break;
        
      case DISPLAY_ERROR:
        // Error display is handled automatically by DisplayManager
        break;
        
      default:
        displayManager.displayInfo(gpsData);
        break;
    }
  }

  // ====== CRITICAL OPERATIONS (every loop) ======
  // Time-sensitive operations that cannot be delayed
  
  // UDP socket management and NTP request processing (high priority)
  networkManager.manageUdpSockets();
  if (ntpServer) {
    ntpServer->processRequests();
  }

  // Process logging service (syslog transmission and retries)
  if (loggingService) {
    loggingService->processLogs();
  }

  // ====== LOW PRIORITY OPERATIONS (already moved above) ======
  // Network monitoring, status updates moved to 1-second interval
  
  // Debug network status every 30 seconds (reduced frequency)
  static unsigned long lastNetworkDebug = 0;
  if (millis() - lastNetworkDebug > 30000) {
    lastNetworkDebug = millis();
#ifdef DEBUG_NETWORK
    Serial.print("Network Status - Connected: ");
    Serial.print(networkManager.isConnected() ? "YES" : "NO");
    if (networkManager.isConnected()) {
      Serial.print(", IP: ");
      Serial.print(Ethernet.localIP());
    }
    Serial.print(", Hardware: ");
    switch(Ethernet.hardwareStatus()) {
      case EthernetNoHardware: Serial.print("NO_HW"); break;
      case EthernetW5100: Serial.print("W5100"); break;
      case EthernetW5200: Serial.print("W5200"); break;
      case EthernetW5500: Serial.print("W5500"); break;
      default: Serial.print("UNKNOWN"); break;
    }
    Serial.print(", Link: ");
    switch(Ethernet.linkStatus()) {
      case Unknown: Serial.print("UNKNOWN"); break;
      case LinkON: Serial.print("ON"); break;
      case LinkOFF: Serial.print("OFF"); break;
    }
    Serial.println();
#endif
  }

#if defined(DEBUG_CONSOLE_GPS)
  // Get current RTC time using RTClib
  DateTime now = rtc.now();
  Serial.print("RTC DateTime: ");

  char dateTimechr[20];
  sprintf(dateTimechr, "%04d/%02d/%02d %02d:%02d:%02d",
          now.year(), now.month(), now.day(),
          now.hour(), now.minute(), now.second());
  Serial.print(dateTimechr);
  
  // Additional RTC debug info
  static unsigned long lastRtcDetailDebug = 0;
  if (millis() - lastRtcDetailDebug > 10000) { // Every 10 seconds
    Serial.print(" [I2C Address: 0x68, Wire1 Bus]");
    if (rtc.lostPower()) {
      Serial.print(" [POWER_LOST]");
    }
    Serial.printf(" Temp: %.2f°C", rtc.getTemperature());
    lastRtcDetailDebug = millis();
  }

  switch (now.dayOfTheWeek())
  {
  case 1:
    Serial.print(" Sun");
    break;
  case 2:
    Serial.print(" Mon");
    break;
  case 3:
    Serial.print(" Tue");
    break;
  case 4:
    Serial.print(" Wed");
    break;
  case 5:
    Serial.print(" Thu");
    break;
  case 6:
    Serial.print(" Fri");
    break;
  case 7:
    Serial.print(" Sat");
    break;
  default:
    // Nothing
    break;
  }

  Serial.print(" - Temp: ");
  Serial.print(rtc.getTemperature());

  Serial.println();
  delay(1000);
#endif
}