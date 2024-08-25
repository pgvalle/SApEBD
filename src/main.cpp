#include <Arduino.h>
#include <SoftwareSerial.h>

#include <WiFiEsp.h>
#include <TimeLib.h>  // Time by Michael Margolis
#include <TimeAlarms.h>  // TimeAlarms by Michael Margolis

#define DELAY_INTERVAL 500  // milliseconds
#define SYNC_INTERVAL 1440

#define RING_DURATION 5  // seconds
#define RELAY 8
#define LED   13

// Your WiFi credentials
#define SSID "ssid"
#define PASS "pass"

enum TimeStatus {
  TIME_OK, TIME_IMPRECISE, TIME_WRONG
};

// STATE VARIABLES //

SoftwareSerial espSerial(10, 11);  // RX, TX
TimeStatus t1meStatus = TIME_WRONG;
int precisionLoss = 0;
time_t unixTime = 0;

// FUNCTIONS //

void ringAlarm();
void showTimeStatus();
time_t syncTime();

// ENTRY POINT //

void setup() {
  Serial.begin(115200);

  // setup pins
  pinMode(LED, OUTPUT);
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, false);

  // Initialize the WiFiEsp library with the software serial
  espSerial.begin(9600);
  WiFi.init(&espSerial);

  // configure TimeLib with rtc
  setTime(0);
  setSyncInterval(SYNC_INTERVAL);  // sync once each 2 days
  setSyncProvider(syncTime);

  Alarm.alarmRepeat(dowSunday, 9, 0, 0, ringAlarm);  // EBD início
  Alarm.alarmRepeat(dowSunday, 11, 0, 0, ringAlarm);  // EBD final
  Alarm.alarmRepeat(dowSunday, 11, 10, 0, ringAlarm);  // EBD final 2
  Alarm.alarmRepeat(dowSunday, 18, 30, 0, ringAlarm);  // Domingo noite
  Alarm.alarmRepeat(dowWednesday, 19, 30, 0, ringAlarm);  // Quarta

  //Alarm.alarmRepeat(dowSunday, 13, 54, 30, ringAlarm);  // test
}

void loop() {
  showTimeStatus();
  Alarm.delay(DELAY_INTERVAL);
}

// FUNCTIONS DEFINITIONS //

void ringAlarm() {
  if (t1meStatus == TIME_WRONG)
    return;

  digitalWrite(RELAY, true);

  Alarm.timerOnce(RING_DURATION, []() {
    digitalWrite(RELAY, false);
  });
}

void showTimeStatus() {
  int ticks2wait;
  switch (t1meStatus) {
    case TIME_OK:
      digitalWrite(LED, HIGH);
      return;
    case TIME_IMPRECISE:
      ticks2wait = 2;
      break;
    case TIME_WRONG:
      ticks2wait = 1;
      break;
  default:
    break;
  }

  // blink
  static int ledTicks = 0;

  if (++ledTicks == ticks2wait) {
    static bool ledOn = false;

    ledOn = !ledOn;
    ledTicks = 0;
    digitalWrite(LED, ledOn);
  }
}

void failSafe() {
  if (t1meStatus == TIME_OK)
    t1meStatus = TIME_IMPRECISE;
  else if (t1meStatus == TIME_IMPRECISE) {
    if (++precisionLoss >= 5)
      t1meStatus = TIME_WRONG;
  }

  unixTime += SYNC_INTERVAL + 5;  // aproximation. Precission loss
}

time_t syncTime() {
  if (WiFi.status() != WL_CONNECTED)
    WiFi.begin(SSID, PASS);  // try once

  if (WiFi.status() != WL_CONNECTED) {
    failSafe();
    return unixTime;
  }

  WiFiEspClient client;
  client.setTimeout(5000);
  client.connect("worldtimeapi.org", 80);
  if (!client.connected()) {
    failSafe();
    return unixTime;
  }

  client.println("GET /api/timezone/America/Sao_Paulo HTTP/1.1");
  client.println("Host: worldtimeapi.org");
  client.println("Connection: close");
  client.println();

  // ignore everything before the value we care about
  client.find((char *)"unixtime\":");

  String unixTimeStr = client.readStringUntil(',');

  precisionLoss = 0;
  t1meStatus = TIME_OK;
  unixTime = unixTimeStr.toInt() - 10800;

  client.stop();

  return unixTime;
}
