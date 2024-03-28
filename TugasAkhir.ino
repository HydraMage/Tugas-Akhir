#include <DHT.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

#define DHTPIN D2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Pin untuk LED indikator nyala (success)
const int ledSuccessPin = D4;


const char* ssid = "KopiBang.id 2";
const char* password = "bukajam8pagi";
const char* mqtt_server = "test.mosquitto.org";

const char* topicSensorData = "HydraMage/data";

const float R0_NO2 = 76.63;
const float m_NO2 = -0.42;
const float b_NO2 = 37.97; 

WiFiClient espClient;
PubSubClient client(espClient);

int mqPin = A0;
int pm10Pin = D5;
int pm25Pin = D8;

void setup() {
  Serial.begin(9600);
  dht.begin();

  // Set pin mode untuk LED indikator
  pinMode(ledSuccessPin, OUTPUT);


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

  // Menampilkan hasil sensor secara ringkas di Serial Monitor
  Serial.print("Temp: ");
  Serial.print(temperature);
  Serial.print("°C, Hum: ");
  Serial.print(humidity);
  Serial.print("%, NO2: ");
  Serial.print(NO2Concentration);
  Serial.print(", PM10: ");
  Serial.print(readPM10Concentration(pm10Value));
  Serial.print(", PM2.5: ");
  Serial.println(readPM25Concentration(pm25Value));

  // Membuat objek DynamicJsonDocument
  DynamicJsonDocument sensorData(256);
  sensorData["temperature"] = temperature;
  sensorData["humidity"] = humidity;
  sensorData["air_quality"] = NO2Concentration;
  sensorData["pm10"] = readPM10Concentration(pm10Value);
  sensorData["pm25"] = readPM25Concentration(pm25Value);

  // Mengkonversi objek JSON menjadi string
  char jsonBuffer[256];
  serializeJson(sensorData, jsonBuffer);

  // Kirim data ke broker MQTT
  client.publish(topicSensorData, jsonBuffer);

  // Nyalakan LED indikator saat berhasil baca sensor
  digitalWrite(ledSuccessPin, HIGH);
  delay(1000);
  digitalWrite(ledSuccessPin, LOW);

  delay(2000);
}

float readNO2Concentration() {
  int rawValue = analogRead(mqPin);
  float resistance = (1023.0 / rawValue) * 5.0 - 1.0;
  float NO2Concentration = (resistance - R0_NO2) / m_NO2 + b_NO2;


  return NO2Concentration;
}

// Fungsi untuk menghitung konsentrasi PM10
float readPM10Concentration(int rawValue) {
  // Konstanta kalibrasi PM10
  const float A_PM10 = 10.0;
  const float B_PM10 = 2.2;
  const float C_PM10 = 100.0;

  // Konversi ADC ke tegangan
  float voltage = (rawValue / 4095.0) * 3.3;

  // Periksa nilai ADC
  if (voltage < 0.0 || voltage > 3.3) {
    return -1.0; // Nilai ADC tidak valid
  }

  // Hitung rasio hamburan
  float scatteringRatio = (voltage / 1.5) * 100.0;

  // Hitung konsentrasi PM10
  // Hitung konsentrasi PM10
float concentration = (scatteringRatio - A_PM10) / B_PM10 * C_PM10;

  return concentration;
}

// Fungsi untuk menghitung konsentrasi PM2.5
float readPM25Concentration(int rawValue) {
  // Konstanta kalibrasi PM2.5
  const float A_PM25 = 5.0;
  const float B_PM25 = 2.5;
  const float C_PM25 = 100.0;

  // Konversi ADC ke tegangan
  float voltage = (rawValue / 4095.0) * 3.3;

  // Periksa nilai ADC
  if (voltage < 0.0 || voltage > 3.3) {
    return -1.0; // Nilai ADC tidak valid
  }

  // Hitung rasio hamburan
  float scatteringRatio = (voltage / 1.5) * 100.0;

  // Hitung konsentrasi PM2.5
  float concentration = (scatteringRatio - A_PM25) / B_PM25 * C_PM25;

  return concentration;
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

