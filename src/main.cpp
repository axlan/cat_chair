

#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

#include "secrets.h"

AsyncWebServer server(80);

const char* DIR_PARAM = "dir";
const char* TIME_PARAM = "time";

// L298 pins
// D2
const int EN_PIN = 4;
// D1
const int L1_PIN = 5;
// D0
const int L2_PIN = 16;

unsigned long stop_time = 0;

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void stop() {
    digitalWrite(EN_PIN, LOW);
    digitalWrite(L1_PIN, LOW);
    digitalWrite(L2_PIN, LOW);
    stop_time = 0;
}

void setup() {

    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);



    pinMode(EN_PIN, OUTPUT);
    pinMode(L1_PIN, OUTPUT);
    pinMode(L2_PIN, OUTPUT);

    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("WiFi Failed!\n");
        return;
    }

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Hello, world");
    });

    // Send a GET request to <IP>/set?dir=<1 or 2>&time=<on time in millies, or -1 for stay on>
    server.on("/set", HTTP_GET, [] (AsyncWebServerRequest *request) {
        String param;
        long dir = 0;
        long on_time = 0;
        stop_time = 0;
        if (request->hasParam(DIR_PARAM)) {
            param = request->getParam(DIR_PARAM)->value();
            dir = param.toInt();
        }
        if (request->hasParam(TIME_PARAM)) {
            param = request->getParam(TIME_PARAM)->value();
            on_time = param.toInt();
        }

        if (dir < 1 || dir > 2 || on_time == 0) {
          stop();
          Serial.println("Stop cmd");
          request->send(200, "text/plain", "Set - Stop");
        } else {

          digitalWrite(L1_PIN, dir == 1 ? HIGH : LOW);
          digitalWrite(L2_PIN, dir == 2 ? HIGH : LOW);
          if (on_time > 0) {
            stop_time = on_time + millis();
          }

          digitalWrite(EN_PIN, HIGH);
          String msg = String("Set - Direction: ") + dir + " OnTime: " + on_time;
          Serial.println(msg);
          request->send(200, "text/plain", msg);
        }
    });

    server.onNotFound(notFound);

    server.begin();
}

void loop() {
  if (stop_time > 0 && stop_time < millis()) {
    stop();
    Serial.println("Timed stop");
  }
  delay(10);
}
