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