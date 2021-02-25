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
            char *buff = data["password1"];
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
    String response, str;
    bool ignoreJustReset = false;

    DynamicJsonDocument doc(1024);
    JsonObject jObj = doc.as<JsonObject>();
    char message[32] = {0};
    char device_mode[32] = {0};
    char ssid[64] = {0};
    char wifiPassword[64] = {0};
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

        // str = NVS.getString("ssid", "base_iot");
        // str.toCharArray(ssid, str.length() + 1);
        // str = NVS.getString("wifiPassword", "");
        // str.toCharArray(wifiPassword, str.length() + 1);
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

    } else if (data["cmd"] == "updateDHCP") {
        if (data["dhcp"]) {
            sprintf(dhcp, "%s", "DHCP");
        } else {
            sprintf(dhcp, "%s", "STATIC");
        }
        NVS.putString("dhcpMode", dhcp);

        str = NVS.getString("ssid", "base_iot");
        str.toCharArray(ssid, str.length() + 1);
        str = NVS.getString("wifiPassword", "");
        str.toCharArray(wifiPassword, str.length() + 1);
        str = NVS.getString("device_mode", "AP");
        str.toCharArray(device_mode, str.length() + 1);
        // str = NVS.getString("dhcpMode", "DHCP");
        // str.toCharArray(dhcp, str.length() + 1);
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

        str = NVS.getString("ssid", "base_iot");
        str.toCharArray(ssid, str.length() + 1);
        str = NVS.getString("wifiPassword", "");
        str.toCharArray(wifiPassword, str.length() + 1);
        str = NVS.getString("device_mode", "AP");
        str.toCharArray(device_mode, str.length() + 1);
        str = NVS.getString("dhcpMode", "DHCP");
        str.toCharArray(dhcp, str.length() + 1);
        str = NVS.getString("SSID_IN_Client", "");
        str.toCharArray(SSID_IN_Client, str.length() + 1);
        str = NVS.getString("Auth_IN_Client", "");
        str.toCharArray(Auth_IN_Client, str.length() + 1);
    } else if (data["cmd"] == "updateDeviceMode") {
        if (data["wifiAP"]) {
            sprintf(device_mode, "%s", "AP");
        } else {
            sprintf(device_mode, "%s", "STA");
        }
        NVS.putString("device_mode", device_mode);
        sprintf(message, "%s", "Device Mode Update sucess");

        str = NVS.getString("ssid", "base_iot");
        str.toCharArray(ssid, str.length() + 1);
        str = NVS.getString("wifiPassword", "");
        str.toCharArray(wifiPassword, str.length() + 1);
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
    } else if (data["cmd"] == "updateSSIDinSTA") {
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
    } else {
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
    if (!ignoreJustReset && NVS.getBool("justReset", true))
        NVS.putBool("justReset", false);
    NVS.end();

    jObj["message"] = message;
    jObj["device_mode"] = device_mode;
    jObj["ssid"] = ssid;
    jObj["wifiPassword"] = wifiPassword;
    jObj["dhcp"] = dhcp;
    jObj["ip1"] = ip1;
    jObj["ip2"] = ip2;
    jObj["ip3"] = ip3;
    jObj["ip4"] = ip4;
    jObj["sm1"] = sm1;
    jObj["sm2"] = sm2;
    jObj["sm3"] = sm3;
    jObj["sm4"] = sm4;
    jObj["dg1"] = dg1;
    jObj["dg2"] = dg2;
    jObj["dg3"] = dg3;
    jObj["dg4"] = dg4;
    jObj["SSID_IN_Client"] = SSID_IN_Client;
    jObj["Auth_IN_Client"] = Auth_IN_Client;
    serializeJson(doc, response);
    Serial.println(response);
    request->send(200, "application/json", response);
});

AsyncCallbackJsonWebHandler *scanWifiHandler = new AsyncCallbackJsonWebHandler("/wifi/scan", [](AsyncWebServerRequest *request, JsonVariant &json) {
    StaticJsonDocument<200> data;
    if (json.is<JsonArray>()) {
        data = json.as<JsonArray>();
    } else if (json.is<JsonObject>()) {
        data = json.as<JsonObject>();
    }
    String response;
    serializeJson(data, response);
    request->send(200, "application/json", response);
    Serial.println(response);
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
