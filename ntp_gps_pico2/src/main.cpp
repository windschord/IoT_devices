#include <Ethernet.h>
#include <Wire.h>
#include <SPI.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include <uRTCLib.h>
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

// Hardware configuration moved to HardwareConfig.h

// Global hardware instances
SFE_UBLOX_GNSS myGNSS;
EthernetServer server(80);
EthernetUDP ntpUdp;
WebServer webServer;
GpsClient gpsClient(Serial);
Adafruit_SH1106 display(OLED_RESET);
uRTCLib rtc;
byte rtcModel = RTC_MODEL;

// Global system instances
ConfigManager configManager;
TimeSync timeSync = {0, 0, 0, 0, false, 1.0};
TimeManager timeManager(&rtc, &timeSync, nullptr);
NetworkManager networkManager(&ntpUdp);
SystemMonitor* systemMonitor = nullptr;
NtpServer* ntpServer = nullptr;
DisplayManager displayManager(&display);
LoggingService* loggingService = nullptr;
PrometheusMetrics* prometheusMetrics = nullptr;

// Global state variables
volatile unsigned long lastPps = 0;
volatile bool ppsReceived = false;
bool gpsConnected = false;
bool webServerStarted = false;

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
  Serial.println("=== GPS Setup Starting ===");
  Serial.print("GPS SDA Pin: "); Serial.println(GPS_SDA_PIN);
  Serial.print("GPS SCL Pin: "); Serial.println(GPS_SCL_PIN);
  
  Wire1.setSDA(GPS_SDA_PIN);
  Wire1.setSCL(GPS_SCL_PIN);
  Wire1.begin();
  Serial.println("Wire1 initialized");

  Serial.println("Attempting to connect to u-blox GNSS module...");
  if (myGNSS.begin(Wire1) == false) // Connect to the u-blox module using Wire port
  {
    Serial.println(F("❌ FAILED: u-blox GNSS not detected at default I2C address (0x42)"));
    Serial.println(F("   Check I2C wiring: SDA=GPIO6, SCL=GPIO7"));
    Serial.println(F("   Check power supply to GPS module"));
    Serial.println(F("❌ GPS initialization FAILED - continuing without GPS"));
    LOG_ERR_MSG("GPS", "u-blox GNSS not detected at I2C address 0x42");
    LOG_ERR_MSG("GPS", "Check wiring - SDA=GPIO6, SCL=GPIO7 and power supply");
    analogWrite(LED_ERROR_PIN, 255);
    displayManager.displayError("GPS Module not detected. Check wiring.");
    gpsConnected = false;
    return; // エラーで停止せず、続行する
  }
  
  Serial.println("✅ GPS module connected successfully!");
  Serial.println("✅ GPS initialization completed");
  LOG_INFO_MSG("GPS", "u-blox GNSS module connected successfully at I2C 0x42");
  LOG_INFO_MSG("GPS", "QZSS L1S signal reception enabled for disaster alerts");
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
  URTCLIB_WIRE.begin();
  rtc.set_rtc_address(0x68);
  rtc.set_model(rtcModel);
  // refresh data from RTC HW in RTC class object so flags like rtc.lostPower(), rtc.getEOSCFlag(), etc, can get populated
  rtc.refresh();
  
  // Check if RTC lost power and needs initialization
  if (rtc.lostPower()) {
    Serial.println("RTC lost power - setting to current time (2025-01-21 12:00:00)");
    // RTCLib::set(byte second, byte minute, byte hour (0-23:24-hr mode only), byte dayOfWeek (Tue = 3), byte dayOfMonth (1-31), byte month (1-12), byte year (25))
    rtc.set(0, 0, 12, 3, 21, 1, 25);  // 2025-01-21 12:00:00 Tuesday
  } else {
    Serial.print("RTC time: ");
    Serial.printf("20%02d/%02d/%02d %02d:%02d:%02d\n", 
                  rtc.year(), rtc.month(), rtc.day(), 
                  rtc.hour(), rtc.minute(), rtc.second());
  }

  // use the following if you want to set main clock in 24 hour mode
  rtc.set_12hour_mode(false);

  if (rtc.enableBattery())
  {
    Serial.println("Battery activated correctly.");
  }
  else
  {
    Serial.println("ERROR activating battery.");
  }
  // Check whether OSC is set to use VBAT or not
  if (rtc.getEOSCFlag())
    Serial.println(F("Oscillator will not use VBAT when VCC cuts off. Time will not increment without VCC!"));
  else
    Serial.println(F("Oscillator will use VBAT when VCC cuts off."));

  Serial.print("Lost power status: ");
  if (rtc.lostPower())
  {
    Serial.print("POWER FAILED. Clearing flag...");
    rtc.lostPowerClear();
    Serial.println(" done.");
  }
  else
  {
    Serial.println("POWER OK");
  }
}

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(SERIAL_BAUD_RATE);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Start");

  // LED initialization
  pinMode(LED_PPS_PIN, OUTPUT);
  pinMode(LED_ONBOARD_PIN, OUTPUT);
  pinMode(LED_ERROR_PIN, OUTPUT);

  // I2C for OLED and RTC
  Wire.begin();

  // Initialize configuration manager first
  configManager.init();

  // RTC setup
  setupRtc();

  // Initialize system modules  
  networkManager.init();
  displayManager.init();
  
  // Initialize logging service
  loggingService = new LoggingService(&ntpUdp);
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
  
  // Initialize Prometheus metrics
  prometheusMetrics = new PrometheusMetrics();
  prometheusMetrics->init();
  LOG_INFO_MSG("SYSTEM", "PrometheusMetrics initialized");

  // Log system startup
  LOG_INFO_MSG("SYSTEM", "GPS NTP Server starting up");
  LOG_INFO_F("SYSTEM", "RAM: %lu bytes, Flash: %lu bytes", 
             (unsigned long)17856, (unsigned long)406192);

  // Initialize system monitor with references FIRST
  systemMonitor = new SystemMonitor(&gpsClient, &gpsConnected, &ppsReceived);
  systemMonitor->init();
  LOG_INFO_MSG("SYSTEM", "SystemMonitor initialized");
  
  // Initialize TimeManager and set GpsMonitor reference
  timeManager.init();
  timeManager.setGpsMonitor(&systemMonitor->getGpsMonitor());
  LOG_INFO_MSG("SYSTEM", "TimeManager initialized with GPS monitor reference");

  // Initialize NTP server
  const UdpSocketManager& udpStatus = networkManager.getUdpStatus();
  ntpServer = new NtpServer(&ntpUdp, &timeManager, const_cast<UdpSocketManager*>(&udpStatus));
  ntpServer->init();
  LOG_INFO_MSG("NTP", "NTP Server initialized and listening on port 123");
  
  // Connect ConfigManager and PrometheusMetrics to web server
  webServer.setConfigManager(&configManager);
  webServer.setPrometheusMetrics(prometheusMetrics);
  LOG_INFO_MSG("WEB", "WebServer configured with ConfigManager and PrometheusMetrics");

  // Webサーバーを起動（ネットワーク接続状態に関わらず起動）
  server.begin();
  Serial.print("Web server started on port 80 - Network connected: ");
  Serial.println(networkManager.isConnected() ? "YES" : "NO");
  if (networkManager.isConnected()) {
    Serial.print("Server accessible at: http://");
    Serial.println(Ethernet.localIP());
    LOG_INFO_F("WEB", "Web server accessible at http://%d.%d.%d.%d", 
               Ethernet.localIP()[0], Ethernet.localIP()[1], 
               Ethernet.localIP()[2], Ethernet.localIP()[3]);
  } else {
    LOG_WARN_MSG("WEB", "Web server started without network connection");
  }

  // setup GPS
  LOG_INFO_MSG("GPS", "Starting GPS initialization");
  setupGps();

  // GPS PPS interrupt
  attachInterrupt(digitalPinToInterrupt(GPS_PPS_PIN), trigerPps, FALLING);
  LOG_INFO_MSG("GPS", "PPS interrupt attached to GPIO pin");

  LOG_INFO_MSG("SYSTEM", "System initialization completed successfully");
  Serial.println("System initialization completed");
}

unsigned long ledOffTime = 0;

void loop()
{
  // GPS signal monitoring and fallback management
  if (systemMonitor) {
    systemMonitor->monitorGpsSignal();
  }
  
  if (gpsConnected) {
    myGNSS.checkUblox();     // Check for the arrival of new data and process it.
    myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.
    
    // PPS signal processing
    GpsSummaryData gpsData = gpsClient.getGpsSummaryData();
    timeManager.processPpsSync(gpsData);
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
  displayManager.checkDisplayButton();
  displayManager.update();
  
  if (displayManager.shouldDisplay() && micros() - lastPps > 1000) {
    GpsSummaryData gpsData = gpsClient.getGpsSummaryData();
    
    // Display content based on current mode
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

  // Network connection monitoring and automatic recovery
  networkManager.monitorConnection();
  networkManager.attemptReconnection();
  
  // Debug network status every 10 seconds
  static unsigned long lastNetworkDebug = 0;
  if (millis() - lastNetworkDebug > 10000) {
    lastNetworkDebug = millis();
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
  }
  
  // UDP socket management and NTP request processing
  networkManager.manageUdpSockets();
  if (ntpServer) {
    ntpServer->processRequests();
  }

  // Process logging service (syslog transmission and retries)
  if (loggingService) {
    loggingService->processLogs();
  }

  // Update Prometheus metrics
  if (prometheusMetrics && ntpServer && systemMonitor) {
    GpsSummaryData gpsData = gpsClient.getGpsSummaryData();
    const NtpStatistics& ntpStats = ntpServer->getStatistics();
    const GpsMonitor& gpsMonitor = systemMonitor->getGpsMonitor();
    extern TimeManager timeManager;
    unsigned long ppsCount = timeManager.getPpsCount();
    
    prometheusMetrics->update(&ntpStats, &gpsData, &gpsMonitor, ppsCount);
  }

#if defined(DEBUG_CONSOLE_GPS)
  rtc.refresh();
  Serial.print("RTC DateTime: ");

  char dateTimechr[20];
  sprintf(dateTimechr, "20%02d/%02d/%02d %02d:%02d:%02d",
          rtc.year(), rtc.month(), rtc.day(),
          rtc.hour(), rtc.minute(), rtc.second());
  Serial.print(dateTimechr);

  switch (rtc.dayOfWeek())
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
  Serial.print((float)rtc.temp() / 100);

  Serial.println();
  delay(1000);
#endif
}