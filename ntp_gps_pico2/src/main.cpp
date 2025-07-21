#include <Ethernet.h>
#include <Wire.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include <uRTCLib.h>
#include <WebServer.h>
#include <Gps_Client.h>

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
WebServer webServer;
GpsClient gpsClient(Serial);
Adafruit_SH1106 display(OLED_RESET);
uRTCLib rtc;
byte rtcModel = URTCLIB_MODEL_DS3231;

// Enter a MAC address for your controller below.
// https://www.hellion.org.uk/cgi-bin/randmac.pl?scope=local&type=unicast
byte mac[] = {
    0x6e, 0xc9, 0x4c, 0x32, 0x3a, 0xf6};

volatile unsigned long lastPps = 0;
bool gpsConnected = false;

void trigerPps()
{
#if defined(DEBUG_CONSOLE_PPS)
  Serial.println("PPS");
  Serial.println(micros() - lastPps);
#endif
  lastPps = micros();
  analogWrite(LED_ONBOARD_PIN, 255);
  analogWrite(LED_PPS_PIN, 100);
  delay(50);
  analogWrite(LED_ONBOARD_PIN, 0);
  analogWrite(LED_PPS_PIN, 0);
}

void printEtherStatus()
{
  switch (Ethernet.maintain())
  {
  case 1:
    // renewed fail
    Serial.println("Error: renewed fail");
    analogWrite(LED_ERROR_PIN, 255);
    break;

  case 2:
    // renewed success
    Serial.println("Renewed success");
    // print your local IP address:
    Serial.print("My IP address: ");
    Serial.println(Ethernet.localIP());
    break;

  case 3:
    // rebind fail
    Serial.println("Error: rebind fail");
    analogWrite(LED_ERROR_PIN, 255);
    break;

  case 4:
    // rebind success
    Serial.println("Rebind success");
    // print your local IP address:
    Serial.print("My IP address: ");
    Serial.println(Ethernet.localIP());
    break;

  default:
    // nothing happened
    break;
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

  // start the Ethernet connection:
  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0)
  {
    analogWrite(LED_ERROR_PIN, 255);
    Serial.println("Failed to configure Ethernet using DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware)
    {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    }
    else if (Ethernet.linkStatus() == LinkOFF)
    {
      Serial.println("Ethernet cable is not connected.");
    }
    // no point in carrying on, so do nothing forevermore:
    for (;;)
      ;
  }
  // print your local IP address:
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());

  // Webサーバーを起動
  server.begin();

  // setup GPS
  setupGps();

  // GPS PPS
  pinMode(GPS_PPS_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(GPS_PPS_PIN), trigerPps, FALLING);
}

int displayCount = 0;

void loop()
{
  if (gpsConnected) {
    myGNSS.checkUblox();     // Check for the arrival of new data and process it.
    myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.
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

#if defined(DEBUG_CONSOLE_GPS)
  printEtherStatus();

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