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
#define RELAY 5  // D1
#define LED   4  // D2
#define RECONNECTION_TIMEOUT 300000  // 5 min (in milliseconds)

// alarm stuff
bool isAlarming = false;
uint64_t alarmStart = 0;
// in case it's not connected to a network
bool triedReconnection = false;
uint64_t reconnectionStart = 0;
// esp wifi connection config
char ssid[17] = "Mefibosete24", pass[17] = "papito12345";
// time sync
WiFiUDP ntpUDP;
NTPClient ntpClient(ntpUDP, -10800, 3600000);
// web server
ESP8266WebServer webServer(80);
// page base
String pageCommon = R"(
  <!DOCTYPE html>
  <html>
  <head>
    <style>
      body { font-family: Arial, sans-serif; text-align: center; }
      button { padding: 10px 20px; margin: 10px; }
      input { padding: 10px; margin: 10px; }
    </style>
    <meta charset='UTF-8'>
    <meta http-equiv='X-UA-Compatible' content='IE=edge'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>Sinal</title>
)";

// alarm stuff
bool checkDOWTime(int dow, int h, int m, int s);
void try2Alarm();
// wifi stuff
void tryConnecting2WiFi();
// Pages handlers
void handleNotFound();
void handleRoot();
void handleRing();
void handleSave();
void handleReboot();


void setup() {
  Serial.begin(9600);

  pinMode(RELAY, OUTPUT);
  pinMode(LED, OUTPUT);

  const size_t memSize = sizeof(ssid) + sizeof(pass) - 2;
  //EEPROM.begin(memSize);

  WiFi.mode(WIFI_AP_STA);  // allow wifi connection and access point
  WiFi.softAP(HOSTNAME, "sinalconf", 1, false, 1);

  // load wifi config
  /*for (int i = 0; i < 16; i++) {
    ssid[i] = EEPROM.read(i);
    pass[i] = EEPROM.read(i + 16);
  }*/

  Serial.println("  ssid: " + String(ssid));
  Serial.println("  pass: " + String(pass));

  tryConnecting2WiFi();

  // setup web server
  webServer.begin();
  webServer.onNotFound(handleNotFound);
  webServer.on("/", HTTP_GET, handleRoot);
  webServer.on("/ring", HTTP_GET, handleRing);
  webServer.on("/save", HTTP_GET, handleSave);
  webServer.on("/reboot", HTTP_GET, handleReboot);

  // setup time client
  ntpClient.begin();

  // so that we don't need to know esp's ip
  MDNS.begin(HOSTNAME);
}

void loop() {
  webServer.handleClient();
  ntpClient.update();  // time sync
  MDNS.update();  // hostname

  try2Alarm();

  // try reconnecting if not connected
  if (!WiFi.isConnected() && !triedReconnection) {
    Serial.println("No Connection. Trying to reconnect...");
    tryConnecting2WiFi();
    triedReconnection = true;
    reconnectionStart = millis();
    // force resync
    if (WiFi.isConnected())
      ntpClient.forceUpdate();
  }

  // Reconnection should be tried once each RECONNECTION_TIMEOUT milliseconds
  if (triedReconnection) {
    if (millis() - reconnectionStart > RECONNECTION_TIMEOUT)
      triedReconnection = false;
  }
}


bool checkDOWTime(int dow, int h, int m, int s) {
  return ntpClient.getDay() == dow &&
         ntpClient.getHours() == h &&
         ntpClient.getMinutes() == m &&
         ntpClient.getSeconds() == s;
}

void try2Alarm() {
  if (isAlarming) {
    if (millis() - alarmStart >= 5000) {  // 5 seconds alarming
      digitalWrite(RELAY, LOW);
      isAlarming = false;
    }
    return;
  }

  // 9:00:00  // inicie ebd
  // 10:55:00  // ebd final 1
  // 11:05:00  // ebd final 2
  // 11:15:00  // ebd final 3
  // 18:30:00 // domingo noite
  // 19:30:00 // quarta
  if (checkDOWTime(0, 9, 0, 0)) {
    digitalWrite(RELAY, HIGH);
    isAlarming = true;
    alarmStart = millis();
    return;
  }

  if (checkDOWTime(0, 10, 55, 0)) {
    digitalWrite(RELAY, HIGH);
    isAlarming = true;
    alarmStart = millis();
    return;
  }

  if (checkDOWTime(0, 11, 05, 0)) {
    digitalWrite(RELAY, HIGH);
    isAlarming = true;
    alarmStart = millis();
    return;
  }

  if (checkDOWTime(0, 11, 15, 0)) {
    digitalWrite(RELAY, HIGH);
    isAlarming = true;
    alarmStart = millis();
    return;
  }

  if (checkDOWTime(0, 18, 30, 0)) {
    digitalWrite(RELAY, HIGH);
    isAlarming = true;
    alarmStart = millis();
    return;
  }

  if (checkDOWTime(3, 19, 30, 0)) {
    digitalWrite(RELAY, HIGH);
    isAlarming = true;
    alarmStart = millis();
    return;
  }
}

void tryConnecting2WiFi() {
  WiFi.begin(ssid, pass);

  if (WiFi.waitForConnectResult(8000) == WL_CONNECTED)
    Serial.println("Connected");
  else
    Serial.println("Not Connected");
}

void handleNotFound() {
  webServer.send(404, "text/html", pageCommon + R"(
    </head>
    <body>
      <h1>Página não encontrada</h1>
      <form action='/' method='GET'>
        <button type='submit'>página inicial</button>
      </form>
    </body>
    </html>
  )");
}

void handleRoot() {
  Serial.println("/");
  webServer.send(200, "text/html", pageCommon + R"(
    </head>
    <body>
      <h1>Sinal Autônomo</h1>
      <form action='/save' method='GET'>
        <input type='text' name='ssid' placeholder='Nova rede ()" + ssid + R"()'><br>
        <input type='password' name='pass' placeholder='Nova senha'><br>
        <button type='submit'>salvar</button>
      </form>
      <form action='/ring' method='GET'>
        <button type='submit'>tocar (5s)</button>
      </form>
      <form action='/reboot' method='GET'>
        <button type='submit'>reiniciar</button>
      </form>
    </body>
    </html>
  )");
}

void handleRing() {
  // only alarm now if not alarming
  if (!isAlarming) {
    digitalWrite(RELAY, HIGH);
    isAlarming = true;
    alarmStart = millis();
  }

  Serial.println("/ring");
  webServer.send(200, "text/html", pageCommon + R"(
      <meta http-equiv='refresh' content='5; url=/'>
    </head>
    <body>
      <h1>Tocando</h1>
    </body>
    </html>
  )");
}

void handleSave() {
  String network = webServer.arg("ssid");  // Get value from the first text input
  String password = webServer.arg("pass");  // Get value from the second text input

  Serial.println("  new ssid: " + network);
  Serial.println("  new pass: " + password);

  /*for (int i = 0; i < 12; i++) {
    EEPROM.put(0, ssid[i]);
    EEPROM.put(12, pass[i]);
  }*/

  if (EEPROM.commit()) {
    Serial.println("/save Succeeded");
    webServer.send(200, "text/html", pageCommon + R"(
        <meta http-equiv='refresh' content='10; url=/'>
      </head>
      <body>
        <h1>Configurações salvas</h1>
        <p>Reiniciando em 10 segundos</p>
      </body>
      </html>
    )");
  }
  else {
    Serial.println("/save Failed");
    webServer.send(200, "text/html", pageCommon + R"(
      </head>
      <body>
        <h1>Falha ao salvar configurações</h1>
      </body>
      </html>
    )");
  }
}

void handleReboot() {
  webServer.send(200, "text/html", pageCommon + R"(
      <meta http-equiv='refresh' content='10; url=/'>
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