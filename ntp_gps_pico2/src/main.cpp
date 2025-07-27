#include <Ethernet.h>
#include <Wire.h>
#include <SPI.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include "RTClib.h"
#include <WebServer.h>
#include <time.h>
#include <EthernetUdp.h>

// Refactored modules
#include "HardwareConfig.h"
#include "SystemTypes.h"
#include "TimeManager.h"
#include "NetworkManager.h"
#include "SystemMonitor.h"
#include "NtpServer.h"
#include "NtpTypes.h"
#include "DisplayManager.h"
#include "ConfigManager.h"
#include "LoggingService.h"
#include "PrometheusMetrics.h"
#include "SystemController.h"
#include "ErrorHandler.h"
#include "PhysicalReset.h"

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
void trigerPps() {
  timeManager.onPpsInterrupt();
  lastPps = micros();
}

// Function moved to NetworkManager class

// Function moved to NetworkManager class

// Function moved to NetworkManager class

// Function moved to NtpServer class

// Function moved to DisplayManager class

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

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(SERIAL_BAUD_RATE);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  // Note: Can't use LOG_INFO_MSG yet as LoggingService is not initialized
  Serial.println("=== GPS NTP Server v1.0 ===");

  // LED initialization
  pinMode(LED_GNSS_FIX_PIN, OUTPUT);  // GNSS Fix Status LED (Green)
  pinMode(LED_NETWORK_PIN, OUTPUT);   // Network Status LED (Blue)
  pinMode(LED_ERROR_PIN, OUTPUT);     // Error Status LED (Red)
  pinMode(LED_PPS_PIN, OUTPUT);       // PPS Status LED (Yellow)
  pinMode(LED_ONBOARD_PIN, OUTPUT);   // Onboard LED

  // Note: Button initialization is handled by PhysicalReset via ButtonHAL
  // GPIO 11 is managed centrally to avoid conflicts

  // I2C for OLED (Wire0 bus - GPIO 0/1)
  Wire.setSDA(0);  // GPIO 0 for SDA
  Wire.setSCL(1);  // GPIO 1 for SCL
  
  // Enable internal pull-ups (in case external pull-ups are missing)
  pinMode(0, INPUT_PULLUP);  // SDA pull-up
  pinMode(1, INPUT_PULLUP);  // SCL pull-up
  
  // Note: Can't use LOG_INFO_MSG yet as LoggingService is not initialized
  Serial.println("I2C pull-ups enabled, starting I2C...");
  Wire.begin();
  Wire.setClock(100000); // Use slower 100kHz for more reliable communication
  Serial.println("Wire0 initialized for OLED display - SDA: GPIO 0, SCL: GPIO 1");

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
  
  // Set LoggingService references for components
  displayManager.setLoggingService(loggingService);
  networkManager.setLoggingService(loggingService);
  networkManager.setConfigManager(&configManager);
  timeManager.setLoggingService(loggingService);
  systemMonitor->setLoggingService(loggingService);

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

  // Initialize NTP server (now using static instance)
  const UdpSocketManager& udpStatus = networkManager.getUdpStatus();
  // Update the static instance with the UDP socket manager
  ntpServerInstance = NtpServer(&ntpUdp, &timeManager, const_cast<UdpSocketManager*>(&udpStatus));
  // Set LoggingService BEFORE calling init()
  ntpServer->setLoggingService(loggingService);
  ntpServer->init();
  LOG_INFO_MSG("NTP", "NTP Server initialized and listening on port 123");
  
  // Connect ConfigManager and PrometheusMetrics to web server
  webServer.setConfigManager(&configManager);
  webServer.setPrometheusMetrics(prometheusMetrics);
  LOG_INFO_MSG("WEB", "WebServer configured with ConfigManager and PrometheusMetrics");

  // Webサーバーを起動（ネットワーク接続状態に関わらず起動）
  server.begin();
  LOG_INFO_F("WEB", "Web server started on port 80 - Network connected: %s", 
             networkManager.isConnected() ? "YES" : "NO");
  if (networkManager.isConnected()) {
    LOG_INFO_F("WEB", "Web server accessible at http://%d.%d.%d.%d", 
               Ethernet.localIP()[0], Ethernet.localIP()[1], 
               Ethernet.localIP()[2], Ethernet.localIP()[3]);
  } else {
    LOG_INFO_MSG("WEB", "Web server accessible at http://0.0.0.0");
  }

  // setup GPS
  LOG_INFO_MSG("GPS", "Starting GPS initialization");
  setupGps();

  // Setup RTC after GPS initialization (they share Wire1 bus)
  LOG_INFO_MSG("RTC", "Starting RTC initialization on Wire1 bus");
  setupRtc();

  // GPS PPS interrupt
  attachInterrupt(digitalPinToInterrupt(GPS_PPS_PIN), trigerPps, FALLING);
  LOG_INFO_MSG("GPS", "PPS interrupt attached to GPIO pin");

  // Initialize SystemController and register all services
  systemController.init();
  systemController.setServices(&timeManager, &networkManager, systemMonitor,
                               ntpServer, &displayManager, &configManager,
                               loggingService, prometheusMetrics);
  
  // Update hardware status in SystemController
  systemController.updateGpsStatus(gpsConnected);
  systemController.updateNetworkStatus(networkManager.isConnected());
  systemController.updateDisplayStatus(true); // Display is always available
  
  LOG_INFO_MSG("SYSTEM", "SystemController initialized and services registered");
  
  // Initialize PhysicalReset with DisplayManager and ConfigManager
  if (physicalReset.initialize(&displayManager, &configManager)) {
    LOG_INFO_MSG("RESET", "PhysicalReset initialized successfully");
  } else {
    LOG_ERR_MSG("RESET", "PhysicalReset initialization failed");
    REPORT_ERROR(ErrorType::SYSTEM_ERROR, "RESET", "PhysicalReset initialization failed");
  }
  
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