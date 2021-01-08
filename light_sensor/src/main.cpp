

#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

#include <math.h>

#include "secrets.h"

#include <Wire.h>
#include <BH1750.h>

AsyncWebServer server(80);

BH1750 lightMeter;

float lux;

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void setup() {

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Initialize the I2C bus (BH1750 library doesn't do this automatically)
  Wire.begin(D1, D2); /* join i2c bus with SDA=D1 and SCL=D2 of NodeMCU */

  lightMeter.begin(BH1750::ONE_TIME_HIGH_RES_MODE);

  Serial.println(F("BH1750 One-Time Test"));

  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("WiFi Failed!\n");
    return;
  }

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String msg = "light: ";
    request->send(200, "text/plain", msg + lux);
  });


  server.onNotFound(notFound);

  server.begin();
}

void loop() {
  while (!lightMeter.measurementReady(true)) {
    yield();
  }
  lux = lightMeter.readLightLevel();
  // Serial.print("Light: ");
  // Serial.print(lux);
  // Serial.println(" lx");
  lightMeter.configure(BH1750::ONE_TIME_HIGH_RES_MODE);
}
