/*
 * Smart Irrigation System (IoT-Based Water & Soil Monitoring)
 *
 * This project implements an automated irrigation system using ESP8266.
 * It monitors soil moisture, water level, temperature, and humidity,
 * and controls a water pump accordingly.
 *
 * Features:
 * - Soil moisture monitoring using analog sensor
 * - Water level monitoring
 * - Temperature & humidity sensing using DHT11
 * - Automatic pump control based on soil moisture
 * - Manual pump control via Blynk IoT app
 * - Real-time data monitoring on mobile dashboard
 * - Alert notification for low soil moisture
 *
 * Applications:
 * - Smart Agriculture
 * - Automated Irrigation Systems
 * - Smart City Green Infrastructure
 */

// Blynk Template and Auth Token
#define BLYNK_TEMPLATE_ID "TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Water & Soil Monitor"
#define BLYNK_AUTH_TOKEN "YOUR_AUTH_TOKEN"

#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>

// WiFi & Blynk Credentials
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "YOUR_WIFI";     // Wi-Fi SSID
char pass[] = "YOUR_PASSWORD"; // Wi-Fi Password

// Sensor Pins
#define SOIL_SENSOR_POWER D6   // Soil moisture sensor power pin
#define WATER_SENSOR_POWER D5  // Water level sensor power pin
#define ANALOG_PIN A0          // Analog sensor pin (shared for soil & water)
#define RELAY_PIN D2           // Pump Relay

// DHT Sensor
#define DHTPIN D4     
#define DHTTYPE DHT11  
DHT dht(DHTPIN, DHTTYPE);  

// Sensor Calibration
#define SOIL_DRY 700   
#define SOIL_WET 350  
#define WATER_DRY 710  
#define WATER_WET 360  

bool manualPumpState = false;  // Track manual pump state
BlynkTimer timer;
bool soilAlertSent = false;  // Track if an alert was already sent

// *Read Sensors & Control Pump*
void sendSensorData() {
  // *Read Soil Moisture*
  digitalWrite(SOIL_SENSOR_POWER, HIGH);
  delay(100);
  int soilValue = analogRead(ANALOG_PIN);
  digitalWrite(SOIL_SENSOR_POWER, LOW);
  int soilMoisture = map(soilValue, SOIL_DRY, SOIL_WET, 0, 100);
  soilMoisture = constrain(soilMoisture, 0, 100);
  Blynk.virtualWrite(V2, soilMoisture);

  // *Send Alert if Soil Moisture is Below 5%*
  if (soilMoisture <= 5 && !soilAlertSent) {
    Blynk.logEvent("low_soil_moisture", "⚠️ Warning! Soil Moisture is Very Low! ⚠️");
    soilAlertSent = true;  // Prevent repeated notifications
  }
  if (soilMoisture > 5) {
    soilAlertSent = false;  // Reset alert when soil moisture is above 5%
  }

  // *Read Water Level (Only for Monitoring)*
  digitalWrite(WATER_SENSOR_POWER, HIGH);
  delay(100);
  int waterValue = analogRead(ANALOG_PIN);
  digitalWrite(WATER_SENSOR_POWER, LOW);
  int waterLevel = map(waterValue, WATER_DRY, WATER_WET, 0, 100);
  waterLevel = constrain(waterLevel, 0, 100);
  Blynk.virtualWrite(V1, waterLevel);

  // *Read Temperature & Humidity*
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  if (!isnan(temperature) && !isnan(humidity)) {
    Blynk.virtualWrite(V3, temperature);
    Blynk.virtualWrite(V4, humidity);
    Serial.print("Temp: "); Serial.print(temperature); Serial.println("°C");
    Serial.print("Humidity: "); Serial.print(humidity); Serial.println("%");
  } else {
    Serial.println("Failed to read from DHT sensor!");
  }

  // *Automatic Pump Control (Only Based on Soil Moisture)*
  if (!manualPumpState) {
    if (soilMoisture <= 5) {  
      digitalWrite(RELAY_PIN, LOW);  // Turn ON Pump
      Serial.println("Pump ON (Auto Mode: Low Soil Moisture)");
    } else if (soilMoisture >= 10) {  
      digitalWrite(RELAY_PIN, HIGH);  // Turn OFF Pump
      Serial.println("Pump OFF (Auto Mode: Soil Moisture OK)");
    }
  }

  Serial.print("Soil Moisture: "); Serial.print(soilMoisture); Serial.println("%");
  Serial.print("Water Level: "); Serial.print(waterLevel); Serial.println("%");
}

// *Manual Pump Control from Blynk*
BLYNK_WRITE(V5) {  
  int pumpState = param.asInt();
  manualPumpState = (pumpState == 1);
  
  if (manualPumpState) {
    digitalWrite(RELAY_PIN, LOW);  // Pump ON (Manual)
    Serial.println("Pump ON (Manual Mode)");
  } else {
    digitalWrite(RELAY_PIN, HIGH);  // Pump OFF (Manual)
    Serial.println("Pump OFF (Manual Mode)");
  }
}

void setup() {
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass);
  
  pinMode(SOIL_SENSOR_POWER, OUTPUT);
  pinMode(WATER_SENSOR_POWER, OUTPUT);
  pinMode(ANALOG_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  
  digitalWrite(RELAY_PIN, HIGH);  // Ensure Pump is OFF at startup
  dht.begin();
  timer.setInterval(3000L, sendSensorData);  
}

void loop() {
  Blynk.run();
  timer.run();
}
