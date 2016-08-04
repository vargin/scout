#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include "halfduplexspi.h"
#include <radio.h>

MDNSResponder mdns;
Radio radio;

const uint8_t pir_pin = 13;

const char* ssid = "";
const char* password = "";

const uint8_t txPipe[5] = {0x71, 0xCD, 0xAB, 0xCD, 0xAB};
const uint8_t rxPipe[5] = {0x7C, 0x68, 0x52, 0x4d, 0x54};
uint8_t rxData[6] = {"-----"};
uint8_t data[6] = {"_PONG"};
const uint32_t timeoutPeriod = 3000;

ESP8266WebServer server(80);

void handleRoot() {
  int read = analogRead(A0);
  String response = "<html><head><title>Sensor</title></head><body><p><b>";
  response += String(read);
  response += "</b></p></body></html>";
  server.send(200, "text/html", response);
}

void setup() {
  //pinMode(A0, INPUT);

  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  if (mdns.begin("azesp", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }

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
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);

  server.begin();
  Serial.println("HTTP server started");

  uint8_t result, registerAddress, registerIndex;

  if (radio.setup()) {
    Serial.println("nRF24L01+ is set up and ready!");
  } else {
    Serial.println("nRF24L01+ DOES NOT respond!");
  }

  radio.setChannel(1);
  radio.setOutputPower(OutputPower::POWER_HIGH);

  if (radio.setDataRate(DataRate::RATE_250KBPS)) {
    Serial.println("True + Module!");
  } else {
    Serial.println("Panic!!!! It's not true + module");
  }

  radio.setAutoAck(1);
  radio.setRetries(2, 15);
  radio.openWritingPipe(txPipe);
  radio.openReadingPipe(rxPipe);
  radio.startListening();

  radio.powerUp();
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();

  //int read = analogRead(A0);
  //Serial.println("+");

  Serial.println(WiFi.localIP());

  if (radio.available()) {
    radio.read(&rxData, 6);

    Serial.println("Message received: ");
    Serial.println((const char *)rxData);

    radio.openWritingPipe(rxPipe);
    radio.openReadingPipe(txPipe);
    radio.stopListening();

    data[0] = rxData[0];

    if (!radio.writeBlocking(&data, 6, timeoutPeriod)) {
      Serial.println("Message timed out!");
    } else {
      Serial.println("Message successfully sent!");
    }

    radio.openWritingPipe(txPipe);
    radio.openReadingPipe(rxPipe);
    radio.startListening();
  } else {
    Serial.println("No data is received!");
  }

  delay(1000);
}
