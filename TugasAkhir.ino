#include <DHT.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

#define DHTPIN D2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "LANTJAR";
const char* password = "seksekakulali";
const char* mqtt_server = "test.mosquitto.org";  // Misal: "mqtt.eclipse.org"

const char* topicSensorData = "sensor/data";

const float R0_NO2 = 76.63;  // Nilai resistansi pada konsentrasi NO2 = 100 ppm
const float m_NO2 = -0.42;   // Kemiringan garis regresi
const float b_NO2 = 37.97;   // Intersepsi garis regresi

WiFiClient espClient;
PubSubClient client(espClient);

int mqPin = A0;
int pm10Pin = D1;
int pm25Pin = D3;

void setup() {
  Serial.begin(9600);
  dht.begin();

  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  float NO2Concentration = readNO2Concentration();
  int pm10Value = analogRead(pm10Pin);
  int pm25Value = analogRead(pm25Pin);

  // Menghitung konsentrasi PM2.5 dan PM10
  float PM25Concentration = readPMConcentration(pm25Value);
  float PM10Concentration = readPMConcentration(pm10Value);

  // Membuat objek DynamicJsonDocument
  DynamicJsonDocument sensorData(256);
  sensorData["temperature"] = temperature;
  sensorData["humidity"] = humidity;
  sensorData["air_quality"] = NO2Concentration;
  sensorData["pm10"] = PM10Concentration;
  sensorData["pm25"] = PM25Concentration;

  // Mengkonversi objek JSON menjadi string
  char jsonBuffer[256];
  serializeJson(sensorData, jsonBuffer);

  // Kirim data ke broker MQTT
  client.publish(topicSensorData, jsonBuffer);

  delay(2000);
}

float readNO2Concentration() {
  int rawValue = analogRead(mqPin);
  float resistance = (1023.0 / rawValue) * 5.0 - 1.0;
  float NO2Concentration = (resistance - R0_NO2) / m_NO2 + b_NO2;

  // Menampilkan nilai NO2 ke Serial Monitor
  Serial.print("NO2 Raw Value: ");
  Serial.print(rawValue);
  Serial.print(", NO2 Resistance: ");
  Serial.print(resistance);
  Serial.print(", NO2 Concentration: ");
  Serial.println(NO2Concentration);

  return NO2Concentration;
}

float readPMConcentration(int rawValue) {
  // Sesuaikan persamaan konversi berdasarkan karakteristik sensor
  // Contoh persamaan linier sederhana, Anda perlu menyesuaikan sesuai datasheet DSM501A
  float PMConcentration = (rawValue * 0.5);  // Contoh persamaan linier sederhana

  // Menampilkan nilai PM ke Serial Monitor
  Serial.print("PM Raw Value: ");
  Serial.print(rawValue);
  Serial.print(", PM Concentration: ");
  Serial.println(PMConcentration);

  return PMConcentration;
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
