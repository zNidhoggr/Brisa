#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Time.h>

#ifndef STASSID
#define STASSID "GoodPlace"
#define STAPSK "pedrinho"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;
WiFiServer server(80);

const long utcOffsetInSeconds = -3600 * 3;  // Fuso horário GMT-3 (Brasil)

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

unsigned long startTime = 0;
unsigned long stopTime = 0;
unsigned long count = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);

  Serial.println();
  Serial.println();
  Serial.print(F("Connecting to "));
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Conectado à rede WiFi");
  Serial.println("Endereço IP: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();  // Inicializar o cliente NTP

  server.begin();
  Serial.println("Servidor iniciado");

  startTime = millis();
}

void loop() {
  timeClient.update();  // Atualizar a hora

  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  Serial.println("Novo cliente conectado");

  String req = client.readStringUntil('\r');
  Serial.println(F("request: "));
  Serial.println(req);

  int val;
  if (req.indexOf(F("/gpio/0")) != -1) {
    val = 0;
  } else if (req.indexOf(F("/gpio/1")) != -1) {
    val = 1;
  } else if (req.indexOf(F("/start")) != -1) {
    startTime = millis();
    stopTime = 0;
    val = digitalRead(LED_BUILTIN);
  } else if (req.indexOf(F("/stop")) != -1) {
    stopTime = millis();
    val = digitalRead(LED_BUILTIN);
  } else {
    Serial.println(F("invalid request"));
    val = digitalRead(LED_BUILTIN);
  }

  digitalWrite(LED_BUILTIN, val);

  while (client.available()) {
    client.read();
  }

  client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n"));

  // Exibir a hora atual
  client.print(F("<p>Hora atual: "));
  client.print(timeClient.getFormattedTime());
  client.print(F("</p>"));

  // Exibir o contador
  unsigned long elapsed = (stopTime > 0) ? stopTime - startTime : millis() - startTime;
  count = elapsed / 1000;
  client.print(F("<p>Contador: "));
  client.print(count);
  client.print(F(" segundos</p>"));

  // Exibir a hora de parada do contador
  // Exibir a hora de parada do contador
if (stopTime > 0) {
    time_t stopTimeSeconds = stopTime / 1000;
    client.print(F("<p>Hora de parada: "));
    client.print(ctime(&stopTimeSeconds));
    client.print(F("</p>"));

    // Exibir a diferença entre a hora de início e a hora de parada
    time_t startTimeSeconds = startTime / 1000;
    client.print(F("<p>Diferença: "));
    client.print(elapsed / 1000);
    client.print(F(" segundos</p>"));
}

  client.print(F("<br><br>Click <a href='/gpio/1'>here</a> to switch LED GPIO on, or <a href='/gpio/0'>here</a> to switch LED GPIO off."));
  client.print(F("<br><a href='/start'>Start Counter</a> <a href='/stop'>Stop Counter</a>"));
  client.print(F("</html>"));

  Serial.println("Resposta enviada ao cliente");
  Serial.println("Disconnecting from client");
}