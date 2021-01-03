

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

AsyncWebServer server(80);

const char* DIR_PARAM = "dir";
const char* TIME_PARAM = "time";
const char* RAMP_PARAM = "ramp";
const char* SPEED_PARAM = "speed";

const unsigned long MAX_RANGE = 1023;

// L298 pins
// D2
const int EN_PIN = 4;
// D1
const int L1_PIN = 5;
// D0
const int L2_PIN = 16;

unsigned long stop_time = 0;
unsigned long start_time = 0;
unsigned long total_ramp_time = 0;
unsigned long speed = 0;

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void stop() {
    analogWrite(EN_PIN, 0);
    digitalWrite(L1_PIN, LOW);
    digitalWrite(L2_PIN, LOW);
    stop_time = 0;
}

void setup() {

    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    analogWriteRange(MAX_RANGE);
    stop();
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
    // Additional optional args:
    // speed=<0.0-1.0 fraction of max speed> defaults to 1.0
    // ramp=<milliseconds to ramp up to speed> defaults to 0
    server.on("/set", HTTP_GET, [] (AsyncWebServerRequest *request) {
        String param;
        long dir = 0;
        long on_time = 0;
        stop_time = 0;
        total_ramp_time = 0;
        speed = MAX_RANGE;
        if (request->hasParam(DIR_PARAM)) {
            param = request->getParam(DIR_PARAM)->value();
            dir = param.toInt();
        }
        if (request->hasParam(TIME_PARAM)) {
            param = request->getParam(TIME_PARAM)->value();
            on_time = param.toInt();
        }
        if (request->hasParam(RAMP_PARAM)) {
            param = request->getParam(RAMP_PARAM)->value();
            total_ramp_time = max(0l, param.toInt());
        }
        if (request->hasParam(SPEED_PARAM)) {
            param = request->getParam(SPEED_PARAM)->value();
            speed = param.toDouble() * MAX_RANGE;
            speed = max(0ul, speed);
            speed = min(MAX_RANGE, speed);
        }

        if (dir < 1 || dir > 2 || on_time == 0) {
          stop();
          Serial.println("Stop cmd");
          request->send(200, "text/plain", "Set - Stop");
        } else {

          digitalWrite(L1_PIN, dir == 1 ? HIGH : LOW);
          digitalWrite(L2_PIN, dir == 2 ? HIGH : LOW);
          start_time = millis();
          if (on_time > 0) {
            stop_time = on_time + start_time;
          }

          String msg = String("Set - Direction: ") + dir + " OnTime: " + on_time + " RampTime: " + total_ramp_time + " Speed: " + speed;
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
  if (stop_time != 0) {
    float ramp_time = millis() - start_time;
    float ramp_fraction =  ramp_time / float(total_ramp_time);
    int pwm_val = min(speed, (unsigned long)(ramp_fraction * speed));
    Serial.println(String("R: ") + pwm_val);
    analogWrite(EN_PIN, pwm_val);
  }
  delay(10);
}
