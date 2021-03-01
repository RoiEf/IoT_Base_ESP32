#pragma once

#include <Arduino.h>
#include <DallasTemperature.h>  // library for the temp sensor
#include <ESPAsyncWebServer.h>
// #include <IoT_Base_defaults.h>
#include <OneWire.h>  // library for the temp sensor
// #include <Preferences.h>  // flash memory
#define TEMP_SENSE 22  // DS1820 One wire

// #include "ArduinoJson.h"
// #include "AsyncJson.h"

// extern String webPassword;
// extern Preferences NVS;

// Temp one wire stuff
OneWire TempSensors(TEMP_SENSE);           // creating oneWire bus object on pin oneWirePin
DallasTemperature sensors1(&TempSensors);  // initialising temp sensors on the oneWire bus object
DeviceAddress Thermometer;

void tempratureHandler(AsyncWebServerRequest *request) {
    String response = "{\"temprature\":";
    float currentTemp;

    sensors1.requestTemperatures();
    currentTemp = sensors1.getTempC(Thermometer);  // getTempCByIndex(0);
    // currentTemp = round(currentTemp * 10.0) / 10.0;
    response += currentTemp;
    response += "}";
    Serial.println("/temprature");
    Serial.print("response: ");
    Serial.println(response);

    request->send(200, "application/json", response);
}
