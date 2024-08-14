#include <Arduino.h>
#include <SoftwareSerial.h>

#include <WiFiEsp.h>
#include <TimeLib.h>  // Time by Michael Margolis
#include <TimeAlarms.h>  // TimeAlarms by Michael Margolis

#define DELAY_INTERVAL 10  // milliseconds

#define RING_DURATION 5  // seconds
#define RELAY 8
#define LED   13

// Your WiFi credentials
#define SSID "IBS"
#define PASS "cristovida1234"

enum TimeStatus {
  TIME_OK, TIME_IMPRECISE, TIME_WRONG
};

// STATE VARIABLES //

SoftwareSerial espSerial(10, 11);  // RX, TX
bool alarmRinging = false;
TimeStatus t1meStatus = TIME_WRONG;
bool ledOn = false;
int ledTicks = 0;

// FUNCTIONS //

void alarmRing();
void timeShowStatus();
time_t timeSync();

// ENTRY POINT //

void setup() {
  Serial.begin(115200);

  // setup pins
  pinMode(LED, OUTPUT);
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, alarmRinging);

  // Initialize the WiFiEsp library with the software serial
  espSerial.begin(9600);
  WiFi.init(&espSerial);

  // configure TimeLib with rtc
  setSyncInterval(2880);  // sync once each 2 days
  setSyncProvider(timeSync);

  Alarm.alarmRepeat(dowSunday, 9, 0, 0, alarmRing);  // EBD início
  Alarm.alarmRepeat(dowSunday, 11, 0, 0, alarmRing);  // EBD final
  Alarm.alarmRepeat(dowSunday, 11, 10, 0, alarmRing);  // Domingo manhã
  Alarm.alarmRepeat(dowSunday, 18, 30, 0, alarmRing);  // Domingo noite
  Alarm.alarmRepeat(dowWednesday, 19, 30, 0, alarmRing);  // Quarta

  // Alarm.alarmRepeat(dowSunday, 8, 23, 30, alarmRing);  // test
}

void loop() {
  timeShowStatus();
  Alarm.delay(DELAY_INTERVAL);
}

// FUNCTIONS DEFINITIONS //

void alarmRing() {
  digitalWrite(RELAY, true);
  alarmRinging = true;

  Alarm.timerOnce(RING_DURATION, []() {
    digitalWrite(RELAY, false);
    alarmRinging = false;
  });
}

void timeShowStatus() {
  int ticks2wait;
  switch (t1meStatus) {
    case TIME_OK:
      digitalWrite(LED, HIGH);
      return;
    case TIME_IMPRECISE:
      ticks2wait = 500 / DELAY_INTERVAL;
      break;
    case TIME_WRONG:
      ticks2wait = 250 / DELAY_INTERVAL;
      break;
  default:
    break;
  }

  if (++ledTicks % ticks2wait == 0) {
    ledOn = !ledOn;
    ledTicks = 0;
    digitalWrite(LED, ledOn);
  }
}

time_t timeSync() {
  static int precisionLoss = 0;

  if (WiFi.status() != WL_CONNECTED) WiFi.begin(SSID, PASS);  // try once
  if (WiFi.status() != WL_CONNECTED) {
    if (t1meStatus == TIME_OK) t1meStatus = TIME_IMPRECISE;
    else if (precisionLoss >=  7) t1meStatus = TIME_WRONG;
    else if (t1meStatus == TIME_IMPRECISE) precisionLoss++;
    return now();
  }

  WiFiEspClient client;
  client.connect("worldtimeapi.org", 80);
  if (!client.connected()) {
    if (t1meStatus == TIME_OK) t1meStatus = TIME_IMPRECISE;
    else if (precisionLoss >=  7) t1meStatus = TIME_WRONG;
    else if (t1meStatus == TIME_IMPRECISE) precisionLoss++;
    return now();
  }

  client.println("GET /api/timezone/America/Sao_Paulo HTTP/1.1");
  client.println("Host: worldtimeapi.org");
  client.println("Connection: close");
  client.println();

  // ignore everything before the value we care about
  client.find((char *)"unixtime\":");

  String unixTimeStr = client.readStringUntil(',');
  const time_t unixTime = unixTimeStr.toInt() - 10800;

  client.stop();

  t1meStatus = TIME_OK;
  precisionLoss = 0;

  return unixTime;
}
