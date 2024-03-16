#include <Arduino.h>
#include <ArduinoJson.h>
#include <FirebaseESP32.h>
#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>
#include <WiFi.h>

// Inclui as bibliotecas necessárias para a geração de token e manipulação de
// dados no Firebase
#include <addons/RTDBHelper.h>
#include <addons/TokenHelper.h>

// Configuração do GPS
static const int RXPin = 16, TXPin = 17;
static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);

// Credenciais WiFi
#define WIFI_SSID "Casa"
#define WIFI_PASSWORD "S3msenh@"

// Chave da API e URL do RTDB
#define API_KEY "AIzaSyCZLnWY2-qzWmVd_8UXaPqo-LxpiXs3VVo"
#define DATABASE_URL "https://coffeemap-afbd1-default-rtdb.firebaseio.com"

// Objetos Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;

void setup() {
  Serial.begin(115200);
  ss.begin(GPSBaud);

  // Inicialização do GPS e conexão WiFi
  Serial.println(F("DeviceExample.ino"));
  Serial.println(
      F("Uma demonstração simples do TinyGPSPlus com um módulo GPS anexado"));
  Serial.print(F("Testando a biblioteca TinyGPSPlus v. "));
  Serial.println(TinyGPSPlus::libraryVersion());
  Serial.println();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Conectado com IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Cliente Firebase v%s\n\n", FIREBASE_CLIENT_VERSION);

  // Configuração do Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Tentativa de cadastro no Firebase
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("SignUp Ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  // Configuração do callback para a geração de token
  config.token_status_callback = tokenStatusCallback;

  // Controle de reconexão WiFi
  Firebase.reconnectNetwork(true);
  Firebase.begin(&config, &auth);
}

void loop() {
  if (ss.available() > 0) {
    if (gps.encode(ss.read()) && gps.location.isValid() && gps.date.isValid() &&
        gps.time.isValid()) {
      if (Firebase.ready() && signupOK) {
        if (Firebase.setTimestamp(fbdo, "devices/1")) {
          String path = "devices/1"; // + fbdo.to<int>();

          FirebaseJson json;

          json.set("lat", String(gps.location.lat(), 6));
          json.set("long", String(gps.location.lng(), 6));

          if (Firebase.setJSON(fbdo, path, json)) {
            displayInfo();
            Serial.println("Dados enviado com sucesso");
          } else {
            displayInfo();
            Serial.println("Falha ao enviar dados");
            Serial.println(fbdo.errorReason());
          }
        }
      }
    }
  } else if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println(F("No GPS detected: check wiring."));
    while (true)
      ;
  }
}

void displayInfo() {
  Serial.print(F("Location: "));
  if (gps.location.isValid()) {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  } else {
    Serial.print(F("INVALID"));
  }
  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid()) {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  } else {
    Serial.print(F("INVALID"));
  }
  Serial.print(F(" "));
  if (gps.time.isValid()) {
    if (gps.time.hour() < 10) {
      Serial.print(F("0"));
    }
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) {
      Serial.print(F("0"));
    }
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) {
      Serial.print(F("0"));
    }
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) {
      Serial.print(F("0"));
    }
    Serial.print(gps.time.centisecond());
  } else {
    Serial.print(F("INVALID"));
  }
  Serial.println();
}