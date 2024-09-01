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

// in case it's not connected to a network
bool triedReconnection = false;
uint64_t timeLastReconnection = 0;
// esp wifi connection config
char ssid[17] = "Mefibosete24", pass[17] = "papito12345";
// time sync
WiFiUDP ntpUDP;
NTPClient ntpClient(ntpUDP);
// web server
ESP8266WebServer webServer(80);

bool saveWiFiConf();
void loadWiFiConf();
void tryConnecting2WiFi();
// Pages handlers
void handleNotFound();
void handleRoot();
void handleAlarms();
void handleSave();
void handleReboot();


void setup() {
  Serial.begin(9600);

  const size_t memSize = sizeof(ssid) + sizeof(pass) - 2;
  //EEPROM.begin(memSize);

  WiFi.mode(WIFI_AP_STA);  // allow wifi connection and access point
  WiFi.softAP(HOSTNAME, "sinalconf", 1, false, 1);

  // load wifi config
  // for (int i = 0; i < 16; i++) {
  //   ssid[i] = EEPROM.read(i);
  //   pass[i] = EEPROM.read(i + 16);
  // }

  tryConnecting2WiFi();

  // setup web server
  webServer.begin();
  webServer.onNotFound(handleNotFound);
  webServer.on("/", HTTP_GET, handleRoot);
  webServer.on("/save", HTTP_GET, handleSave);
  webServer.on("/reboot", HTTP_GET, handleReboot);

  // setup time client
  ntpClient.begin();
  ntpClient.setUpdateInterval(3600000);  // update each hour
  ntpClient.setTimeOffset(-10800);  // UTC-3

  // so that we don't need to know esp's ip
  MDNS.begin(HOSTNAME);
}

void loop() {
  webServer.handleClient();
  ntpClient.update();  // time sync
  MDNS.update();  // hostname

  // try reconnecting if not connected
  if (!WiFi.isConnected() && !triedReconnection) {
    Serial.println("No Connection. Trying to reconnect...");
    tryConnecting2WiFi();
    triedReconnection = true;
    timeLastReconnection = millis();
    // force resync
    if (WiFi.isConnected())
      ntpClient.forceUpdate();
  }

  // Reconnection should be tried once each RECONNECTION_TIMEOUT milliseconds
  if (triedReconnection) {
    const uint64_t delta = millis() - timeLastReconnection;
    if (delta > RECONNECTION_TIMEOUT)
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

void tryConnecting2WiFi() {
  WiFi.begin(ssid, pass);

  if (WiFi.waitForConnectResult(8000) == WL_CONNECTED)
    Serial.println("Connected");
  else
    Serial.println("Not Connected");
}

void handleNotFound() {
  webServer.send(404, "text/html", "Página não encontrada");
}

void handleRoot() {
  String root = R"(
    <html>
    <head>
      <style>
        body { font-family: Arial, sans-serif; text-align: center; }
        button { padding: 10px 20px; margin: 10px; }
        input { padding: 10px; margin: 10px; }
      </style>
      <meta charset="UTF-8">
      <meta http-equiv="X-UA-Compatible" content="IE=edge">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Sinal</title>
    </head>
    <body>
      <h1>Sinal Autônomo</h1>
      <form action='/save' method='GET'>
        <input type='text' name='ssid' placeholder='Nova rede ()";
  
  root += ssid;

  root += R"()'><br>
        <input type='password' name='pass' placeholder='Nova senha'><br>
        <button type='submit'>salvar</button>
      </form>
      <form action='/alarms' method='GET'>
        <button type='submit'>alarmes</button>
      </form>
      <form action='/reboot' method='GET'>
        <button type='submit'>reiniciar</button>
      </form>
    </body>
    </html>
  )";

  Serial.println("/");
  webServer.send(200, "text/html", root);
}

void handleAlarms() {
  Serial.println("/alarms");
  webServer.send(200, "text/html", "In progress...");
}

void handleSave() {
  String network = webServer.arg("ssid");  // Get value from the first text input
  String password = webServer.arg("pass");  // Get value from the second text input

  Serial.println("/save");
  // just for debug now
  Serial.println("  ssid: " + network);
  Serial.println("  pass: " + password);

  /*
  for (int i = 0; i < 12; i++) {
    EEPROM.put(0, ssid[i]);
    EEPROM.put(12, pass[i]);
  }*/

  if (EEPROM.commit())
    webServer.send(200, "text/html", R"(
      <!DOCTYPE html>
      <html lang="br">
      <head>
        <style>
          body { font-family: Arial, sans-serif; text-align: center; }
          button { padding: 10px 20px; margin: 10px; }
          input { padding: 10px; margin: 10px; }
        </style>
        <meta charset="UTF-8">
        <meta http-equiv="X-UA-Compatible" content="IE=edge">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <meta http-equiv='refresh' content='10; url=/'>
        <title>Sinal</title>
      </head>
      <body>
        <h1>Nova rede salva</h1>
        Reiniciando em 10 segundos
      </body>
      </html>
    )");
  else
    webServer.send(200, "text/html", R"(Erro ao salvar)");

  // if data saved, restart automatically
  // else tell something went wrong
}

void handleReboot() {
  webServer.send(200, "text/html", R"(
    <!DOCTYPE html>
    <html lang="br">
    <head>
      <style>
        body { font-family: Arial, sans-serif; text-align: center; }
        button { padding: 10px 20px; margin: 10px; }
        input { padding: 10px; margin: 10px; }
      </style>
      <meta charset="UTF-8">
      <meta http-equiv="X-UA-Compatible" content="IE=edge">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <meta http-equiv='refresh' content='10; url=/'>
      <title>Sinal</title>
    </head>
    <body>
      <h1>Reiniciando...</h1>
    </body>
    </html>
  )");

  Serial.println("/reboot");
  delay(1000);
  ESP.restart();  // Reboot the ESP8266
}