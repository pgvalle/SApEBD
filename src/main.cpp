// wifi stuff
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
// webpage
#include <ESP8266WebServer.h>
// ntp client
#include <WiFiUdp.h>
#include <NTPClient.h>

#define HOSTNAME "sinal"
#define RELAY 15 // D1
#define LED   13 // D2

// alarm stuff
bool isAlarming = false;
uint64_t alarmStart = 0;
// time sync
WiFiUDP ntpUDP;
NTPClient ntpClient(ntpUDP, -10800, 3600000);  // UTC-3 (-10800), update each hour (3600000)
// web server
ESP8266WebServer webServer(80);
// common stuff for all subpages
String pageCommon = R"(
  <!DOCTYPE html>
  <html>
  <head>
    <script>
      window.onload = function () {
        const navType = performance.navigation.type
        if (navType === 1 || navType === 2)
          window.location.replace('/')
      }
    </script>
    <style>
      body { font-family: Arial, sans-serif; text-align: center; }
      p { margin: 10px 0px 5px; }
      button { padding: 5px 10px; margin: 5px; }
      input { padding: 5px; margin: 5px; }
      .form-container {
        background-color: #d0d0d0;
        padding: 0px 15px;
        display: inline-block; margin: 5px
      }
      .button-group {
        display: flex;
        justify-content: center;
        margin-top: 10px;
        margin: 0 5px;
      }
    </style>
    <meta charset='UTF-8'>
    <meta http-equiv='X-UA-Compatible' content='IE=edge'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>Sinal</title>
)";

// alarm stuff
bool checkDOWTime(int dow, int h, int m, int s);
void try2Alarm();
// Pages handlers
void handleNotFound();
void handleRoot();
void handleRing();
void handleSave();


void setup() {
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  
  Serial.begin(9600);

  // setup persistent wifi
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  WiFi.mode(WIFI_AP_STA);  // allow wifi connection and access point
  WiFi.softAP(HOSTNAME, "sinalconf", 1, false, 1);
  WiFi.begin();

  WiFi.waitForConnectResult(8000);

  // setup web server
  webServer.begin();
  webServer.onNotFound(handleNotFound);
  webServer.on("/", HTTP_GET, handleRoot);
  webServer.on("/ring", HTTP_GET, handleRing);
  webServer.on("/save", HTTP_GET, handleSave);

  // setup time client
  ntpClient.begin();
  // Hostname domain so that we don't need to know esp's ip
  MDNS.begin(HOSTNAME);

  Serial.println("Setup done");
}

void loop() {
  webServer.handleClient();
  ntpClient.update();  // time sync
  MDNS.update();  // hostname

  try2Alarm();

  digitalWrite(LED, WiFi.isConnected());
}


bool checkDOWTime(int dow, int h, int m, int s) {
  // if time not set, then don't ring
  if (!ntpClient.isTimeSet())
    return false;

  const time_t givenDaySeconds = 3600 * h + 60 * m + s;
  const time_t realDaySeconds = ntpClient.getEpochTime() % 86400L;
  return ntpClient.getDay() == dow &&
         abs(givenDaySeconds - realDaySeconds) <= 2;
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

void handleNotFound() {
  Serial.println("Not found");
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
      <form class='form-container' action='/save' method='GET'>
        <p>Configure uma nova rede<p>
        <input type='text' name='ssid' placeholder=')" + WiFi.SSID() + R"( (atual)'><br>
        <input type='password' name='pass' placeholder='senha'><br>
        <button type='submit'>salvar</button>
      </form>
      <div class='button-group'>
        <form action='/ring' method='GET'>
          <button type='submit'>tocar 5s</button>
        </form>
      </div>
    </body>
    </html>
  )");
}

void handleRing() {
  Serial.println("/ring");

  // only alarm now if not alarming
  if (!isAlarming) {
    digitalWrite(RELAY, HIGH);
    isAlarming = true;
    alarmStart = millis();
  }

  webServer.send(200, "text/html", pageCommon + R"(
      <meta http-equiv='refresh' content='5; url=/'>
    </head>
    <body>
      <h1>Tocando</h1>
      <p>...</p>
    </body>
    </html>
  )");
}

void handleSave() {
  Serial.println("/save");

  String ssid = webServer.arg("ssid");  // Get value from the first text input
  String pass = webServer.arg("pass");  // Get value from the second text input

  if (ssid == "" && pass == "") {  // both empty fields not expected
    webServer.send(200, "text/html", pageCommon + R"(
      </head>
      <body>
        <h1>???</h1>
        <p>Você tentou salvar uma rede sem nome e sem senha</p>
        <form action='/' method='GET'>
          <button type='submit'>pagina inicial</button>
        </form>
      </body>
      </html>
    )");

    return;
  }

  if (ssid == "")  // just changing the password, so ssid keeps the same
    ssid = WiFi.SSID();

  webServer.send(200, "text/html", pageCommon + R"(
    </head>
    <body>
      <h1>Configurações salvas</h1>
      <p>Conecte seu dispositivo à nova rede ou à rede do sinal antes de prosseguir</p>
      <form action='/' method='GET'>
        <button type='submit'>pagina inicial</button>
      </form>
    </body>
    </html>
  )");

  delay(2000);
  WiFi.begin(ssid, pass);
}
