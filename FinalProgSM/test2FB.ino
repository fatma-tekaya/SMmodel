#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "MAX30105.h"
#include "Protocentral_MAX30205.h"
#include "spo2_algorithm.h"
#include "heartRate.h"

// Provide the token generation process info.
#include <addons/TokenHelper.h>

// Define WiFi credentials
#define WIFI_SSID "fatmawifi"
#define WIFI_PASSWORD "fatmatekaya"

// Firebase project credentials
#define FIREBASE_HOST "https://smart-watch-e7244-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "TiegvWNG4wOS6f9r6abSWEmhBeSXcgQFpT3LzBOy"

// Initialize the sensors
MAX30105 particleSensor;
MAX30205 tempSensor;

unsigned long sendDataPrevMillis = 0;
unsigned long interval = 20000; // Post data every 20 seconds

int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;
float temperature;

// Buffer for storing raw data
const byte SENSOR_BUFFER_SIZE = 100;
uint32_t irBuffer[SENSOR_BUFFER_SIZE]; // Infrared LED sensor data
uint32_t redBuffer[SENSOR_BUFFER_SIZE]; // Red LED sensor data

// Firebase data object
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

// Unique identifier for the user
String userId = "6655b5d54f262a24c916c164";

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Initialize WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 was not found. Please check wiring/power.");
    while (1);
  }
  else {  Serial.println("MAX30105 was found.");}
  particleSensor.setup();
  
  if (!tempSensor.scanAvailableSensors()) {
    Serial.println("Couldn't find the temperature sensor, please connect the sensor.");
    while (1);
  }else {  Serial.println("tempSensor was found.");}


  // Assign the project host and auth
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;

  // Assign the callback function for the long running token generation task
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - sendDataPrevMillis >= interval) {
    sendDataPrevMillis = currentMillis;

    // Read sensors
    for (byte i = 0; i < SENSOR_BUFFER_SIZE; i++) {
      while (particleSensor.available() == false) // wait until new data is available
        particleSensor.check(); // Check the sensor

      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample(); // Move to next sample
    }

    // Calculate SPO2 and heart rate
    maxim_heart_rate_and_oxygen_saturation(irBuffer, SENSOR_BUFFER_SIZE, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

    temperature = tempSensor.getTemperature();

    // Print values for debugging
    Serial.print("SPO2: "); Serial.println(spo2);
    Serial.print("Heart Rate: "); Serial.println(heartRate);
    Serial.print("Temperature: "); Serial.println(temperature);

    // Check if any of the values are zero
    if (spo2 != 0 && heartRate != 0 && temperature != 0) {
      // Prepare data for Firebase
      String path = "/users/" + userId + "/vitals";

      // Use JSON to batch data updates
      FirebaseJson json;
      json.set("spo2", spo2);
      json.set("heartRate", heartRate);
      json.set("temp", temperature);

      if (Firebase.RTDB.setJSON(&firebaseData, path, &json)) {
        Serial.println("Data updated successfully");
      } else {
        Serial.println("Failed to update data, reason: " + firebaseData.errorReason());
      }
    } else {
      Serial.println("Sensor readings are zero, not sending data");
    }
  }
  
}
