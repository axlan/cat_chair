

#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>

#include <math.h>

#include "secrets.h"

#include <Wire.h>
#include <BH1750.h>

AsyncWebServer server(80);

BH1750 lightMeterA;
BH1750 lightMeterB;

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void setup() {

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Initialize the I2C bus (BH1750 library doesn't do this automatically)
  Wire.begin(D1, D2); /* join i2c bus with SDA=D1 and SCL=D2 of NodeMCU */

  /*
    ADD pin is used to set sensor I2C address. If it has voltage greater or equal to
    0.7VCC voltage (e.g. you've connected it to VCC) the sensor address will be
    0x5C. In other case (if ADD voltage less than 0.7 * VCC) the sensor address will
    be 0x23 (by default).
  */
  lightMeterA.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23);
  lightMeterB.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x5C);

  Serial.println(F("BH1750 One-Time Test"));

  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("WiFi Failed!\n");
    return;
  }

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String msg = "[";
    msg += lightMeterA.readLightLevel();
    msg += ",";
    msg += lightMeterB.readLightLevel();
    msg += "]";
    request->send(200, "application/json", msg);
  });


  server.onNotFound(notFound);

  server.begin();
}

void loop() {
  ArduinoOTA.handle();
}
