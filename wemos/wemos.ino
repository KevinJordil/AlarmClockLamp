#include "Arduino.h"

#include <NTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#define PORT 80
#define LAMP_STATES 100

const char* ssid = "";
const char* password = "";

const int switchButtonPin  = D5; //11;
const int zeroCrossPin  = D6; //12;
const int pwmPin  = D7; //13;


ESP8266WebServer server(80);
WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 7200);
int startTimestamp = -1;
int durationTimestamp = -1;
int epochStates = -1;

bool lampOn = true;
bool progressive_intensity = false;
int delay_time;
int lamp_intensity = 0;

int readSwitch;
int lastReadSwitchButton = -1;


void ICACHE_RAM_ATTR interrupt() {
  if (lamp_intensity > 2) {
    // Set delay time by lamp_intensity, 0.5 for 50% maximum
    delay_time = (100 - (lamp_intensity * 0.5)) * 100;
    
    // Minimum delay time to avoid glitch
    delay_time = delay_time < 250 ? 250 : delay_time;

    //Serial.println(delay_time);

    delayMicroseconds(delay_time);

    digitalWrite(pwmPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(pwmPin, LOW);
  }
}

void setup() {


  Serial.begin(115200);
  Serial.println("Begin...");

  pinMode(LED_BUILTIN, OUTPUT); // Led on Wemos
  digitalWrite(LED_BUILTIN, LOW); // led On

  pinMode(switchButtonPin, INPUT_PULLUP);
  digitalWrite(switchButtonPin, HIGH);

  pinMode(pwmPin, OUTPUT);
  pinMode(zeroCrossPin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(zeroCrossPin), interrupt, RISING);

  Serial.println("Connect to wifi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Try to connect to wifi...");
    delayMicroseconds(16383); // Max miscroseconds time
  }

  Serial.println("Connected!");
  Serial.println("Start web server...");

  server.on("/", HTTP_POST, []() {

    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("Get request");
    startTimestamp = server.arg("starttimestamp").toInt();
    durationTimestamp  = server.arg("durationtimestamp").toInt();

    epochStates = durationTimestamp / LAMP_STATES;

    timeClient.setTimeOffset(server.arg("gmtoffset").toInt());
    timeClient.update();

    delayMicroseconds(16383);
    digitalWrite(LED_BUILTIN, HIGH);

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

  readSwitch = digitalRead(switchButtonPin);
  if (readSwitch != lastReadSwitchButton) {
    //Serial.println("Switch");
    lastReadSwitchButton = readSwitch;
    lampOn = !lampOn;
    if (progressive_intensity) {
      startTimestamp = -1;
      progressive_intensity = false;
    }
  }

  if (startTimestamp != -1 && timeClient.getEpochTime() >= startTimestamp && !lampOn) {
    //Serial.println("Ready to start intensity");
    progressive_intensity = true;
    lampOn = true;
  }

  if (lampOn) {
    // Need to power on with specific intensity
    if (startTimestamp != -1 && timeClient.getEpochTime() >= startTimestamp && timeClient.getEpochTime() < startTimestamp + durationTimestamp) {

      //Serial.println(((timeClient.getEpochTime() - startTimestamp) / epochStates) + 1);
      lamp_intensity = ((timeClient.getEpochTime() - startTimestamp) / epochStates) + 1;

    } else {
      if (progressive_intensity) {
        startTimestamp = -1;
        progressive_intensity = false;
      }
      lamp_intensity = 100;
      //Serial.println("ON");
    }
  } else {
    lamp_intensity = 0;
  }

}
