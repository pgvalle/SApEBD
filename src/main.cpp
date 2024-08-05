#include <Arduino.h>
#include <SoftwareSerial.h>

#include <WiFiEsp.h>
#include <TimeLib.h>  // Time by Michael Margolis
#define dtNBR_ALARMS 32
#include <TimeAlarms.h>  // TimeAlarms by Michael Margolis

#define DELAY_INTERVAL 50  // milliseconds

#define RING_DURATION 5  // seconds
#define RELAY 12  // TODO: change pin when installing relay
#define LED   13

// Your WiFi credentials
#define SSID "Mefibosete24"
#define PASS "papito12345"

enum TimeStatus {
  TIME_OK, TIME_IMPRECISE, TIME_WRONG
};

// STATE VARIABLES //

SoftwareSerial espSerial(10, 11);  // RX, TX
bool alarmRinging = false;
TimeStatus timeStatus = TIME_WRONG;
bool ledOn = false;
bool ledTicks = 0;

// FUNCTIONS //

void alarmRing();
void alarmStopRinging();
void timeShowStatus();
time_t timeSync();

// ENTRY POINT //

void setup() {
  Serial.begin(115200);

  // setup pins
  pinMode(LED, OUTPUT);
  pinMode(RELAY, OUTPUT);

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

  alarmStopRinging();
}

void loop() {
  timeShowStatus();

  Alarm.delay(DELAY_INTERVAL);
}

// FUNCTIONS DEFINITIONS //

void alarmRing() {
  alarmRinging = true;
  digitalWrite(RELAY, alarmRinging);

  Serial.println("Ringing");

  Alarm.timerOnce(RING_DURATION, alarmStopRinging);
}

void alarmStopRinging() {
  alarmRinging = false;
  digitalWrite(RELAY, alarmRinging);

  Serial.println("Not ringing");
}

void timeShowStatus() {
  int ticks2wait;
  switch (timeStatus) {
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
    if (timeStatus == TIME_OK) timeStatus = TIME_IMPRECISE;
    else if (precisionLoss >=  7) timeStatus = TIME_WRONG;
    else if (timeStatus == TIME_IMPRECISE) precisionLoss++;
    return now();
  }

  WiFiEspClient client;
  client.connect("worldtimeapi.org", 80);
  if (!client.connected()) {
    if (timeStatus == TIME_OK) timeStatus = TIME_IMPRECISE;
    else if (precisionLoss >=  7) timeStatus = TIME_WRONG;
    else if (timeStatus == TIME_IMPRECISE) precisionLoss++;
    return now();
  }

  client.println("GET /api/timezone/America/Sao_Paulo HTTP/1.1");
  client.println("Host: worldtimeapi.org");
  client.println("Connection: close");
  client.println();

  // ignore everything before the value we care about
  client.find("unixtime\":");

  String unixTimeStr = client.readStringUntil(',');
  const time_t unixTime = unixTimeStr.toInt() - 10795;

  Serial.println(unixTime);

  client.stop();

  timeStatus = TIME_OK;
  precisionLoss = 0;

  return unixTime;
}
