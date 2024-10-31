#include <WiFi.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "certificates.h"

// Definindo os pinos dos sensores
#define DHTPIN 18
#define DHTTYPE DHT22
#define SOIL_MOISTURE_PIN 36
#define LUMINOSITY_PIN 39
#define DS18B20_PIN 4

DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(DS18B20_PIN);
DallasTemperature soilTempSensor(&oneWire);

// Variáveis de WiFi
AsyncWebServer server(80);
AsyncWebSocket ws("/logs");

// Variáveis BLE
BLEServer *pServer = nullptr;
BLEAdvertising *pAdvertising = nullptr;

// Declaração de funções
void setupBLE();
void listWiFiNetworks();
void connectWiFi();
float calibrateSoilMoisture(int rawValue);
void sendLog(String message);

// Definindo UUIDs do serviço e característica BLE
#define SERVICE_UUID "4fafc201-1fb5-459e-8d40-b6b0e6e9b6b2"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

String ssid, password;
String crops[2];
String uuid;

// Funções BLE
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        Serial.println("Cliente BLE conectado");
    }

    void onDisconnect(BLEServer* pServer) {
        Serial.println("Cliente BLE desconectado");
        pAdvertising->start();
        Serial.println("Publicidade BLE reiniciada.");
    }
};

class MyCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        String value = pCharacteristic->getValue();
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, value);

        if (error) {
            Serial.print("Falha ao parsear JSON: ");
            Serial.println(error.f_str());
            return;
        }

        if (doc.containsKey("crops") && doc["crops"].is<JsonArray>() && doc["crops"].size() >= 2) {
            crops[0] = doc["crops"][0].as<String>();
            crops[1] = doc["crops"][1].as<String>();
            Serial.println("Culturas recebidas: " + crops[0] + ", " + crops[1]);
        } else {
            Serial.println("Erro: Dados de 'crops' incompletos ou ausentes");
            crops[0] = "";
            crops[1] = "";
        }

        if (doc.containsKey("uuid")) {
            uuid = doc["uuid"].as<String>();
        } else {
            Serial.println("Erro: UUID ausente");
            uuid = "";
        }

        Serial.println("Dados recebidos via BLE:");
        Serial.print("Cultura 1: "); Serial.println(crops[0].isEmpty() ? "Não recebida" : crops[0]);
        Serial.print("Cultura 2: "); Serial.println(crops[1].isEmpty() ? "Não recebida" : crops[1]);
        Serial.print("UUID: "); Serial.println(uuid.isEmpty() ? "Não recebido" : uuid);
    }
};

void connectWebSocket() {
    ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
        if (type == WS_EVT_CONNECT) {
            Serial.printf("Cliente conectado: %u\n", client->id());
        } else if (type == WS_EVT_DISCONNECT) {
            Serial.printf("Cliente desconectado: %u\n", client->id());
        }
    });
    server.addHandler(&ws);
    server.begin();
    Serial.println("Servidor WebSocket inicializado.");
}

void sendLog(String message) {
    ws.textAll(message);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  dht.begin();
  soilTempSensor.begin();
  listWiFiNetworks();
  connectWiFi();

  setupBLE();
  connectWebSocket();
}

void loop() {
  ws.cleanupClients();

  // Exemplo de lógica de coleta de dados e envio de logs
  float soilMoisture = calibrateSoilMoisture(analogRead(SOIL_MOISTURE_PIN));
  float airHumidity = dht.readHumidity();
  float airTemp = dht.readTemperature();
  soilTempSensor.requestTemperatures();
  float soilTemp = soilTempSensor.getTempCByIndex(0);

  String logMessage = "Umidade do Solo: " + String(soilMoisture) + "% | " +
                      "Temperatura do Solo: " + String(soilTemp) + "°C | " +
                      "Umidade do Ar: " + String(airHumidity) + "% | " +
                      "Temperatura do Ar: " + String(airTemp) + "°C";
  
  Serial.println(logMessage);
  sendLog(logMessage);

  delay(2000);
}

void listWiFiNetworks() {
  Serial.println("Procurando redes WiFi disponíveis...");
  int numNetworks = WiFi.scanNetworks();

  if (numNetworks == 0) {
    Serial.println("Nenhuma rede encontrada.");
    return;
  }

  Serial.println("Redes WiFi disponíveis:");
  for (int i = 0; i < numNetworks; ++i) {
    Serial.print(i);
    Serial.print(": ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" (");
    Serial.print(WiFi.RSSI(i));
    Serial.println(" dBm)");
    delay(10);
  }

  Serial.println("Digite o nome (SSID) da rede que você deseja conectar:");
  while (Serial.available() == 0) {}
  ssid = Serial.readString();
  ssid.trim();

  Serial.println("Digite a senha da rede WiFi:");
  while (Serial.available() == 0) {}
  password = Serial.readString();
  password.trim();
}

void connectWiFi() {
  Serial.println("Conectando à rede WiFi...");
  WiFi.begin(ssid.c_str(), password.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado com sucesso!");
}

void setupBLE() {
    BLEDevice::init("ESP32_WiFi_Credentials");
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);
    BLECharacteristic *pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_INDICATE
    );

    pCharacteristic->setCallbacks(new MyCallbacks());
    pService->start();
    
    pAdvertising = pServer->getAdvertising();
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->start();

    Serial.println("Bluetooth configurado.");
}

float calibrateSoilMoisture(int rawValue) {
  int dry = 700;
  int wet = 3200;
  return map(rawValue, dry, wet, 0, 100);
}
