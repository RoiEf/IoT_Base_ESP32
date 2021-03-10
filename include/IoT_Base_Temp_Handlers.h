#pragma once

#include <Arduino.h>
#include <DallasTemperature.h>  // library for the temp sensor
#include <ESPAsyncWebServer.h>
#include <OneWire.h>  // library for the temp sensor
// TIMER
#include <dwd.hpp>

// #include <IoT_Base_defaults.h>
// #include <Preferences.h>  // flash memory
#define TEMP_SENSE 22  // DS1820 One wire

// #include "ArduinoJson.h"
// #include "AsyncJson.h"

// extern String webPassword;
// extern Preferences NVS;

// Temp one wire stuff
OneWire TempSensors(TEMP_SENSE);           // creating oneWire bus object on pin oneWirePin
DallasTemperature sensors1(&TempSensors);  // initialising temp sensors on the oneWire bus object
                                           // DeviceAddress Thermometer;
float currentTemp = 333;
bool tempMutex = false;
bool tempratureTreadFlag = true;
bool tempratureTreadFinish = false;
TaskHandle_t tempratureTread;

void tempratureHandler(AsyncWebServerRequest *request) {
    String response = "{\"temprature\":";

    while (tempMutex)
        ;
    tempMutex = true;
    response += currentTemp;
    tempMutex = false;
    response += "}";
    Serial.println("/temprature");
    Serial.print("response: ");
    Serial.println(response);

    request->send(200, "application/json", response);
}

void tempratureTreadFunc(void *pvParameters) {
    const TickType_t xDelay = 100 / portTICK_PERIOD_MS;
    float readTemptature = 444;
    TIMER tempThreadTimer(10000);

    tempThreadTimer.resetTimer();
    while (tempratureTreadFlag) {
        tempratureTreadFinish = 1;
        if (tempThreadTimer.checkInterval()) {
            sensors1.requestTemperatures();
            tempThreadTimer.resetTimer();
            vTaskDelay(xDelay);
            readTemptature = sensors1.getTempCByIndex(0);
            while (tempMutex)
                ;
            tempMutex = true;
            currentTemp = readTemptature;
            tempMutex = false;
            Serial.print("Temprature: ");
            Serial.println(currentTemp);
        }

        tempratureTreadFinish = 0;
        vTaskDelay(xDelay * 10);
    }

    vTaskDelete(NULL);
}