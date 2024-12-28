#include <Ethernet.h>
#include <Wire.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS
#include <Adafruit_SSD1306.h>
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
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Enter a MAC address for your controller below.
// https://www.hellion.org.uk/cgi-bin/randmac.pl?scope=local&type=unicast
byte mac[] = {
    0x6e, 0xc9, 0x4c, 0x32, 0x3a, 0xf6};

volatile unsigned long lastPps = 0;

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
  display.setTextColor(SSD1306_WHITE);
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

void setupGps()
{
  Wire1.setSDA(GPS_SDA_PIN);
  Wire1.setSCL(GPS_SCL_PIN);
  Wire1.begin();
  Serial.println("Wire1 begin");

  Wire1.beginTransmission(0x42);
  Wire1.endTransmission(false);

  if (myGNSS.begin(Wire1) == false) // Connect to the u-blox module using Wire port
  {
    Serial.println(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing."));
    analogWrite(LED_ERROR_PIN, 255);
    while (1)
      ;
  }

  myGNSS.setI2COutput(COM_TYPE_UBX);                 // Set the I2C port to output both NMEA and UBX messages
  myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT); // Save (only) the communications port settings to flash and BBR

  myGNSS.setAutoNAVSATcallbackPtr([](UBX_NAV_SAT_data_t *ubxDataStruct)
                                  { gpsClient.newNAVSAT(ubxDataStruct); });
  myGNSS.setAutoPVTcallbackPtr([](UBX_NAV_PVT_data_t *ubxDataStruct)
                               { gpsClient.getPVTdata(ubxDataStruct); });
}

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
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
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    analogWrite(LED_ERROR_PIN, 255);
    for (;;)
      ; // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.display();

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
  myGNSS.checkUblox();     // Check for the arrival of new data and process it.
  myGNSS.checkCallbacks(); // Check if any callbacks are waiting to be processed.

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
#endif
}