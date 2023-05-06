#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Wire.h>
#include "SSD1306Wire.h"
<<<<<<< HEAD

// OLED definition
SSD1306Wire display(0x3c, SDA, SCL, GEOMETRY_128_32);
const char* AP_SSID = "AutoConnectAP";
const char* AP_PASS = "password";

void setup() {
=======
#include "powerMonitor.h"

const int BUTTON_PIN = 25;

// OLED definition
SSD1306Wire display(0x3c, SDA, SCL, GEOMETRY_128_32);

// Modbus definition
PowerMonitor pm;

// WiFiManager definition
WiFiManager wm;
const char *AP_SSID = "AutoConnectAP";
const char *AP_PASS = "password";

// custom parameters
char alarm_threshold[6] = "1000";
WiFiManagerParameter customAlarmThreshold("alarm_threshold", "Alarm threshold", alarm_threshold, 6);
char reset_energy[2] = "0";
WiFiManagerParameter customResetEnergy("reset_energy", "Reset energy counter (1 is reset)", reset_energy, 2);

// prometheus labels
const String PROMETHEUS_LABLE = "device=\"" + WiFi.macAddress() + "\"";

void handleMetricsRoute() {
    Serial.println("[HTTP] handle route");
    String body = "";
    // voltage
    body += "# HELP ac_power_meter_voltage The AC power meter of voltage. Unit 'V'.\n";
    body += "# TYPE ac_power_meter_voltage counter\n";
    body += "ac_power_meter_voltage{" + PROMETHEUS_LABLE + "} " + String(pm.getVoltage()) + "\n";
    // current
    body += "# HELP ac_power_meter_current The AC power meter of current. Unit 'A'.\n";
    body += "# TYPE ac_power_meter_current counter\n";
    body += "ac_power_meter_current{" + PROMETHEUS_LABLE + "} " + String(pm.getCurrent()) + "\n";
    // power
    body += "# HELP ac_power_meter_power The AC power meter of power. Unit 'W'.\n";
    body += "# TYPE ac_power_meter_power counter\n";
    body += "ac_power_meter_power{" + PROMETHEUS_LABLE + "} " + String(pm.getPower()) + "\n";
    // energy
    body += "# HELP ac_power_meter_energy The AC power meter of energy. Unit 'Wh'.\n";
    body += "# TYPE ac_power_meter_energy counter\n";
    body += "ac_power_meter_energy{" + PROMETHEUS_LABLE + "} " + String(pm.getEnergy()) + "\n";
    // frequency
    body += "# HELP ac_power_meter_frequency The AC power meter of frequency. Unit 'Hz'.\n";
    body += "# TYPE ac_power_meter_frequency counter\n";
    body += "ac_power_meter_frequency{" + PROMETHEUS_LABLE + "} " + String(pm.getFrequency()) + "\n";
    // power factor
    body += "# HELP ac_power_meter_power_factor The AC power meter of power factor. Unit 'percent'.\n";
    body += "# TYPE ac_power_meter_power_factor counter\n";
    body += "ac_power_meter_power_factor{" + PROMETHEUS_LABLE + "} " + String(pm.getPowerFactor()) + "\n";
    // alarm state
    body += "# HELP ac_power_meter_alarm_state The AC power meter of alarm state. Alarmed 1 else 0. \n";
    body += "# TYPE ac_power_meter_alarm_state counter\n";
    body += "ac_power_meter_alarm_state{" + PROMETHEUS_LABLE + "} " + String(pm.getAlarmState()) + "\n";
    // alarm threshold
    body += "# HELP ac_power_meter_alarm_threshold The AC power meter of alarm threshold. Unit 'W'. \n";
    body += "# TYPE ac_power_meter_alarm_threshold counter\n";
    body += "ac_power_meter_alarm_threshold{" + PROMETHEUS_LABLE + "} " + String(pm.getAlarmThreshold()) + "\n";

    wm.server->send(200, "text/plain", body);
}

void bindServerCallback() {
    wm.server->on("/metrics", handleMetricsRoute);
}

void displayWifiInfo() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("AP mode");
        display.clear();
        display.drawString(0, 0, "SSID: " + String(AP_SSID));
        display.drawString(0, 10, "Pass: " + String(AP_PASS));
        display.drawString(0, 20, "IP adrs: 192.168.4.1");
        display.display();
    } else {
        IPAddress ipadr = WiFi.localIP();

        display.clear();
        display.drawString(0, 0, "connected");
        display.drawString(0, 10, "SSID: " + WiFi.SSID());
        display.drawString(0, 20,
                           "IP adrs: " + String(ipadr[0]) + "." + String(ipadr[1]) + "." + String(ipadr[2]) + "." +
                           String(ipadr[3]));
        display.display();
    }
}

void task1(void *pvParameters) {
    while (1) {
        wm.process();
//        Serial.println("[WIFI] task1");
        delay(1);
    }
}

void saveParamCallback() {
    Serial.println("[PARAM] Callback");

    if (atof(alarm_threshold) != atof(customAlarmThreshold.getValue())) {
        strcpy(alarm_threshold, customAlarmThreshold.getValue());

        Serial.println("[UPDATE] alarm threshold: " + (String) pm.getAlarmThreshold());
        pm.setAlarmThreshold(atof(alarm_threshold));
    }

    if (String("1").equals(customResetEnergy.getValue())) {
        customResetEnergy.setValue(reset_energy, 2);
        Serial.println("[Reset] reset energy counter");
        pm.resetEnergy();
    }

}

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    Serial.println("Startup....");

>>>>>>> 9ab0d43 (separate power monitor functions)
    // Initialize the display
    display.init();
    display.setFont(ArialMT_Plain_10);
    display.flipScreenVertically();
<<<<<<< HEAD
//    display.drawString(0, 0, "Hello SSD1306!!");    //(0,0)の位置にHello Worldを表示
//    display.display();   //指定された情報を描画

    // WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
    // it is a good practice to make sure your code sets wifi mode how you want it.

    // put your setup code here, to run once:
    Serial.begin(115200);

    //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wm;

    // reset settings - wipe stored credentials for testing
    // these are stored by the esp library
    // wm.resetSettings();

    display.drawString(0, 0, "SSID: " + String(AP_SSID));
    display.drawString(0, 10, "Pass: " + String(AP_PASS));
    display.drawString(0, 20, "IP adrs: 192.168.4.1");
    display.display();

    bool res;
    res = wm.autoConnect(AP_SSID,AP_PASS); // password protected ap

    if(!res) {
        Serial.println("Failed to connect");
         ESP.restart();
    }
    else {
        //if you get here you have connected to the WiFi
        Serial.println("connected...yeey :)");

        IPAddress ipadr = WiFi.localIP();

        display.clear();
        display.drawString(0, 0, "connected (^^)");
        display.drawString(0, 10, "SSID: " + WiFi.SSID());
        display.drawString(0, 20, "IP adrs: " + (String)ipadr[0] + "." + (String)ipadr[1] + "." + (String)ipadr[2] + "." + (String)ipadr[3]);
        display.display();
    }

}

void loop() {
    // put your main code here, to run repeatedly:
=======

    //setup Modbus
    pm.setup(0x01);

    // setup wifi manager
    wm.setDebugOutput(true);
    wm.setSaveParamsCallback(saveParamCallback);
    wm.setWebServerCallback(bindServerCallback);
    wm.setConnectTimeout(30);
    wm.setConnectRetries(5);

    // setup custom menu
    const char *menuhtml = "<form action='/metrics' method='get'><button>Metrics</button></form><br/>\n";
    wm.setCustomMenuHTML(menuhtml);

    std::vector<const char *> menu = {"custom", "wifi", "info", "param", "close", "sep", "erase", "update", "restart"};
    wm.setMenu(menu);

    // setup custom parameters
    dtostrf(pm.getAlarmThreshold(), -1, 0, alarm_threshold);
    customAlarmThreshold.setValue(alarm_threshold, 6);
    wm.addParameter(&customAlarmThreshold);
    wm.addParameter(&customResetEnergy);

    // start wifi manager
    displayWifiInfo();
    wm.autoConnect(AP_SSID, AP_PASS);
    wm.startWebPortal();

    // setup pinmode
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // setup task
    xTaskCreateUniversal(
            task1,
            "task1",
            8192,
            NULL,
            1,
            NULL,
            APP_CPU_NUM
    );
}

int resetCounter = 0;

void resetWifi() {
    resetCounter++;
    if (resetCounter > 5) {
        display.clear();
        display.drawString(0, 0, "RESET......!!");
        display.display();
        delay(1000);
        wm.resetSettings();
        ESP.restart();
    }
}

bool displayFlag = false;

void loop() {
    if (displayFlag) {
        // get power data
        pm.requestPowerData();

        // display power data
        display.clear();
        display.drawString(0, 0,
                           String(pm.getVoltage()) + "V " + String(pm.getCurrent()) + "A " + String(pm.getPower()) +
                           "W");
        display.drawString(0, 10, String(pm.getPowerFactor()) + "pf " + String(pm.getEnergy()) + "Wh");
        display.drawString(0, 20, String(pm.getFrequency()) + "Hz ALM " + ((pm.getAlarmState()) ? "ALARM" : "OK"));
        display.display();

        displayFlag = false;
    } else {
        displayWifiInfo();
        displayFlag = true;
    }

    // reset wifi if button is pressed
    if (!digitalRead(BUTTON_PIN)) {
        resetWifi();
    }

    delay(1000);
>>>>>>> 9ab0d43 (separate power monitor functions)
}
