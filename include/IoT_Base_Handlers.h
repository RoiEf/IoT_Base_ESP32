#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <IoT_Base_defaults.h>
#include <Preferences.h>  // flash memory
#include <Update.h>       // OTA Update
#include <html.h>

#include "ArduinoJson.h"
#include "AsyncJson.h"

extern String webPassword;
extern Preferences NVS;

char message[32] = {0};
char ssid[64] = {0};
char wifiPassword[64] = {0};
char device_mode[32] = {0};
char dhcp[16] = {0};
int ip1 = 0;
int ip2 = 0;
int ip3 = 0;
int ip4 = 0;
int sm1 = 0;
int sm2 = 0;
int sm3 = 0;
int sm4 = 0;
int dg1 = 0;
int dg2 = 0;
int dg3 = 0;
int dg4 = 0;
char SSID_IN_Client[64] = {0};
char Auth_IN_Client[64] = {0};

void handleRoot(AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", htmlData);
}

AsyncCallbackJsonWebHandler *loginHandler = new AsyncCallbackJsonWebHandler("/login", [](AsyncWebServerRequest *request, JsonVariant &json) {
    StaticJsonDocument<200> data;
    if (json.is<JsonArray>()) {
        data = json.as<JsonArray>();
    } else if (json.is<JsonObject>()) {
        data = json.as<JsonObject>();
    }
    String response;
    if (data["userName"] == "admin" && data["password"] == webPassword) {
        response = "{\"message\": \"Auth sucess\"}";
    } else {
        response = "{\"message\": \"Auth Faild\"}";
    }

    request->send(200, "application/json", response);
    Serial.print("response from /login: ");
    Serial.println(response);
});

AsyncCallbackJsonWebHandler *adminHandler = new AsyncCallbackJsonWebHandler("/updates/password", [](AsyncWebServerRequest *request, JsonVariant &json) {
    StaticJsonDocument<200> data;
    if (json.is<JsonArray>()) {
        data = json.as<JsonArray>();
    } else if (json.is<JsonObject>()) {
        data = json.as<JsonObject>();
    }
    if (data["userName"] == "admin" && data["password"] == webPassword) {
        if (data["password1"] == data["password2"]) {
            const char *buff = data["password1"];
            NVS.begin(NVS_STRING, false);
            NVS.putString("webPassword", buff);
            if (NVS.getBool("justReset", true))
                NVS.putBool("justReset", false);

            NVS.end();
            webPassword = buff;
            Serial.print("response from /updates/password: ");
            Serial.println(buff);

            request->send(200, "application/json", "{\"message\": \"Password Update sucess\"}");
        } else {
            request->send(200, "application/json", "{\"message\": \"Password Update Faild\"}");
        }

    } else {
        request->send(401, "application/json", "{\"message\": \"Auth Faild\"}");
    }
});

AsyncCallbackJsonWebHandler *wifiHandler = new AsyncCallbackJsonWebHandler("/wifi", [](AsyncWebServerRequest *request, JsonVariant &json) {
    StaticJsonDocument<200> data;
    String response;
    bool ignoreJustReset = false;

    StaticJsonDocument<1024> doc;
    // JsonObject jObj = doc.as<JsonObject>();

    if (json.is<JsonArray>()) {
        data = json.as<JsonArray>();
    } else if (json.is<JsonObject>()) {
        data = json.as<JsonObject>();
    }

    NVS.begin(NVS_STRING, false);
    if (data["cmd"] == "update") {
        strlcpy(ssid, data["ssid"] | "iot_base", 64);
        strlcpy(wifiPassword, data["wifiPassword"] | "", 64);
        NVS.putString("ssid", ssid);
        NVS.putString("wifiPassword", wifiPassword);

    } else if (data["cmd"] == "updateDHCP") {
        if (data["dhcp"]) {
            strlcpy(dhcp, data["dhcpMode"] | "DHCP", 16);
        } else {
            strlcpy(dhcp, data["dhcpMode"] | "STATIC", 16);
        }
        NVS.putString("dhcpMode", dhcp);

    } else if (data["cmd"] == "updateStaticIP") {
        NVS.putUInt("ip1", data["ip1"].as<int>());
        NVS.putUInt("ip2", data["ip2"].as<int>());
        NVS.putUInt("ip3", data["ip3"].as<int>());
        NVS.putUInt("ip4", data["ip4"].as<int>());
        NVS.putUInt("sm1", data["sm1"].as<int>());
        NVS.putUInt("sm2", data["sm2"].as<int>());
        NVS.putUInt("sm3", data["sm3"].as<int>());
        NVS.putUInt("sm4", data["sm4"].as<int>());
        NVS.putUInt("dg1", data["dg1"].as<int>());
        NVS.putUInt("dg2", data["dg2"].as<int>());
        NVS.putUInt("dg3", data["dg3"].as<int>());
        NVS.putUInt("dg4", data["dg4"].as<int>());
        ip1 = data["ip1"];
        ip2 = data["ip2"];
        ip3 = data["ip3"];
        ip4 = data["ip4"];
        sm1 = data["sm1"];
        sm2 = data["sm2"];
        sm3 = data["sm3"];
        sm4 = data["sm4"];
        dg1 = data["dg1"];
        dg2 = data["dg2"];
        dg3 = data["dg3"];
        dg4 = data["dg4"];

    } else if (data["cmd"] == "updateDeviceMode") {
        if (data["wifiAP"]) {
            strlcpy(device_mode, data["device_mode"] | "AP", 32);
        } else {
            strlcpy(device_mode, data["device_mode"] | "STA", 32);
        }
        NVS.putString("device_mode", device_mode);
        sprintf(message, "%s", "Device Mode Update sucess");

    } else if (data["cmd"] == "updateSSIDinSTA") {
    } else {
        Serial.println("/wifi accsessd");
        String str;
        ignoreJustReset = true;

        str = NVS.getString("ssid", "base_iot");
        str.toCharArray(ssid, str.length() + 1);
        str = NVS.getString("wifiPassword", "");
        str.toCharArray(wifiPassword, str.length() + 1);
        str = NVS.getString("device_mode", "AP");
        str.toCharArray(device_mode, str.length() + 1);
        str = NVS.getString("dhcpMode", "DHCP");
        str.toCharArray(dhcp, str.length() + 1);
        ip1 = NVS.getInt("ip1", 0);
        ip2 = NVS.getInt("ip2", 0);
        ip3 = NVS.getInt("ip3", 0);
        ip4 = NVS.getInt("ip4", 0);
        sm1 = NVS.getInt("sm1", 0);
        sm2 = NVS.getInt("sm2", 0);
        sm3 = NVS.getInt("sm3", 0);
        sm4 = NVS.getInt("sm4", 0);
        dg1 = NVS.getInt("dg1", 0);
        dg2 = NVS.getInt("dg2", 0);
        dg3 = NVS.getInt("dg3", 0);
        dg4 = NVS.getInt("dg4", 0);
        str = NVS.getString("SSID_IN_Client", "");
        str.toCharArray(SSID_IN_Client, str.length() + 1);
        str = NVS.getString("Auth_IN_Client", "");
        str.toCharArray(Auth_IN_Client, str.length() + 1);
    }
    if (!ignoreJustReset) {
        if (NVS.getBool("justReset", true))
            NVS.putBool("justReset", false);
    }
    NVS.end();

    doc["ssid"] = ssid;

    Serial.print("jObj[ssid] = ssid: ");
    Serial.println(doc["ssid"].as<String>());

    doc["wifiPassword"] = wifiPassword;
    doc["device_mode"] = device_mode;
    doc["dhcp"] = dhcp;
    doc["ip1"] = ip1;
    doc["ip2"] = ip2;
    doc["ip3"] = ip3;
    doc["ip4"] = ip4;
    doc["sm1"] = sm1;
    doc["sm2"] = sm2;
    doc["sm3"] = sm3;
    doc["sm4"] = sm4;
    doc["dg1"] = dg1;
    doc["dg2"] = dg2;
    doc["dg3"] = dg3;
    doc["dg4"] = dg4;
    doc["SSID_IN_Client"] = SSID_IN_Client;
    doc["Auth_IN_Client"] = Auth_IN_Client;

    serializeJson(doc, response);
    Serial.print("response: ");
    Serial.println(response);
    request->send(200, "application/json", response);
});

AsyncCallbackJsonWebHandler *scanWifiHandler = new AsyncCallbackJsonWebHandler("/wifi/scan", [](AsyncWebServerRequest *request, JsonVariant &json) {
    StaticJsonDocument<200> data;
    StaticJsonDocument<1024> doc;
    String response;

    if (json.is<JsonArray>()) {
        data = json.as<JsonArray>();
    } else if (json.is<JsonObject>()) {
        data = json.as<JsonObject>();
    }
    if (data["userName"] == "admin" && data["password"] == webPassword) {
        Serial.println("scan start");

        // WiFi.scanNetworks will return the number of networks found
        int n = WiFi.scanNetworks();
        Serial.println("scan done");
        if (n == 0) {
            sprintf(message, "no networks found");
            Serial.println("no networks found");
        } else {
            sprintf(message, "%d Networks found", n);
            Serial.println(n);
            Serial.println(" networks found");
            for (int i = 0; i < n; ++i) {
                // Print SSID and RSSI for each network found
                // *str += "<li><input type=\"radio\" name=\"netSelect\" id=\"\" value=\"";
                // *str += String(WiFi.SSID(i));
                // *str += "\">";
                // *str += String(WiFi.SSID(i));
                // *str += " | ";
                // *str += String(WiFi.RSSI(i));
                // *str += " | ";
                // *str += String((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "&#x26BF;");
                // *str += "</li>";
            }
        }

        doc["message"] = message;

        serializeJson(doc, response);
        Serial.println(response);
        request->send(200, "application/json", response);
    } else {
        request->send(401, "application/json", "{\"message\": \"Auth Faild\"}");
    }
});

size_t content_len;
void handleDoUpdate(AsyncWebServerRequest *request,
                    const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
    int cmd = U_FLASH;

    if (!index) {
        Serial.println("Update");
        content_len = request->contentLength();

        if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
            Update.printError(Serial);
        }
    }

    if (Update.write(data, len) != len) {
        Update.printError(Serial);
    }

    if (final) {
        AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Please wait while the device reboots");
        response->addHeader("Refresh", "20");
        response->addHeader("Location", "/");
        request->send(response);
        if (!Update.end(true)) {
            Update.printError(Serial);
        } else {
            Serial.println("Update complete");
            Serial.flush();
            ESP.restart();
        }
    }
}
