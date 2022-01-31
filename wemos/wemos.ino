
#include "Arduino.h"

#include <NTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <RBDmcuESP8266.h>
#define PORT 80
#define LAMP_STATES 100

const char* ssid = "Wifi Kevin 2.5";
const char* password = "YverdonBeach";

//Parameters
const int switchButton  = D5; //11;
const int zeroCrossPin  = D6; //12;
const int acdPin  = D7; //13;
bool lampOn = true;
int lastRead = -1;


//Objects
dimmerLamp acd(acdPin, zeroCrossPin);


ESP8266WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 7200);
int startTimestamp = -1, durationTimestamp = -1, epochStates = -1, lastPower = 0;
String displayAlarm = "";


void setup() {

  // Connect to wifi
  Serial.begin(115200);
  Serial.println("Begin...");
  acd.begin(NORMAL_MODE, ON);
  pinMode(switchButton, INPUT_PULLUP);
  digitalWrite(switchButton, HIGH);
  acd.setPower(10);
  Serial.println("Connect to wifi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    acd.setPower(1);
    delay(250);
    acd.setPower(10);
    delay(250);
  }
  Serial.println("Connected!");
  Serial.println("Start web server...");
  acd.setPower(10);
  server.on("/", HTTP_POST, []() {

    Serial.println("Get request");
    startTimestamp = server.arg("starttimestamp").toInt();
    durationTimestamp  = server.arg("durationtimestamp").toInt();

    epochStates = durationTimestamp / LAMP_STATES;

    displayAlarm = server.arg("alarm");

    timeClient.setTimeOffset(server.arg("gmtoffset").toInt());
    timeClient.update();


    server.send(200, "text/plain", "Timestamp receive");
    acd.setPower(20);
    delay(1000);
    acd.setPower(lastPower);
  });

  server.begin();
  timeClient.begin();
  timeClient.update();
  delay(500);
  acd.setPower(20);
  delay(2000);
  acd.setPower(1);
  Serial.println("Listening...");
}

void loop() {
  server.handleClient();
  int readSwitch = digitalRead(switchButton);
  if (readSwitch != lastRead) {
    Serial.println("Switch");
    lastRead = readSwitch;
    lampOn = !lampOn;
  }

  if (startTimestamp != -1 && timeClient.getEpochTime() >= startTimestamp && lastPower == 0){
    Serial.println("Ready to start intensity");
    lampOn = true;
  }

  if (lampOn) {
    if (startTimestamp != -1 && timeClient.getEpochTime() >= startTimestamp && startTimestamp + durationTimestamp > timeClient.getEpochTime()) {//Allumer lampe avec intensité

      
      int intensity = ((timeClient.getEpochTime() - startTimestamp) / epochStates) + 1;
      if (lastPower != intensity) {
        Serial.print("Set intensity : ");
        Serial.println(intensity);
        acd.setPower(intensity);
        lastPower = intensity;
      }

    } else if (lastPower != 100) {
      startTimestamp = -1;
      lastPower = 100;
      acd.setPower(100);
      Serial.println("ON");
    }
  } else if (lastPower != 0) {
      startTimestamp = -1;
      lastPower = 0;
  } else {
      acd.setPower(1);
      Serial.println("OFF");
  }

  //delay(1000);
}
