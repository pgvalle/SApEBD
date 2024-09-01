// for storing wifi connection data
#include <ESP_EEPROM.h>
// wifi stuff
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
// webpage
#include <ESP8266WebServer.h>
// ntp client
#include <NTPClient.h>
#include <WiFiUdp.h>

#define HOSTNAME "sinal"
#define RELAY 2
#define LED   3
#define UPDATE_INTERVAL 250
#define RECONNECTION_TIMEOUT 300000  // 5 min

bool triedReconnection = false;
uint64_t timeLastReconnection = 0;
// esp wifi connection config
char ssid[17] = "ssid", pass[17] = "pass";
// time sync
WiFiUDP ntpUDP;
NTPClient ntpClient(ntpUDP);
// web server
ESP8266WebServer webServer(80);

bool saveWiFiConf();
void loadWiFiConf();
void tryConnecting2WiFi();
// server
void setupServices();
void handleMainPage();
void handlePageNotFound();



void setup() {
  Serial.begin(9600);

  const size_t memSize = sizeof(ssid) + sizeof(pass) - 2;
  //EEPROM.begin(memSize);

  //loadWiFiConf();
  tryConnecting2WiFi();
  setupServices();

  // so that we don't need to know esp's ip
  MDNS.begin(HOSTNAME);
}

void loop() {
  webServer.handleClient();
  ntpClient.update();  // time sync
  MDNS.update();  // hostname

  // try reconnecting if not connected
  if (!WiFi.isConnected() && !triedReconnection) {
    Serial.println("Connection lost. Trying to reconnect...");
    tryConnecting2WiFi();
    setupServices();
    triedReconnection = true;
    timeLastReconnection = millis();
  }

  // econnection should be tried once each RECONNECTION_TIMEOUT milliseconds
  if (triedReconnection) {
    const uint64_t timeSinceReconnection = millis() - timeLastReconnection;
    if (timeSinceReconnection > RECONNECTION_TIMEOUT)
      triedReconnection = false;
  }
}



bool saveWiFiConf() {
  for (int i = 0; i < 12; i++) {
    EEPROM.put(0, ssid[i]);
    EEPROM.put(12, pass[i]);
  }

  return EEPROM.commit();
}

void loadWiFiConf() {
  for (int i = 0; i < 12; i++) {
    ssid[i] = EEPROM.read(i);
    pass[i] = EEPROM.read(i + 12);
  }
}

void tryConnecting2WiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  for (int t = 15; t > 0; t--) {  // 15 trials
    delay(500);  // check if connected each half second
    if (WiFi.isConnected()) {
      Serial.print("Connected, IP: ");
      Serial.println(WiFi.localIP());
      return;
    }
  }

  triedReconnection = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP(HOSTNAME, "sinalconf", 1, false, 1);

  Serial.println("Not Connected, AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void setupServices() {
  // setup web server
  webServer.begin();
  webServer.on("/", HTTP_GET, handleMainPage);
  webServer.onNotFound(handlePageNotFound);

  // setup time client
  ntpClient.begin();
  ntpClient.setUpdateInterval(3600000);  // update each hour
  ntpClient.setTimeOffset(-10800);  // UTC-3
}

void handleMainPage() {
  String message = R"STR(<html>
<meta http-equiv='content-type' content='text/html; charset=utf-8'>
<meta http-equiv='refresh' content='10'>)STR";
  message += "<p>";
  message += ntpClient.getFormattedTime();
  message += "</p>";
  message += "<p>";
  message += WiFi.isConnected() ? "connected" : "not connected";
  message += "</p>";
  message += "</html>";

  webServer.send(200, "text/html", message);
}

void handlePageNotFound() {
  webServer.send(404, "text/html", "Página não encontrada");
}