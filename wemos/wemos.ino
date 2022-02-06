#include "Arduino.h"

#include <NTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#define PORT 80
#define LAMP_STATES 100

const char* ssid = "";
const char* password = "";

//Parameters
const int switchButton  = D5; //11;
const int zeroCrossPin  = D6; //12;
const int acdPin  = D7; //13;
bool lampOn = true;
int lastRead = -1;
int delay_time;
int lamp_intensity = 0;


ESP8266WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 7200);
int startTimestamp = -1, durationTimestamp = -1, epochStates = -1, lastPower = 0;

void ICACHE_RAM_ATTR interrupt() {
  if (lamp_intensity != 0) {
    delay_time = (100 - (lamp_intensity * 0.5)) * 100;
    delay_time = delay_time < 250 ? 250 : delay_time;
    
    delayMicroseconds(delay_time);
    
    digitalWrite(acdPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(acdPin, LOW);
  }
}

void setup() {


  Serial.begin(115200);
  Serial.println("Begin...");
  
  pinMode(LED_BUILTIN, OUTPUT); // Led on Wemos
  digitalWrite(LED_BUILTIN, LOW); // led On
  
  pinMode(switchButton, INPUT_PULLUP);
  digitalWrite(switchButton, HIGH);

  pinMode(acdPin, OUTPUT);
  pinMode(zeroCrossPin, INPUT_PULLUP);
  
  attachInterrupt(digitalPinToInterrupt(zeroCrossPin), interrupt, RISING);
  

  Serial.println("Connect to wifi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Try to connect...");
    delayMicroseconds(16383); // Max miscroseconds time
  }
  Serial.println("Connected!");
  Serial.println("Start web server...");
  
  server.on("/", HTTP_POST, []() {

    Serial.println("Get request");
    startTimestamp = server.arg("starttimestamp").toInt();
    durationTimestamp  = server.arg("durationtimestamp").toInt();

    epochStates = durationTimestamp / LAMP_STATES;

    timeClient.setTimeOffset(server.arg("gmtoffset").toInt());
    timeClient.update();


    server.send(200, "text/plain", "Timestamp receive");
  });

  server.begin();
  timeClient.begin();
  timeClient.update();

  
  lamp_intensity = 0;
  Serial.println("Listening...");
  digitalWrite(LED_BUILTIN, HIGH); 
}

void loop() {
  server.handleClient();
  int readSwitch = digitalRead(switchButton);
  if (readSwitch != lastRead) {
    Serial.println("Switch");
    lastRead = readSwitch;
    lampOn = !lampOn;
  }

  if (startTimestamp != -1 && timeClient.getEpochTime() >= startTimestamp && lastPower == 0) {
    Serial.println("Ready to start intensity");
    lampOn = true;
  }

  if (lampOn) {
    if (startTimestamp != -1 && timeClient.getEpochTime() >= startTimestamp && startTimestamp + durationTimestamp > timeClient.getEpochTime()) {//Allumer lampe avec intensité


      int intensity = ((timeClient.getEpochTime() - startTimestamp) / epochStates) + 1;
      if (lastPower != intensity) {
        Serial.print("Set intensity : ");
        Serial.println(intensity);
        lamp_intensity = intensity;
        lastPower = intensity;
      }

    } else if (lastPower != 100) {
      startTimestamp = -1;
      lastPower = 100;
      lamp_intensity = 100;
      Serial.println("ON");
    }
  } else if (lastPower != 0) {
    startTimestamp = -1;
    lastPower = 0;
  } else {
    lamp_intensity = 0;
  }

}
