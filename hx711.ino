#include <Arduino.h>
#include <WiFi.h>
#include "HX711.h"
#include <Firebase.h>

// Wi-Fi and Firebase Configuration
#define WIFI_SSID "archer_2.4G"
#define WIFI_PASSWORD "05132000"
#define FIREBASE_URL "https://firext-system-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_DOCK_ID "-ONINaRvlfmil9bk230u"
#define FIREBASE_DOCKS_PATH "docks"

// HX711 Configuration
#define LOADCELL_DOUT_PIN 23
#define LOADCELL_SCK_PIN 18
#define CALIBRATION_FACTOR -1000.0
#define READINGS_PER_SAMPLE 5
#define DELAY_BETWEEN_READINGS 100  // milliseconds

HX711 scale;
Firebase fb(FIREBASE_URL);
unsigned long lastReading = 0;
bool dockVerified = false;

// Function to check if dock ID exists in database
bool checkDockExists() {
  Serial.print("Verifying dock ID: ");
  Serial.println(FIREBASE_DOCK_ID);
  
  // Create the path to check
  String path = String(FIREBASE_DOCKS_PATH) + "/" + String(FIREBASE_DOCK_ID);
  
  // Try to get any data from this path to verify it exists
  String result = fb.getString(path);
  if (result != "null" && result.length() > 0) {
    Serial.println("Dock ID verified in database");
    return true;
  } else {
    Serial.println("ERROR: Dock ID not found in database");
    return false;
  }
}

// Function to send weight data to Firebase
void sendWeightToFirebase(float weight_kg) {
  // First verify if we haven't checked yet
  if (!dockVerified) {
    dockVerified = checkDockExists();
    if (!dockVerified) {
      Serial.println("Cannot send data - invalid dock ID");
      return;
    }
  }
  
  // If we reach here, the dock ID is verified
  String path = String(FIREBASE_DOCKS_PATH) + "/" + String(FIREBASE_DOCK_ID) + "/weight";
  
  if (fb.setFloat(path, weight_kg)) {
    Serial.println("Weight data sent to Firebase");
  } else {
    Serial.println("Failed to send data to Firebase");
  }
}

void setup() {
  Serial.begin(115200);
  
  // Initialize WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi connected");
  
  // Initialize scale
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(CALIBRATION_FACTOR);
  
  Serial.println("Taring scale...");
  scale.tare();
  Serial.println("Scale ready | 't'=tare | 'c'=calibrate");
  
  // Verify dock ID on startup
  dockVerified = checkDockExists();
}

void loop() {
  // Read scale if ready
  if (scale.is_ready()) {
    float weight_g = scale.get_units(READINGS_PER_SAMPLE);
    float weight_kg = weight_g / 1000.0;  // Convert to kilograms
    
    // Display with precision
    Serial.print("Weight: ");
    Serial.print(weight_kg, 3);  // 3 decimal places for kg
    Serial.println(" kg");
    
    // Send to Firebase with ID verification
    sendWeightToFirebase(weight_kg);
  }
  
  // Handle commands
  if (Serial.available()) {
    char cmd = Serial.read();
    
    if (cmd == 't') {
      scale.tare();
      Serial.println("Scale tared");
    } 
    else if (cmd == 'c') {
      calibrationMode();
    }
    else if (cmd == 'v') {
      // Force re-verification of dock ID
      dockVerified = checkDockExists();
    }
  }
  
  delay(DELAY_BETWEEN_READINGS);
}

void calibrationMode() {
  Serial.println("Calibration mode | '+'/'-'=adjust | 'x'=exit");
  float calibration_factor = CALIBRATION_FACTOR;
  bool exit_cal = false;
  
  while (!exit_cal) {
    scale.set_scale(calibration_factor);
    float weight_g = scale.get_units(10);
    float weight_kg = weight_g / 1000.0;
    
    Serial.print("Reading: ");
    Serial.print(weight_kg, 3);
    Serial.print(" kg | Factor: ");
    Serial.println(calibration_factor);
    
    if (Serial.available()) {
      char key = Serial.read();
      if (key == '+') calibration_factor += 10;
      else if (key == '-') calibration_factor -= 10;
      else if (key == 'x') {
        exit_cal = true;
        scale.set_scale(calibration_factor);
        Serial.print("New calibration factor: ");
        Serial.println(calibration_factor);
      }
    }
    delay(500);
  }
}
