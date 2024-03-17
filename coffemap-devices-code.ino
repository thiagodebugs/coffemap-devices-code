#include <Arduino.h>
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

// Path do RTDB
uint64_t chipid = 0;
String path = "/devices/";

bool signupOK = false;

void setup() {
  // Inicialização da comunicação serial
  Serial.begin(115200);
  ss.begin(GPSBaud);

  Serial.println("Inicializando...");
  Serial.printf("TinyGPSPlus v%s\n", TinyGPSPlus::libraryVersion());
  Serial.printf("Cliente Firebase v%s\n", FIREBASE_CLIENT_VERSION);

  // Conexão do WiFi
  Serial.print("Conectando ao Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.printf("Wi-Fi Conectado! IP: %s\n", WiFi.localIP().toString().c_str());

  // Configuração do Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Autenticação anonima no Firebase (signUp)
  if (Firebase.signUp(&config, &auth, "", "")) {
    signupOK = true;
    Serial.println("Autenticação Ok!");
  } else {
    Serial.printf("Erro de autenticação: %s\n",
                  config.signer.signupError.message.c_str());
  }

  // Configuração do callback para a geração de token
  config.token_status_callback = tokenStatusCallback;

  // Controle de reconexão WiFi
  Firebase.reconnectNetwork(true);
  Firebase.begin(&config, &auth);

  // Configuração do path do RTDB
  chipid = ESP.getEfuseMac();
  path += String(chipid, HEX);
}

void loop() {
  // Verifica se o GPS está disponível
  if (ss.available() > 0) {
    // Verifica se o GPS e o Firebase está pronto
    if (allValid()) {
      // Cria um objeto Firebase Json para armazenar os dados
      FirebaseJson json;
      json.set("timestamp/.sv", "timestamp");
      json.set("latitude", String(gps.location.lat(), 6));
      json.set("longitude", String(gps.location.lng(), 6));

      // Insere os dados do GPS no Firebase
      if (Firebase.pushJSON(fbdo, path.c_str(), json)) {
        Serial.println("Dados inseridos com sucesso!");
        // Exibe as informações do GPS
        displayInfo();
      } else {
        Serial.printf("Erro ao inserir dados: %s\n",
                      fbdo.errorReason().c_str());
      }

      // Intervalo de 10 segundos
      delay(10000);
    }
  } else if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println("Sem dados do GPS, verifique e/ou reinicie a conexão.");
    while (true)
      ;
  }
}

bool allValid() {
  return gps.encode(ss.read()) && gps.location.isValid() &&
         gps.date.isValid() && gps.time.isValid() && Firebase.ready() &&
         signupOK;
}

void displayInfo() {
  Serial.printf("Location: %s, %s | Date: %d/%d/%d | Time: %d:%d:%d\n",
                String(gps.location.lat(), 6), String(gps.location.lng(), 6),
                gps.date.month(), gps.date.day(), gps.date.year(),
                gps.time.hour(), gps.time.minute(), gps.time.second());
}