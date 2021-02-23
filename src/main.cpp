#include <Arduino.h>
#include <ArduinoJson.h>        // API communications in Device mode
#include <ESPAsyncWebServer.h>  // Web server
#include <ESPmDNS.h>            // Host name resolve
#include <HTTPClient.h>         // API communications in Device mode
#include <Preferences.h>        // flash memory
#include <SPIFFS.h>             // to save a log
#include <Update.h>             //  OTA Update
#include <WiFi.h>               // main WiFi library
#include <esp_int_wdt.h>        //  Watch dog timers. used for hard resetting
#include <esp_task_wdt.h>       //  Watch dog timers. used for hard resetting

// TIMER
#include <dwd.hpp>

// Self created includes
#include <IoT_Base_Handlers.h>
#include <IoT_Base_defaults.h>
#include <IoT_Types.h>

// System control pins
#define MODE_PIN 4    //  Stand alone or networked led's
#define STATUS_LED 5  // orange led
#define RESET_PIN 23  // reset button input

MODE mode = AP;
bool justReset = true;
bool STA_Static = false;

TIMER timer(1000);
int loopI = 0;

Preferences NVS;

/* Put your SSID & Password */
char wifi_ssid[32] = AP_SSID;
char wifi_password[63] = {0};
char Host_Name[32] = AP_HOST;

/* Put IP Address details */
IPAddress local_ip(AP_IP1, AP_IP2, AP_IP3, AP_IP4);
IPAddress gateway(AP_IP1, AP_IP2, AP_IP3, AP_IP4);
IPAddress subnet(AP_SUB1, AP_SUB2, AP_SUB3, AP_SUB4);

AsyncWebServer server(80);
String webPassword = WEB_PASSWORD;
bool runAPIFlag = true;
bool runSenderFlag = false;

// STA variables
TaskHandle_t connTask;
byte connTaskFlag = 0;
byte connTaskFinish = 0;
byte connectedFlag = 0;
byte firstConnect = 1;
WIFI_STATUS wifi_status = DISCONNECTED;

void blink(uint8_t times, unsigned long speed, uint8_t pin);
void toReset(void);
void hard_restart(void);

int scanWifi(String *str);
void connTreadFunc(void *pvParameters);

// size_t content_len;
// void handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final);

void setup() {
    String str;

    Serial.begin(BAUD);

    pinMode(STATUS_LED, OUTPUT);
    pinMode(MODE_PIN, OUTPUT);
    pinMode(RESET_PIN, INPUT_PULLDOWN);
    digitalWrite(STATUS_LED, HIGH);
    digitalWrite(MODE_PIN, HIGH);
    delay(500);

    while (!Serial)
        ;

    Serial.println("Hello IoT Base V0.1");
    toReset();

    NVS.begin(NVS_STRING, false);
    justReset = NVS.getBool("justReset", true);

    if (mode == RESET && !justReset) {
        NVS.clear();
        justReset = true;
        delay(500);
    }

    mode = AP;

    if (!justReset) {
        mode = (MODE)NVS.getUChar("mode", MODE::AP);
        webPassword = NVS.getString("webPassword", "12345");

        if (mode == AP) {
            local_ip = NVS.getUInt("AP_local_ip", 3232235777);  //  192.168.1.1
            gateway = local_ip;
            Serial.println("AP_local_ip: ");
            Serial.println(local_ip);

            subnet = NVS.getUInt("AP_subnet", 4294967040);  //  255.255.255.0

            memset(wifi_ssid, '\0', 32);
            memset(wifi_password, '\0', 63);
            memset(Host_Name, '\0', 32);
            str = NVS.getString("AP_SSID", "IoT_Base");
            str.toCharArray(wifi_ssid, str.length() + 1);
            str = NVS.getString("AP_password", "");
            str.toCharArray(wifi_password, str.length() + 1);
            str = NVS.getString("AP_hostname", "IoT_Base");
            str.toCharArray(Host_Name, str.length() + 1);
        } else {  // mode is STA
            memset(wifi_ssid, '\0', 32);
            memset(wifi_password, '\0', 63);
            memset(Host_Name, '\0', 32);
            str = NVS.getString("STA_SSID", "");
            str.toCharArray(wifi_ssid, str.length() + 1);
            str = NVS.getString("STA_password", "");
            str.toCharArray(wifi_password, str.length() + 1);
            str = NVS.getString("STA_hostname", "IoT_Base");
            str.toCharArray(Host_Name, str.length() + 1);

            STA_Static = NVS.getBool("STA_Static", false);
            if (STA_Static) {
                local_ip = NVS.getUInt("STA_local_ip", 3232235777);  //  192.168.1.1
                gateway = NVS.getUInt("STA_gateway", 3232235777);    //  192.168.1.1
                subnet = NVS.getUInt("STA_subnet", 4294967040);      //  255.255.255.0
            }
        }
    }
    NVS.end();

    if (mode == AP) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(wifi_ssid, wifi_password);
        delay(250);
        WiFi.softAPConfig(local_ip, gateway, subnet);
        Serial.println(WiFi.softAPIP());
        digitalWrite(STATUS_LED, LOW);
    } else {
        // Start connection thread on main core
        Serial.println("Starting connection thread");
        connTaskFlag = 1;
        xTaskCreatePinnedToCore(
            connTreadFunc, /* Function to implement the task */
            "connTask",    /* Name of the task */
            10000,         /* The size of the task stack specified as the number of bytes. Note that this differs from vanilla FreeRTOS. */
            NULL,          /* Task input parameter */
            2,             /* Priority of the task */
            &connTask,     /* Task handle. */
            1);            /* Core where the task should run */

        // WiFi.mode(WIFI_STA);
        // if STA_Static -> WiFi.config(local_ip, gateway, subnet);
        // WiFi.begin(wifi_ssid, wifi_password);
    }
    SPIFFS.begin();

    server.on("/", handleRoot);
    server.addHandler(loginHandler);
    server.addHandler(adminHandler);
    server.addHandler(scanWifiHandler);
    server.addHandler(wifiHandler);
    // server.on("/AP", handleAP);
    // server.on("/STA", handleSTA);
    // server.on("/ModFeat", HTTP_POST, handleModFeat);
    // if (runAPIFlag) server.on("/API", handleAPI);
    //     if (runSenderFlag) ; // run sender thread
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/favicon.ico", "image/x-icon");
    });

    server.on(
        "/updates/firmware", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data,
           size_t len, bool final) { handleDoUpdate(request, filename, index, data, len, final); });

    server.onNotFound(handleRoot);

    server.begin();
    Serial.println("HTTP server started");

    if (!MDNS.begin(Host_Name)) {  //http://IoT_Base.local
        Serial.println("Error setting up MDNS responder!");
    } else {
        Serial.println("Created local DNS");
        MDNS.addService("http", "tcp", 80);
    }

    // Start any sensor

    delay(2000);
}

void loop() {
    if (timer.checkInterval(true))
        Serial.println(loopI++);

    delay(10);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void blink(uint8_t times, unsigned long speed, uint8_t pin) {
    TIMER timer(speed);
    uint8_t counter = 0;
    bool stat = HIGH;

    digitalWrite(pin, stat);
    while (counter < times) {
        if (timer.checkInterval()) {
            timer.resetTimer();

            ++counter;
            if (stat) {
                stat = LOW;
            } else {
                stat = HIGH;
            }
            digitalWrite(pin, stat);
        }
    }
    digitalWrite(pin, HIGH);
}
void toReset(void) {
    TIMER timer1(5000);
    TIMER ledTimer(500);
    bool ledStat = HIGH;

    digitalWrite(STATUS_LED, ledStat);
    while (!timer1.checkInterval()) {
        if (ledTimer.checkInterval()) {
            ledTimer.resetTimer();
            if (ledStat)
                ledStat = LOW;
            else
                ledStat = HIGH;
            digitalWrite(STATUS_LED, ledStat);
        }
        if (!digitalRead(RESET_PIN))
            return;
        delay(5);
    }

    mode = RESET;
    blink(40, 100, STATUS_LED);
}
void hard_restart(void) {
    const TickType_t xDelay = 500 / portTICK_PERIOD_MS;  // 500 Msec

    Serial.println("HARD RESTART");
    Serial.flush();
    vTaskDelay(xDelay);
    esp_task_wdt_init(1, true);
    esp_task_wdt_add(NULL);
    while (true)
        ;
}

// void handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
// int cmd = U_FLASH;

// if (!index) {
//     Serial.println("Update");
//     content_len = request->contentLength();

//     if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
//         Update.printError(Serial);
//     }
// }

// if (Update.write(data, len) != len) {
//     Update.printError(Serial);
// }

// if (final) {
//     AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Please wait while the device reboots");
//     response->addHeader("Refresh", "20");
//     response->addHeader("Location", "/");
//     request->send(response);
//     if (!Update.end(true)) {
//         Update.printError(Serial);
//     } else {
//         Serial.println("Update complete");
//         Serial.flush();
//         ESP.restart();
//     }
// }
// }

int scanWifi(String *str) {
    Serial.println("scan start");

    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0) {
        Serial.println("no networks found");
        *str = "<li>No Network Detected.</li>";
    } else {
        Serial.println(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            *str += "<li><input type=\"radio\" name=\"netSelect\" id=\"\" value=\"";
            *str += String(WiFi.SSID(i));
            *str += "\">";
            *str += String(WiFi.SSID(i));
            *str += " | ";
            *str += String(WiFi.RSSI(i));
            *str += " | ";
            *str += String((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "&#x26BF;");
            *str += "</li>";
        }
    }
    return n;
}
void connTreadFunc(void *pvParameters) {
    const TickType_t xDelay = 1000 / portTICK_PERIOD_MS;
    connTaskFinish = 0;
    // #if DEBUGLEVEL >= 2
    static int i = 0;
    // #endif
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_password);  // Connect to the network
    Serial.println("Wifi.begine Connecting to: ");
    Serial.println(wifi_ssid);
    Serial.println("password: ");
    Serial.println(wifi_password);

    // // blink fast while NOT connected
    // blinkSpeed = 250;
    // DEBUGPRINTLN1("Starting fast blink thread");
    // blinkTaskFlag = 1;
    // xTaskCreatePinnedToCore(
    //     blinkTreadFunc, /* Function to implement the task */
    //     "blinkTask",    /* Name of the task */
    //     10000,          /* The size of the task stack specified as the number of bytes. Note that this differs from vanilla FreeRTOS. */
    //     NULL,           /* Task input parameter */
    //     0,              /* Priority of the task */
    //     &hBlinkTask,    /* Task handle. */
    //     0);             /* Core where the task should run */

    while (connTaskFlag) {
        connTaskFinish = 1;
        if (WiFi.status() == WL_CONNECTED) {
            // blinkTaskFlag = 0;

            wifi_status = CONNECTED;
            if (firstConnect) {
                Serial.println("First Connection established!");
                Serial.println("IP address:\t");
                Serial.println(WiFi.localIP());  // Send the IP address of the ESP32 to the computer
                firstConnect = 0;
            }

            // while (blinkTaskFinish) {
            //     DEBUGPRINT2("STOP 1 blinkTaskFinish: ");
            //     DEBUGPRINTLN2(blinkTaskFinish);
            //     delay(10);
            // }
            // while (blinkState != DELETED) {
            //     DEBUGPRINT2("STOP 1 blinkTaskFinish: ");
            //     DEBUGPRINTLN2(blinkState);
            //     delay(1);
            // }
            // delay(5);
            // DEBUGPRINT2("STOP END blinkTaskFinish: ");
            // DEBUGPRINTLN2(blinkState);
            // DEBUGPRINT2("STOP END blinkTaskFinish: ");
            // DEBUGPRINTLN2(blinkTaskFinish);

            //  light LED constant, CONNECTED
            digitalWrite(STATUS_LED, LOW);
        } else {
            if (wifi_status == CONNECTED) {
                wifi_status = DISCONNECTED;
                digitalWrite(STATUS_LED, HIGH);
            }

            // if (!blinkTaskFlag) {
            //     blinkTaskFlag = 1;
            //     xTaskCreatePinnedToCore(
            //         blinkTreadFunc, /* Function to implement the task */
            //         "blinkTask",    /* Name of the task */
            //         10000,          /* The size of the task stack specified as the number of bytes. Note that this differs from vanilla FreeRTOS. */
            //         NULL,           /* Task input parameter */
            //         0,              /* Priority of the task */
            //         &hBlinkTask,    /* Task handle. */
            //         0);             /* Core where the task should run */
            // }
            Serial.println("WiFi Not connected ");
            Serial.println(++i);
        }

        connTaskFinish = 0;
        vTaskDelay(xDelay);
    }

    vTaskDelete(NULL);
}
