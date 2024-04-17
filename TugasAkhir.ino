#include <U8g2lib.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <SSD1306Wire.h> // Or #include <SSD1306.h> depending on the library version

// Define sensor pins
#define DHTPIN D2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Define WiFi credentials
const char* ssid = "WAFI";
const char* password = "jusjambu";

// Define MQTT server details
const char* mqtt_server = "test.mosquitto.org";
const char* topicSensorData = "HydraMage/data";

// Define calibration constants for NO2 sensor (replace with actual values)
const float R0_NO2 = 76.63;
const float m_NO2 = -0.42;
const float b_NO2 = 37.97;

// Initialize WiFi client and MQTT client objects
WiFiClient espClient;
PubSubClient client(espClient);

// Define display dimensions
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

// Create an SSD1306 display object, specifying I2C pins
SSD1306Wire display(DISPLAY_WIDTH, DISPLAY_HEIGHT, D1, D2); // SCL, SDA pins

// Define analog input pins for sensors
int mqPin = A0;
int pm10Pin = D6;
int pm25Pin = D1;

// Define LED pin for success indicator
int ledSuccessPin = D4;

// Variables for timing
unsigned long previousMillis = 0;        // Variable to store the last time a message was published
const long interval = 2000;              // Interval at which to publish sensor data (milliseconds)

void setup() {
  Serial.begin(9600);
  dht.begin();

  // Initialize OLED display
  display.init();
  display.setFont(ArialMT_Tiny); // Set a suitable font
  display.clearDisplay();

  // Connect to WiFi network
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  unsigned long currentMillis = millis(); // Get the current time

  // Check and reconnect to MQTT if necessary
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Read sensor data
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  float NO2Concentration = readNO2Concentration();
  int pm10Value = analogRead(pm10Pin);
  int pm25Value = analogRead(pm25Pin);

  // Print sensor data to Serial monitor for debugging
  Serial.print("Temp: ");
  Serial.print(temperature);
  Serial.print("°C, Hum: ");
  Serial.print(humidity);
  Serial.print("%, NO2: ");
  Serial.print(NO2Concentration);
  Serial.print(", PM10: ");
  Serial.print(readPMConcentration(pm10Value, true));
  Serial.print(", PM2.5: ");
  Serial.println(readPMConcentration(pm25Value, false));

  // Display sensor data on OLED
  display.firstPage();
  do {
    display.setCursor(0, 10);
    display.print("Temp: ");
    display.print(temperature);
    display.print(" C");
    display.setCursor(0, 20);
    display.print("Hum: ");
    display.print(humidity);
    display.print("%");
    display.setCursor(0, 30);
    display.print("NO2: ");
    display.print(NO2Concentration);
    display.setCursor(0, 40);
    display.print("PM10: ");
    display.print(readPMConcentration(pm10Value, true));
    display.setCursor(0, 50);
    display.print("PM2.5: ");
    display.print(readPMConcentration(pm25Value, false));
  } while (display.nextPage());
  display.display();  // Send the buffer to the display

  // Create a JSON object for sensor data
  DynamicJsonDocument sensorData(256);
  sensorData["temperature"] = temperature;
  sensorData["humidity"] = humidity;
  sensorData["air_quality"] = NO2Concentration;
  sensorData["pm10"] = readPMConcentration(pm10Value, true);
  sensorData["pm25"] = readPMConcentration(pm25Value, false);

  // Convert JSON object to string
  char jsonBuffer[256];
  serializeJson(sensorData, jsonBuffer);

  // Publish data to MQTT broker
  client.publish(topicSensorData, jsonBuffer);

  // Turn on LED indicator when sensor reading is successful
  digitalWrite(ledSuccessPin, HIGH);
  delay(1000);
  digitalWrite(ledSuccessPin, LOW);

  // Check if it's time to publish again
  if (currentMillis - previousMillis >= interval) {
    // Save the last time a message was published
    previousMillis = currentMillis;
  }
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
