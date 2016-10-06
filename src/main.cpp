#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoOTA.h>
#include "halfduplexspi.h"
#include <radio.h>

Radio radio;

uint8_t counter = 0;
// Variable is used to count main loop cycles when WiFi is powered up. WiFi is powered up initially and turned off after
// 10 cycles (mostly to allow OTA), then once radio receives a message WiFi is turned on again, does the job and turned
// off again.
uint8_t wifiPowerUpCycles = 0;

const uint8_t txPipe[5] = {0x71, 0xCD, 0xAB, 0xCD, 0xAB};
const uint8_t rxPipe[5] = {0x7C, 0x68, 0x52, 0x4d, 0x54};

const uint8_t data[5] = {80, 79, 78, 71, 0};
uint8_t rxData[5] = {0, 0, 0, 0, 0};

const uint32_t timeoutPeriod = 3000;

void beep(unsigned char delayMs) {
  // Almost any value can be used except 0 and 255.
  analogWrite(15, 800);
  // Wait for a delayMs ms.
  delay(delayMs);
  // 0 turns it off.
  analogWrite(15, 0);

  // Wait for a delayMs ms.
  delay(delayMs);
}

void connectToWiFi() {
  WiFi.forceSleepWake();

  WiFi.mode(WIFI_STA);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
}

void disconnectFromWiFi() {
  WiFi.disconnect(true);
  WiFi.forceSleepBegin();
}

void playAlarm(uint8_t times = 1) {
  for (uint8_t counter = 0; counter < times; counter++) {
    beep(50);
    beep(50);
    beep(200);
    beep(50);
    beep(50);

    delay(300);
  }
}

void setupOTA() {
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
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
}

void setupRadio() {
  if (radio.setup()) {
    Serial.println("nRF24L01+ is set up and ready!");
  } else {
    Serial.println("nRF24L01+ DOES NOT respond!");
  }

  radio.setChannel(1);
  radio.setOutputPower(OutputPower::POWER_HIGH);

  if (radio.setDataRate(DataRate::RATE_250KBPS)) {
    Serial.println("nRF24L01+ is verified!");
  } else {
    Serial.println("This is not nRF24L01+ module!");
  }

  radio.setAutoAck(1);
  radio.setRetries(2, 15);

  radio.openWritingPipe(txPipe);
  radio.openReadingPipe(rxPipe);
  radio.startListening();

  radio.powerUp();
}

void setup() {
  // Allow entire circuitry to power up.
  delay(1000);

  Serial.begin(115200);

  Serial.println("Booting...");

  connectToWiFi();

  setupOTA();

  delay(1000);

  setupRadio();

  Serial.println("Booted!");
}

void loop() {
  if (WiFi.isConnected()) {
    ArduinoOTA.handle();
    Serial.println(WiFi.localIP());

    wifiPowerUpCycles++;

    if (wifiPowerUpCycles >= 30) {
      wifiPowerUpCycles = 0;
      disconnectFromWiFi();
    }
  }

  if (!radio.available()) {
    delay(1000);
    return;
  }

  radio.read(&rxData, 5);

  Serial.println("Message received: ");
  Serial.println((const char *) rxData);

  if (rxData[0] == 80 && rxData[1] == 73 && rxData[2] == 78 && rxData[3] == 71 && rxData[4] == 0) {
    counter++;

    radio.openWritingPipe(txPipe);
    radio.openReadingPipe(rxPipe);
    radio.stopListening();

    for (uint8_t counter = 0; counter < 4; counter++) {
      if (!radio.writeBlocking(&data, 5, timeoutPeriod)) {
        Serial.println("Message has not been sent");
      } else {
        Serial.println("Message has been sent!");
      }

      playAlarm();
    }

    playAlarm(20);

    radio.flush_rx();

    radio.openWritingPipe(txPipe);
    radio.openReadingPipe(rxPipe);
    radio.startListening();
  }

  connectToWiFi();

  Serial.println("Sending HTTP POST request....");

  HTTPClient http;
  http.begin("http://things.ubidots.com/api/v1.6/variables/57efb7e57625424e32b6b474/values");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Auth-Token", UBIDOTS_AUTH_KEY);
  http.addHeader("NULL", "NULL");

  String data = "{\"value\": ";
  data += counter;
  data += "}";

  http.POST(data);
  http.writeToStream(&Serial);
  http.end();

  Serial.println("HTTP POST request has been sent.");

  rxData[0] = rxData[1] = rxData[2] = rxData[3] = rxData[4] = 0;

  disconnectFromWiFi();
}
