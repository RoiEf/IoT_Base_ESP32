#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <IoT_Base_defaults.h>
#include <IoT_Types.h>
#include <Preferences.h>  // flash memory
#include <Update.h>       // OTA Update
#include <html.h>

#include "ArduinoJson.h"
#include "AsyncJson.h"

extern String webPassword;
extern Preferences NVS;

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
    StaticJsonDocument<250> data;
    String response;
    bool ignoreJustReset = false;
    MODE cbMode;

    StaticJsonDocument<1024> doc;
    char message[32] = {0};
    char ssid[32] = {0};
    char wifiPassword[63] = {0};
    char device_mode[5] = {0};
    char dhcp[7] = {0};
    bool bDhcp = false;
    ip_convert STA_local_ip;
    ip_convert STA_gateway;
    ip_convert STA_subnet;
    char SSID_IN_Client[32] = {0};
    char Auth_IN_Client[63] = {0};

    bool update = false;
    bool updateStaticIP = false;
    bool updateDHCP = false;
    bool updateDeviceMode = false;
    bool updateSSIDinSTA = false;

    if (json.is<JsonArray>()) {
        data = json.as<JsonArray>();
    } else if (json.is<JsonObject>()) {
        data = json.as<JsonObject>();
    }

    // Serial.print("JsonData: ");
    // String dataStr;
    // serializeJson(data, dataStr);
    // Serial.println(dataStr);

    NVS.begin(NVS_STRING, false);
    if (data["cmd"] == "update") {
        update = true;
        strlcpy(ssid, data["ssid"] | "iot_base", 32);
        strlcpy(wifiPassword, data["wifiPassword"] | "", 63);
        NVS.putString("ssid", ssid);
        NVS.putString("wifiPassword", wifiPassword);

    } else if (data["cmd"] == "updateDHCP") {
        updateDHCP = true;
        if (data["dhcp"]) {
            strlcpy(dhcp, data["dhcpMode"] | "DHCP", 16);
            NVS.putBool("STA_Static", false);
        } else {
            strlcpy(dhcp, data["dhcpMode"] | "STATIC", 16);
            NVS.putBool("STA_Static", true);
        }
        // NVS.putString("dhcpMode", dhcp);

    } else if (data["cmd"] == "updateStaticIP") {
        updateStaticIP = true;

        STA_local_ip.ip.octecs[0] = data["ip4"].as<unsigned char>();
        STA_local_ip.ip.octecs[1] = data["ip3"].as<unsigned char>();
        STA_local_ip.ip.octecs[2] = data["ip2"].as<unsigned char>();
        STA_local_ip.ip.octecs[3] = data["ip1"].as<unsigned char>();
        STA_subnet.ip.octecs[0] = data["sm4"].as<unsigned char>();
        STA_subnet.ip.octecs[1] = data["sm3"].as<unsigned char>();
        STA_subnet.ip.octecs[2] = data["sm2"].as<unsigned char>();
        STA_subnet.ip.octecs[3] = data["sm1"].as<unsigned char>();
        STA_gateway.ip.octecs[0] = data["dg4"].as<unsigned char>();
        STA_gateway.ip.octecs[1] = data["dg3"].as<unsigned char>();
        STA_gateway.ip.octecs[2] = data["dg2"].as<unsigned char>();
        STA_gateway.ip.octecs[3] = data["dg1"].as<unsigned char>();
        NVS.putUInt("STA_local_ip", STA_local_ip.ip.decimal);
        NVS.putUInt("STA_subnet", STA_subnet.ip.decimal);
        NVS.putUInt("STA_gateway", STA_gateway.ip.decimal);

    } else if (data["cmd"] == "updateDeviceMode") {
        updateDeviceMode = true;
        if (data["wifiAP"]) {
            strlcpy(device_mode, data["device_mode"] | "AP", 5);
            cbMode = AP;
        } else {
            strlcpy(device_mode, data["device_mode"] | "STA", 5);
            cbMode = DEVICE;
        }
        NVS.putUChar("mode", cbMode);
        // NVS.putString("mode", device_mode);
        sprintf(message, "Device Mode Update sucess");

    } else if (data["cmd"] == "updateSSIDinSTA") {
        updateSSIDinSTA = true;
        strlcpy(SSID_IN_Client, data["SSID_IN_Client"] | "", 32);
        strlcpy(Auth_IN_Client, data["Auth_IN_Client"] | "", 63);
        NVS.putString("STA_SSID", SSID_IN_Client);
        NVS.putString("STA_password", Auth_IN_Client);

        sprintf(message, "SSIDinSTA Update sucess");
    } else {
        Serial.println("/wifi accsessd");
        ignoreJustReset = true;
    }

    String str;
    if (!update) {
        str = NVS.getString("ssid", "base_iot");
        str.toCharArray(ssid, str.length() + 1);
        str = NVS.getString("wifiPassword", "");
        str.toCharArray(wifiPassword, str.length() + 1);
    }

    if (!updateDHCP) {
        bDhcp = NVS.getBool("STA_Static", false);
        if (bDhcp) {
            strlcpy(dhcp, "STATIC", 7);
        } else {
            strlcpy(dhcp, "DHCP", 7);
        }
    }

    if (!updateStaticIP) {
        STA_local_ip.ip.decimal = NVS.getUInt("STA_local_ip", 3232235777);
        STA_subnet.ip.decimal = NVS.getUInt("STA_subnet", 4294967040);
        STA_gateway.ip.decimal = NVS.getUInt("STA_gateway", 3232235777);
    }

    if (!updateDeviceMode) {
        cbMode = (MODE)NVS.getUChar("mode", MODE::AP);
        if (cbMode == AP)
            strlcpy(device_mode, "AP", 5);
        else
            strlcpy(device_mode, "STA", 5);
    }

    if (!updateSSIDinSTA) {
        str = NVS.getString("STA_SSID", "");
        str.toCharArray(SSID_IN_Client, str.length() + 1);
        str = NVS.getString("STA_password", "");
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
    doc["ip1"] = STA_local_ip.ip.octecs[3];
    doc["ip2"] = STA_local_ip.ip.octecs[2];
    doc["ip3"] = STA_local_ip.ip.octecs[1];
    doc["ip4"] = STA_local_ip.ip.octecs[0];
    doc["sm1"] = STA_subnet.ip.octecs[3];
    doc["sm2"] = STA_subnet.ip.octecs[2];
    doc["sm3"] = STA_subnet.ip.octecs[1];
    doc["sm4"] = STA_subnet.ip.octecs[0];
    doc["dg1"] = STA_gateway.ip.octecs[3];
    doc["dg2"] = STA_gateway.ip.octecs[2];
    doc["dg3"] = STA_gateway.ip.octecs[1];
    doc["dg4"] = STA_gateway.ip.octecs[0];
    doc["SSID_IN_Client"] = SSID_IN_Client;
    doc["Auth_IN_Client"] = Auth_IN_Client;

    serializeJson(doc, response);
    Serial.print("response: ");
    Serial.println(response);
    request->send(200, "application/json", response);
});

AsyncCallbackJsonWebHandler *scanWifiHandler = new AsyncCallbackJsonWebHandler("/wifi/scan", [](AsyncWebServerRequest *request, JsonVariant &json) {
    StaticJsonDocument<100> data;
    // StaticJsonDocument<1024> doc;
    String response;
    char message[32] = {0};

    if (json.is<JsonArray>()) {
        data = json.as<JsonArray>();
    } else if (json.is<JsonObject>()) {
        data = json.as<JsonObject>();
    }
    if (data["userName"] == "admin" && data["password"] == webPassword) {
        Serial.println("scan start");

        // WiFi.scanNetworks will return the number of networks found
        int n = WiFi.scanNetworks();
        // if (n > 15) n = 15;

        Serial.println("scan done");
        response = "{\"message\":\"";
        if (n == 0) {
            sprintf(message, "no networks found");
            Serial.println("no networks found");
            response += message;
            response += "\"";

        } else {
            // const size_t CAPACITY = JSON_ARRAY_SIZE(15);
            // // allocate the memory for the document
            // StaticJsonDocument<CAPACITY> arrDoc;
            // // create an empty array
            // JsonArray array = arrDoc.to<JsonArray>();

            sprintf(message, "%d Networks found", n);
            Serial.print(n);
            Serial.println(" networks found");
            response += message;
            response += "\"";
            response += ",\"networks\":[";
            for (int i = 0; i < n; ++i) {
                response += "{";
                response += "\"SSID\":\"";
                response += String(WiFi.SSID(i));
                response += "\",\"auth\":";
                response += String((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "false" : "true");
                response += ",\"signal\":\"";
                response += String(WiFi.RSSI(i));
                response += "\"";
                response += "},";
            }
            int length = response.length();
            response.remove(length - 1);
            response += "]";
        }
        response += "}";

        // doc["message"] = message;
        // doc["networks"] = response;

        // serializeJson(doc, response);
        Serial.print("/wifi/scan response: ");
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
