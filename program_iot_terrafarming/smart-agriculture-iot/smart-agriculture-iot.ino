#include <WiFi.h>               
#include <WiFiClientSecure.h>    
#include <PubSubClient.h>        
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "certificates.h"

// Definindo os pinos dos sensores
#define DHTPIN 18               // Pino do sensor DHT para umidade e temperatura do ar
#define DHTTYPE DHT22           // Tipo de sensor DHT
#define SOIL_MOISTURE_PIN 36    // VN 39 - Pino do sensor de umidade do solo
#define LUMINOSITY_PIN 36       // VP 36 - Pino do sensor de luminosidade
#define DS18B20_PIN 26          // Pino do sensor de temperatura DS18B20

DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(DS18B20_PIN);           
DallasTemperature soilTempSensor(&oneWire); 

// Variáveis de WiFi e MQTT
WiFiClientSecure espClient;
PubSubClient client(espClient);
String ssid, password;
String crops[3];  // Array para armazenar culturas
String uuid;

// Declaração de funções
void listWiFiNetworks();
void connectWiFi();
void reconnectMQTT();
void sendToMQTT(String topic, String payload);
float calibrateSoilMoisture(int rawValue);
String analyzeStatus(float value, String type);

void setup() {
  Serial.begin(115200);
  delay(1000); 

  dht.begin();  
  soilTempSensor.begin();

  listWiFiNetworks();
  connectWiFi();

  espClient.setCACert(rootCA);
  espClient.setCertificate(certificate);
  espClient.setPrivateKey(privateKey);
  client.setServer(mqttEndpoint, mqttPort); 
  
  // Solicitar culturas e UUID no início
  for (int i = 0; i < 3; i++) {
    Serial.print("Digite a cultura ");
    Serial.print(i + 1);
    Serial.println(" (ex: Milho):");
    while (Serial.available() == 0) {}
    crops[i] = Serial.readString();
    crops[i].trim();
  }

  Serial.println("Digite o UUID da aplicação:");
  while (Serial.available() == 0) {}
  uuid = Serial.readString();
  uuid.trim();

  // Verificar conexão MQTT
  if (!client.connected()) {
    reconnectMQTT();
  }
}

void loop() {
  Serial.println("Iniciando coletas de dados...");

  float soilMoistureAvg = 0, soilTempAvg = 0, airMoistureAvg = 0, airTempAvg = 0, brightnessAvg = 0;

  for (int i = 0; i < 10; i++) {
    float soilMoisture = calibrateSoilMoisture(analogRead(SOIL_MOISTURE_PIN));
    float airHumidity = dht.readHumidity();
    float airTemp = dht.readTemperature(); 
    float luminosity = map(analogRead(LUMINOSITY_PIN), 0, 4095, 0, 100000); 
    
    soilTempSensor.requestTemperatures();
    float soilTemp = soilTempSensor.getTempCByIndex(0);

    // Substituir por zero se não houver leitura válida
    soilMoisture = isnan(soilMoisture) ? 0 : soilMoisture;
    soilTemp = (soilTemp == DEVICE_DISCONNECTED_C || isnan(soilTemp)) ? 0 : soilTemp;
    airHumidity = isnan(airHumidity) ? 0 : airHumidity;
    airTemp = isnan(airTemp) ? 0 : airTemp;
    luminosity = isnan(luminosity) ? 0 : luminosity;

    // Impressão das leituras
    Serial.print("Leitura ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print("Umidade do Solo: " + String(soilMoisture) + "% | ");
    Serial.print("Temperatura do Solo: " + String(soilTemp) + "°C | ");
    Serial.print("Luminosidade: " + String(luminosity) + " Lux | ");
    Serial.print("Umidade do Ar: " + String(airHumidity) + "% | ");
    Serial.println("Temperatura do Ar: " + String(airTemp) + "°C");
    Serial.println("------------------------------");

    soilMoistureAvg += soilMoisture;
    soilTempAvg += soilTemp;
    airMoistureAvg += airHumidity;
    airTempAvg += airTemp;
    brightnessAvg += luminosity;

    delay(2000);
  }

  // Cálculo das médias
  soilMoistureAvg /= 10;
  soilTempAvg /= 10;
  airMoistureAvg /= 10;
  airTempAvg /= 10;
  brightnessAvg /= 10;

  // Análise dos status
  String soilMoistureStatus = analyzeStatus(soilMoistureAvg, "Umidade do Solo");
  String soilTempStatus = analyzeStatus(soilTempAvg, "Temperatura do Solo");
  String brightnessStatus = analyzeStatus(brightnessAvg, "Luminosidade");
  String airMoistureStatus = analyzeStatus(airMoistureAvg, "Umidade do Ar");
  String airTempStatus = analyzeStatus(airTempAvg, "Temperatura do Ar");

  // Verificar e reconectar MQTT se necessário
  if (!client.connected()) {
    reconnectMQTT();
  }

  // Envio das médias para o MQTT
  String cropsArray = "[\"" + crops[0] + "\", \"" + crops[1] + "\", \"" + crops[2] + "\"]";
  sendToMQTT("agriculture/soil/moisture", "{\"moisture\": " + String(soilMoistureAvg) + ", \"status\": \"" + soilMoistureStatus + "\", \"crops\": " + cropsArray + "}");
  sendToMQTT("agriculture/soil/temperature", "{\"temperature\": " + String(soilTempAvg) + ", \"status\": \"" + soilTempStatus + "\", \"crops\": " + cropsArray + "}");
  sendToMQTT("agriculture/brightness", "{\"brightness\": " + String(brightnessAvg) + ", \"status\": \"" + brightnessStatus + "\", \"crops\": " + cropsArray + "}");
  sendToMQTT("agriculture/air/moisture", "{\"moisture\": " + String(airMoistureAvg) + ", \"status\": \"" + airMoistureStatus + "\", \"crops\": " + cropsArray + "}");
  sendToMQTT("agriculture/air/temperature", "{\"temperature\": " + String(airTempAvg) + ", \"status\": \"" + airTempStatus + "\", \"crops\": " + cropsArray + "}");

  client.loop();

  // Impressão das médias finais
  Serial.println("Médias finais:");
  Serial.print("Umidade do Solo: " + String(soilMoistureAvg) + "% | ");
  Serial.print("Temperatura do Solo: " + String(soilTempAvg) + "°C | ");
  Serial.print("Luminosidade: " + String(brightnessAvg) + " Lux | ");
  Serial.print("Umidade do Ar: " + String(airMoistureAvg) + "% | ");
  Serial.println("Temperatura do Ar: " + String(airTempAvg) + "°C");
  Serial.println("------------------------------");

  Serial.println("Deseja realizar uma nova medição agora ou esperar 2 horas?");
  Serial.println("Digite 'N' para nova medição ou 'E' para esperar 2 horas:");

  while (Serial.available() == 0) {} 
  String resposta = Serial.readString();
  resposta.trim();

  if (resposta == "E" || resposta == "e") {
    Serial.println("Esperando por 2 horas...");
    delay(7200000);
  } else if (resposta == "N" || resposta == "n") {
    Serial.println("Nova medição será feita agora.");
  } else {
    Serial.println("Comando não reconhecido. Nova medição será feita.");
  }
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

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Tentando conectar ao MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Conectado ao MQTT.");
    } else {
      Serial.print("Falha na conexão, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5 segundos.");
      delay(5000);
    }
  }
}

void sendToMQTT(String topic, String payload) {
  Serial.println("Enviando mensagem MQTT:");
  Serial.print("Tópico: ");
  Serial.println(topic);
  Serial.print("Payload: ");
  Serial.println(payload);

  if (client.publish(topic.c_str(), payload.c_str())) {
    Serial.println("Mensagem enviada com sucesso.");
  } else {
    Serial.println("Erro ao enviar mensagem MQTT.");
    Serial.println(client.state());
    reconnectMQTT(); // Tentativa de reconectar se falhar
  }
}

float calibrateSoilMoisture(int rawValue) {
  int dry = 0;
  int wet = 2000;
  return map(rawValue, dry, wet, 0, 100);
}

String analyzeStatus(float value, String type) {
  if (type == "Umidade do Solo") {
    if (value < 30) return "Baixa";
    else if (value < 70) return "Normal";
    else return "Alta";
  } else if (type == "Temperatura do Solo" || type == "Temperatura do Ar") {
    if (value < 15) return "Baixa";
    else if (value < 35) return "Normal";
    else return "Alta";
  } else if (type == "Luminosidade") {
    if (value < 10000) return "Baixa";
    else if (value < 50000) return "Normal";
    else return "Alta";
  } else if (type == "Umidade do Ar") {
    if (value < 30) return "Baixa";
    else if (value < 60) return "Normal";
    else return "Alta";
  }
  return "Desconhecido";
}
