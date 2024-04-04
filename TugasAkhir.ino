#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

#define DHTPIN D2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "LANTJAR";
const char* password = "seksekakulali";
const char* mqtt_server = "test.mosquitto.org";

const char* topicSensorData = "HydraMage/data";

const float R0_NO2 = 76.63;
const float m_NO2 = -0.42;
const float b_NO2 = 37.97;

WiFiClient espClient;
PubSubClient client(espClient);

LiquidCrystal_I2C lcd(0x27, 16, 2); // Definisikan objek LCD

int mqPin = A0;
int pm10Pin = D5;
int pm25Pin = D8;
int ledSuccessPin = D7; // Deklarasi pin untuk LED indikator

void setup() {
  Serial.begin(9600);
  dht.begin();
  lcd.init(); // Inisialisasi LCD
  lcd.backlight();

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

  // Menampilkan hasil sensor di Serial Monitor
  Serial.print("Temp: ");
  Serial.print(temperature);
  Serial.print("°C, Hum: ");
  Serial.print(humidity);
  Serial.print("%, NO2: ");
  Serial.print(NO2Concentration);
  Serial.print(", PM10: ");
  Serial.print(readPMConcentration(pm10Value, true)); // Memanggil fungsi untuk PM10
  Serial.print(", PM2.5: ");
  Serial.println(readPMConcentration(pm25Value, false)); // Memanggil fungsi untuk PM2.5

  // Menampilkan hasil sensor di LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print(" C");

  lcd.setCursor(0, 1);
  lcd.print("Hum: ");
  lcd.print(humidity);
  lcd.print("%");

  lcd.setCursor(9, 0);
  lcd.print("PM10: ");
  lcd.print(readPMConcentration(pm10Value, true));

  lcd.setCursor(9, 1);
  lcd.print("PM2.5: ");
  lcd.print(readPMConcentration(pm25Value, false));

  delay(2000);  // Delay untuk menampilkan data di LCD

  // Membuat objek DynamicJsonDocument
  DynamicJsonDocument sensorData(256);
  sensorData["temperature"] = temperature;
  sensorData["humidity"] = humidity;
  sensorData["air_quality"] = NO2Concentration;
  sensorData["pm10"] = readPMConcentration(pm10Value, true);
  sensorData["pm25"] = readPMConcentration(pm25Value, false);

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

float readPMConcentration(int rawValue, bool isPM10) {
  float conversionFactorPM10 = 0.5; // Placeholder, replace with actual value from datasheet for PM10
  float conversionFactorPM2_5 = 0.2; // Placeholder, replace with actual value from datasheet for PM2.5
  float conversionFactor = isPM10 ? conversionFactorPM10 : conversionFactorPM2_5;

  float voltage = rawValue * 5.0 / 1023.0;
  float concentration = voltage * conversionFactor;

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
