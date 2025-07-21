#include <Ethernet.h>
#include <Wire.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include <uRTCLib.h>
#include <WebServer.h>
#include <Gps_Client.h>
#include <time.h>
#include <EthernetUdp.h>

#define GPS_PPS_PIN 8
#define GPS_SDA_PIN 6
#define GPS_SCL_PIN 7
#define BTN_DISPLAY_PIN 11
#define LED_ERROR_PIN 14
#define LED_PPS_PIN 15
#define LED_ONBOARD_PIN 25

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

SFE_UBLOX_GNSS myGNSS;
EthernetServer server(80);
EthernetUDP ntpUdp;        // NTPサーバー用UDPソケット
WebServer webServer;
GpsClient gpsClient(Serial);

#define NTP_PORT 123           // NTP標準ポート
#define NTP_PACKET_SIZE 48     // NTPパケットサイズ
byte ntpPacketBuffer[NTP_PACKET_SIZE]; // NTPパケットバッファ
Adafruit_SH1106 display(OLED_RESET);
uRTCLib rtc;
byte rtcModel = URTCLIB_MODEL_DS3231;

// Enter a MAC address for your controller below.
// https://www.hellion.org.uk/cgi-bin/randmac.pl?scope=local&type=unicast
byte mac[] = {
    0x6e, 0xc9, 0x4c, 0x32, 0x3a, 0xf6};

volatile unsigned long lastPps = 0;
volatile unsigned long ppsTimestamp = 0;
volatile bool ppsReceived = false;
volatile unsigned long ppsCount = 0;
bool gpsConnected = false;

// ネットワーク状態監視とエラーハンドリング
struct NetworkMonitor {
  bool isConnected;              // イーサネット接続状態
  bool dhcpActive;               // DHCP取得状態
  unsigned long lastLinkCheck;   // 最後のリンクチェック時刻
  unsigned long linkCheckInterval; // リンクチェック間隔 (5秒)
  int reconnectAttempts;         // 再接続試行回数
  int maxReconnectAttempts;      // 最大再接続試行回数
  unsigned long lastReconnectTime; // 最後の再接続試行時刻
  unsigned long reconnectInterval; // 再接続間隔 (30秒)
  uint32_t localIP;              // 現在のローカルIP
  uint32_t gateway;              // ゲートウェイIP
  uint32_t dnsServer;            // DNSサーバーIP
  bool ntpServerActive;          // NTPサーバー状態
} networkMonitor = {false, false, 0, 5000, 0, 5, 0, 30000, 0, 0, 0, false};

// UDPソケット管理構造体
struct UdpSocketManager {
  bool ntpSocketOpen;            // NTP UDPソケット状態
  unsigned long lastSocketCheck; // 最後のソケットチェック
  unsigned long socketCheckInterval; // ソケットチェック間隔 (10秒)
  int socketErrors;              // ソケットエラーカウント
} udpManager = {false, 0, 10000, 0};

// GPS信号監視とフォールバック管理
struct GpsMonitor {
  unsigned long lastValidTime;     // 最後の有効なGPS時刻取得時刻
  unsigned long lastPpsTime;       // 最後のPPS信号受信時刻
  unsigned long ppsTimeoutMs;      // PPSタイムアウト時間 (30秒)
  unsigned long gpsTimeoutMs;      // GPS時刻タイムアウト時間 (60秒)
  bool ppsActive;                  // PPS信号の状態
  bool gpsTimeValid;               // GPS時刻の有効性
  int signalQuality;               // 信号品質 (0-10)
  int satelliteCount;              // 受信衛星数
  bool inFallbackMode;             // フォールバックモード状態
} gpsMonitor = {0, 0, 30000, 60000, false, false, 0, 0, false};

// 高精度時刻同期用の構造体
struct TimeSync {
  unsigned long gpsTime;      // GPS時刻 (Unix timestamp)
  unsigned long ppsTime;      // PPS受信時のマイクロ秒
  unsigned long rtcTime;      // RTC時刻
  bool synchronized;          // 同期状態
  float accuracy;            // 精度 (秒)
} timeSync = {0, 0, 0, false, 1.0};

// 高精度時刻取得関数 (フォールバック対応)
unsigned long getHighPrecisionTime() {
  if (timeSync.synchronized && gpsConnected && !gpsMonitor.inFallbackMode) {
    // PPS同期された高精度時刻を返す
    unsigned long elapsed = micros() - timeSync.ppsTime;
    return timeSync.gpsTime * 1000 + elapsed / 1000; // ミリ秒単位
  } else {
    // RTCフォールバック時刻
    rtc.refresh();
    struct tm timeinfo = {0};
    timeinfo.tm_year = rtc.year() + 100; // RTCは2桁年
    timeinfo.tm_mon = rtc.month() - 1;
    timeinfo.tm_mday = rtc.day();
    timeinfo.tm_hour = rtc.hour();
    timeinfo.tm_min = rtc.minute();
    timeinfo.tm_sec = rtc.second();
    return mktime(&timeinfo) * 1000 + millis() % 1000;
  }
}

// NTPサーバー用のStratum レベル取得
int getNtpStratum() {
  if (timeSync.synchronized && !gpsMonitor.inFallbackMode) {
    return 1; // GPS同期時はStratum 1
  } else {
    return 3; // RTCフォールバック時はStratum 3
  }
}

// GPS信号品質監視とフォールバック管理
void monitorGpsSignal() {
  unsigned long now = millis();
  bool wasInFallback = gpsMonitor.inFallbackMode;
  
  if (gpsConnected) {
    // GPS時刻データを取得
    GpsSummaryData gpsData = gpsClient.getGpsSummaryData();
    
    // GPS時刻の有効性をチェック
    if (gpsData.timeValid && gpsData.dateValid) {
      gpsMonitor.lastValidTime = now;
      gpsMonitor.gpsTimeValid = true;
      gpsMonitor.satelliteCount = gpsData.SIV;
      
      // 信号品質を衛星数から推定 (簡易版)
      if (gpsData.SIV >= 8) gpsMonitor.signalQuality = 10;
      else if (gpsData.SIV >= 6) gpsMonitor.signalQuality = 8;
      else if (gpsData.SIV >= 4) gpsMonitor.signalQuality = 6;
      else if (gpsData.SIV >= 3) gpsMonitor.signalQuality = 4;
      else gpsMonitor.signalQuality = 2;
    } else {
      gpsMonitor.gpsTimeValid = false;
      gpsMonitor.signalQuality = 0;
    }
  }
  
  // PPS信号の監視
  if (ppsReceived) {
    gpsMonitor.lastPpsTime = now;
    gpsMonitor.ppsActive = true;
  } else if (now - gpsMonitor.lastPpsTime > gpsMonitor.ppsTimeoutMs) {
    gpsMonitor.ppsActive = false;
  }
  
  // フォールバックモードの判定
  bool shouldFallback = false;
  
  if (!gpsConnected) {
    shouldFallback = true;
  } else if (!gpsMonitor.gpsTimeValid && (now - gpsMonitor.lastValidTime > gpsMonitor.gpsTimeoutMs)) {
    shouldFallback = true;
  } else if (!gpsMonitor.ppsActive && (now - gpsMonitor.lastPpsTime > gpsMonitor.ppsTimeoutMs)) {
    shouldFallback = true;
  }
  
  // フォールバックモード状態の更新
  if (shouldFallback && !gpsMonitor.inFallbackMode) {
    // フォールバックモードに移行
    gpsMonitor.inFallbackMode = true;
    timeSync.synchronized = false;
    timeSync.accuracy = 10.0; // RTC精度に下げる
    analogWrite(LED_ERROR_PIN, 255); // エラーLED点灯
    
#if defined(DEBUG_CONSOLE_GPS)
    Serial.println("GPS signal lost - entering fallback mode (using RTC)");
#endif
  } else if (!shouldFallback && gpsMonitor.inFallbackMode) {
    // フォールバックモードから復帰
    gpsMonitor.inFallbackMode = false;
    analogWrite(LED_ERROR_PIN, 0); // エラーLED消灯
    
#if defined(DEBUG_CONSOLE_GPS)
    Serial.println("GPS signal recovered - exiting fallback mode");
#endif
  }
  
  // デバッグ情報の出力
#if defined(DEBUG_CONSOLE_GPS)
  if (now % 10000 < 100) { // 10秒に1回
    Serial.print("GPS Monitor - Sats: ");
    Serial.print(gpsMonitor.satelliteCount);
    Serial.print(" Quality: ");
    Serial.print(gpsMonitor.signalQuality);
    Serial.print(" PPS: ");
    Serial.print(gpsMonitor.ppsActive ? "OK" : "FAIL");
    Serial.print(" Mode: ");
    Serial.println(gpsMonitor.inFallbackMode ? "FALLBACK" : "GPS");
  }
#endif
}

void trigerPps()
{
  unsigned long now = micros();
  ppsTimestamp = now;
  ppsReceived = true;
  ppsCount++;
  
  // LED点滅（非ブロッキング）
  analogWrite(LED_PPS_PIN, 255);
  
#if defined(DEBUG_CONSOLE_PPS)
  // 割り込み内では最小限の処理のみ
  lastPps = now;
#endif
}

// ネットワーク状態監視と堅牢な再接続処理
void monitorNetworkConnection() {
  unsigned long now = millis();
  bool wasConnected = networkMonitor.isConnected;
  
  // 定期的なリンク状態チェック
  if (now - networkMonitor.lastLinkCheck > networkMonitor.linkCheckInterval) {
    networkMonitor.lastLinkCheck = now;
    
    // ハードウェア状態チェック
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("W5500 hardware not detected");
      networkMonitor.isConnected = false;
      return;
    }
    
    // リンク状態チェック
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable disconnected");
      networkMonitor.isConnected = false;
    } else {
      // IP取得状態チェック
      IPAddress currentIP = Ethernet.localIP();
      if (currentIP[0] == 0) {
        networkMonitor.isConnected = false;
        networkMonitor.dhcpActive = false;
      } else {
        networkMonitor.isConnected = true;
        networkMonitor.dhcpActive = true;
        networkMonitor.localIP = (uint32_t)currentIP;
        networkMonitor.gateway = (uint32_t)Ethernet.gatewayIP();
        networkMonitor.dnsServer = (uint32_t)Ethernet.dnsServerIP();
        networkMonitor.reconnectAttempts = 0; // 成功時にリセット
      }
    }
  }
  
  // DHCP維持処理
  int dhcpResult = Ethernet.maintain();
  switch (dhcpResult) {
    case 1: // renewed fail
      Serial.println("DHCP renewal failed");
      networkMonitor.dhcpActive = false;
      break;
    case 2: // renewed success
      Serial.println("DHCP renewed successfully");
      Serial.print("IP: ");
      Serial.println(Ethernet.localIP());
      networkMonitor.dhcpActive = true;
      break;
    case 3: // rebind fail
      Serial.println("DHCP rebind failed");
      networkMonitor.dhcpActive = false;
      break;
    case 4: // rebind success
      Serial.println("DHCP rebound successfully");
      Serial.print("IP: ");
      Serial.println(Ethernet.localIP());
      networkMonitor.dhcpActive = true;
      break;
  }
  
  // 接続状態の変化を検出
  if (wasConnected && !networkMonitor.isConnected) {
    Serial.println("Network connection lost");
    digitalWrite(LED_ONBOARD_PIN, LOW); // ネットワークLED消灯
  } else if (!wasConnected && networkMonitor.isConnected) {
    Serial.println("Network connection established");
    digitalWrite(LED_ONBOARD_PIN, HIGH); // ネットワークLED点灯
    Serial.print("IP: ");
    Serial.print(Ethernet.localIP());
    Serial.print(", Gateway: ");
    Serial.print(Ethernet.gatewayIP());
    Serial.print(", DNS: ");
    Serial.println(Ethernet.dnsServerIP());
  }
}

// ネットワーク再接続処理
void attemptNetworkReconnection() {
  unsigned long now = millis();
  
  if (!networkMonitor.isConnected && 
      networkMonitor.reconnectAttempts < networkMonitor.maxReconnectAttempts &&
      (now - networkMonitor.lastReconnectTime > networkMonitor.reconnectInterval)) {
    
    networkMonitor.lastReconnectTime = now;
    networkMonitor.reconnectAttempts++;
    
    Serial.print("Attempting network reconnection (attempt ");
    Serial.print(networkMonitor.reconnectAttempts);
    Serial.print("/");
    Serial.print(networkMonitor.maxReconnectAttempts);
    Serial.println(")");
    
    // W5500リセット
    if (Ethernet.hardwareStatus() != EthernetNoHardware) {
      Serial.println("Resetting W5500...");
      
      // DHCP再試行
      if (Ethernet.begin(mac) == 0) {
        Serial.println("DHCP failed, will retry in 30 seconds");
      } else {
        Serial.println("DHCP reconnection successful");
        networkMonitor.isConnected = true;
        networkMonitor.dhcpActive = true;
        networkMonitor.reconnectAttempts = 0;
      }
    }
  }
}

// UDPソケットの管理と監視
void manageUdpSockets() {
  unsigned long now = millis();
  
  // 定期的なソケット状態チェック
  if (now - udpManager.lastSocketCheck > udpManager.socketCheckInterval) {
    udpManager.lastSocketCheck = now;
    
    if (networkMonitor.isConnected) {
      // NTP UDPソケットの管理
      if (!udpManager.ntpSocketOpen) {
        Serial.println("Opening NTP UDP socket on port 123");
        if (ntpUdp.begin(NTP_PORT)) {
          udpManager.ntpSocketOpen = true;
          networkMonitor.ntpServerActive = true;
          Serial.println("NTP UDP socket opened successfully");
        } else {
          Serial.println("Failed to open NTP UDP socket");
          udpManager.socketErrors++;
        }
      }
    } else {
      // ネットワーク切断時はUDPソケットをクローズ
      if (udpManager.ntpSocketOpen) {
        Serial.println("Closing NTP UDP socket due to network disconnection");
        ntpUdp.stop();
        udpManager.ntpSocketOpen = false;
        networkMonitor.ntpServerActive = false;
      }
    }
  }
  
  // ソケットエラーのリセット（成功時）
  if (udpManager.ntpSocketOpen && udpManager.socketErrors > 0) {
    udpManager.socketErrors = 0;
  }
}

// NTPパケットの基本処理（実装はタスク4で完成）
void processNtpRequests() {
  if (!udpManager.ntpSocketOpen || !networkMonitor.ntpServerActive) {
    return;
  }
  
  int packetSize = ntpUdp.parsePacket();
  if (packetSize >= NTP_PACKET_SIZE) {
    Serial.print("Received NTP request from ");
    Serial.print(ntpUdp.remoteIP());
    Serial.print(":");
    Serial.println(ntpUdp.remotePort());
    
    // NTPパケットを読み取り
    ntpUdp.read(ntpPacketBuffer, NTP_PACKET_SIZE);
    
    // 簡易NTP応答（タスク4で正式実装）
    // 現在はテスト目的で簡略化した応答を返す
    memset(ntpPacketBuffer, 0, NTP_PACKET_SIZE);
    ntpPacketBuffer[0] = 0b00100100; // LI=0, VN=4, Mode=4 (server)
    ntpPacketBuffer[1] = getNtpStratum(); // Stratum
    
    // 時刻スタンプの簡易実装（本格的な実装はタスク4で）
    unsigned long currentTime = getHighPrecisionTime() / 1000; // 秒単位
    // NTPタイムスタンプは1900年からの秒数
    unsigned long ntpTime = currentTime + 2208988800UL;
    
    // 送信タイムスタンプ（バイト位置40-47）
    ntpPacketBuffer[40] = (ntpTime >> 24) & 0xFF;
    ntpPacketBuffer[41] = (ntpTime >> 16) & 0xFF;
    ntpPacketBuffer[42] = (ntpTime >> 8) & 0xFF;
    ntpPacketBuffer[43] = ntpTime & 0xFF;
    
    // 応答送信
    ntpUdp.beginPacket(ntpUdp.remoteIP(), ntpUdp.remotePort());
    ntpUdp.write(ntpPacketBuffer, NTP_PACKET_SIZE);
    ntpUdp.endPacket();
    
    Serial.println("NTP response sent");
  }
}

void displayInfo(GpsSummaryData gpsSummaryData)
{
  char dateTimechr[20];
  sprintf(dateTimechr, "%04d/%02d/%02d %02d:%02d:%02d",
          gpsSummaryData.year, gpsSummaryData.month, gpsSummaryData.day,
          gpsSummaryData.hour, gpsSummaryData.min, gpsSummaryData.sec);

  char poschr[100];
  sprintf(poschr, "Lat: %7.4f Long:  %7.4f Height above MSL:  %6.2f m",
          gpsSummaryData.latitude / 10000000.0, gpsSummaryData.longitude / 10000000.0, gpsSummaryData.altitude / 1000.0);

  display.clearDisplay();

  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("DateTime:");
  display.setCursor(0, 10);
  display.println(dateTimechr);
  display.setCursor(0, 20);
  display.println("Position:");
  display.setCursor(0, 30);
  display.println(poschr);
  display.display(); // Show initial text

  delay(1000);
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
  Wire1.setSDA(GPS_SDA_PIN);
  Wire1.setSCL(GPS_SCL_PIN);
  Wire1.begin();
  Serial.println("Wire1 begin");

  if (myGNSS.begin(Wire1) == false) // Connect to the u-blox module using Wire port
  {
    Serial.println(F("u-blox GNSS not detected at default I2C address. Please check wiring."));
    Serial.println(F("Continuing without GPS for debugging..."));
    analogWrite(LED_ERROR_PIN, 255);
    gpsConnected = false;
    return; // エラーで停止せず、続行する
  }
  
  Serial.println("GPS module connected successfully!");
  gpsConnected = true;
  
  // GPS監視の初期化
  gpsMonitor.lastValidTime = millis();
  gpsMonitor.lastPpsTime = millis();

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
  // Only use once, then disable
  // rtc.set(0, 45, 10, 1, 29, 12, 24);
  //  RTCLib::set(byte second, byte minute, byte hour (0-23:24-hr mode only), byte dayOfWeek (Sun = 1, Sat = 7), byte dayOfMonth (1-12), byte month, byte year)

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
  Serial.begin(9600);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Start");

  // Button for display
  pinMode(BTN_DISPLAY_PIN, INPUT_PULLUP);

  // LED for PPS
  pinMode(LED_PPS_PIN, OUTPUT);
  pinMode(LED_ONBOARD_PIN, OUTPUT);

  // LED for Error
  pinMode(LED_ERROR_PIN, OUTPUT);

  // I2C for OLED and RTC
  Wire.begin();

  // OLED setup
  display.begin(SH1106_SWITCHCAPVCC, SCREEN_ADDRESS, false);
  display.clearDisplay();
  display.display();

  // RTC setup
  setupRtc();

  // You can use Ethernet.init(pin) to configure the CS pin
  Ethernet.init(17);

  // W5500 イーサネット初期化（堅牢なエラーハンドリング付き）
  Serial.println("Initializing W5500 Ethernet module...");
  
  // ハードウェア検出
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("ERROR: W5500 Ethernet hardware not found");
    Serial.println("Continuing without Ethernet (GPS-only mode)");
    analogWrite(LED_ERROR_PIN, 255);
    networkMonitor.isConnected = false;
  } else {
    Serial.println("W5500 hardware detected");
    
    // DHCP試行
    Serial.println("Attempting DHCP configuration...");
    if (Ethernet.begin(mac) == 0) {
      Serial.println("DHCP failed, trying static IP fallback");
      
      // 静的IP設定のフォールバック（例：192.168.1.100）
      IPAddress ip(192, 168, 1, 100);
      IPAddress gateway(192, 168, 1, 1);
      IPAddress subnet(255, 255, 255, 0);
      IPAddress dns(8, 8, 8, 8);
      
      Ethernet.begin(mac, ip, dns, gateway, subnet);
      Serial.println("Using static IP configuration");
      networkMonitor.dhcpActive = false;
    } else {
      Serial.println("DHCP configuration successful");
      networkMonitor.dhcpActive = true;
    }
    
    // 接続確認
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("WARNING: Ethernet cable not connected");
      networkMonitor.isConnected = false;
    } else {
      networkMonitor.isConnected = true;
      digitalWrite(LED_ONBOARD_PIN, HIGH); // ネットワークLED点灯
      
      Serial.print("Ethernet initialized successfully");
      Serial.print(" - IP: ");
      Serial.print(Ethernet.localIP());
      Serial.print(", Gateway: ");
      Serial.print(Ethernet.gatewayIP());
      Serial.print(", DNS: ");
      Serial.println(Ethernet.dnsServerIP());
      
      // ネットワーク監視の初期化
      networkMonitor.lastLinkCheck = millis();
      networkMonitor.reconnectAttempts = 0;
      
      // UDPソケット管理の初期化
      udpManager.lastSocketCheck = millis();
    }
  }

  // Webサーバーを起動
  server.begin();

  // setup GPS
  setupGps();

  // GPS PPS
  pinMode(GPS_PPS_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(GPS_PPS_PIN), trigerPps, FALLING);
}

int displayCount = 0;
unsigned long ledOffTime = 0;

// PPS信号とGPS時刻同期処理
void processPpsSync() {
  if (ppsReceived && gpsConnected) {
    ppsReceived = false; // フラグをリセット
    
    // GPS時刻データを取得
    GpsSummaryData gpsData = gpsClient.getGpsSummaryData();
    
    if (gpsData.timeValid && gpsData.dateValid) {
      // Unix timestampに変換
      struct tm timeinfo = {0};
      timeinfo.tm_year = gpsData.year - 1900;
      timeinfo.tm_mon = gpsData.month - 1;
      timeinfo.tm_mday = gpsData.day;
      timeinfo.tm_hour = gpsData.hour;
      timeinfo.tm_min = gpsData.min;
      timeinfo.tm_sec = gpsData.sec;
      
      timeSync.gpsTime = mktime(&timeinfo);
      timeSync.ppsTime = ppsTimestamp;
      timeSync.synchronized = true;
      
      // RTCと同期
      rtc.set(gpsData.sec, gpsData.min, gpsData.hour, 1, 
              gpsData.day, gpsData.month, gpsData.year % 100);
      timeSync.rtcTime = timeSync.gpsTime;
      
      // 精度計算（PPS信号による高精度化）
      timeSync.accuracy = 0.001; // 1ms精度
      
#if defined(DEBUG_CONSOLE_PPS)
      Serial.print("PPS-GPS sync: ");
      Serial.print(gpsData.hour);
      Serial.print(":");
      Serial.print(gpsData.min);
      Serial.print(":");
      Serial.print(gpsData.sec);
      Serial.print(".");
      Serial.print(gpsData.msec);
      Serial.print(" PPS count: ");
      Serial.println(ppsCount);
#endif
    }
  }
}

void loop()
{
  // GPS信号監視とフォールバック管理
  monitorGpsSignal();
  
  if (gpsConnected) {
    myGNSS.checkUblox();     // Check for the arrival of new data and process it.
    myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.
    
    // PPS信号処理
    processPpsSync();
  }
  
  // LED管理（非ブロッキング）
  if (ledOffTime == 0 && digitalRead(LED_PPS_PIN)) {
    ledOffTime = millis() + 50; // 50ms後にLEDを消灯
  }
  if (ledOffTime > 0 && millis() > ledOffTime) {
    analogWrite(LED_PPS_PIN, 0);
    ledOffTime = 0;
  }

  webServer.server(Serial, server, gpsClient.getUbxNavSatData_t(), gpsClient.getGpsSummaryData());

  if (digitalRead(BTN_DISPLAY_PIN) == LOW)
  {
    Serial.println("Button Display");
    displayCount = 1;
  }

  if (micros() - lastPps > 1000 && displayCount > 0)
  {
    if (displayCount < 10)
    {
      displayInfo(gpsClient.getGpsSummaryData());
      displayCount++;
    }
    else
    {
      displayCount = 0;
      display.clearDisplay();
      display.display();
    }
  }

  // ネットワーク接続監視と自動復旧
  monitorNetworkConnection();
  attemptNetworkReconnection();
  
  // UDPソケット管理とNTPリクエスト処理
  manageUdpSockets();
  processNtpRequests();

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