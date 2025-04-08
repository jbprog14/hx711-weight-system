# HX711 Weight Scale with Firebase Integration

This project implements a digital weight scale using an ESP32 microcontroller, HX711 load cell amplifier, and Firebase for cloud storage and monitoring. The system reads weight data, converts it to kilograms, and sends it to a Firebase Realtime Database.

## Hardware Requirements

- ESP32 Development Board
- HX711 Load Cell Amplifier
- Load Cell / Weight Sensor
- Power Supply
- Connecting Wires

## Software Dependencies

- Arduino Core for ESP32
- HX711 Library
- WiFi Library
- Firebase Library

## Pin Configuration

- **HX711 Data Out (DOUT)**: Connected to ESP32 GPIO 23
- **HX711 Clock (SCK)**: Connected to ESP32 GPIO 18

## Code Structure and Functionality

### 1. Library Inclusions and Configuration

```cpp
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
```

**Functionality**:

- Imports required libraries for ESP32, WiFi, HX711, and Firebase
- Defines WiFi credentials for network connection
- Configures Firebase database URL and data path
- Sets up pin definitions for HX711 connection
- Defines calibration factor and timing parameters

### 2. Global Variables and Object Initialization

```cpp
HX711 scale;
Firebase fb(FIREBASE_URL);
unsigned long lastReading = 0;
bool dockVerified = false;
```

**Functionality**:

- Creates a `scale` object for the HX711 sensor
- Initializes Firebase with the database URL
- `lastReading` tracks timing between readings
- `dockVerified` tracks whether the Firebase dock ID exists

### 3. Database Verification Function

```cpp
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
```

**Functionality**:

- Checks if the dock ID exists in Firebase before attempting to send data
- Forms the complete path to check in the database
- Makes a request to get string data from the path
- Returns true if data exists (not "null" and not empty)
- Returns false if the ID doesn't exist in the database
- Prints verification status to Serial monitor

### 4. Firebase Data Transmission Function

```cpp
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
```

**Functionality**:

- Ensures dock ID is verified before sending data
- If not verified yet, checks dock existence
- If dock doesn't exist, stops and reports error
- Constructs the complete path for the weight data
- Sends the weight value (in kg) to Firebase
- Reports success or failure of data transmission

### 5. Setup Function

```cpp
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
```

**Functionality**:

- Initializes serial communication at 115200 baud
- Connects to WiFi using provided credentials
- Provides connection status feedback via serial output
- Initializes the HX711 scale with pin configuration
- Sets the calibration factor for weight measurements
- Tares the scale (sets to zero) at startup
- Verifies Firebase dock ID exists at startup

### 6. Main Loop Function

```cpp
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
```

**Functionality**:

- Continuously reads weight from the scale when ready
- Takes average of multiple readings for stability
- Converts raw readings from grams to kilograms
- Displays weight with 3 decimal precision
- Sends weight data to Firebase via the sendWeightToFirebase function
- Processes serial commands:
  - 't': Tare the scale (reset to zero)
  - 'c': Enter calibration mode
  - 'v': Verify the dock ID in Firebase
- Maintains a consistent delay between readings

### 7. Calibration Mode Function

```cpp
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
```

**Functionality**:

- Enters an interactive calibration mode
- Starts with the default calibration factor
- Takes continuous readings and shows the current factor
- Allows adjustment of calibration factor:
  - '+': Increase factor by 10
  - '-': Decrease factor by 10
  - 'x': Exit calibration and save new factor
- Shows real-time weight readings during calibration
- Uses 10 samples per reading for higher accuracy
- Applies the new calibration factor on exit

## Usage Instructions

1. **Basic Operation**:

   - Upon startup, the scale will connect to WiFi, initialize, and tare automatically
   - Place items on the scale to see weight readings in kilograms
   - Weight data is automatically sent to Firebase

2. **Commands**:

   - Send 't' via Serial to tare the scale (zero it)
   - Send 'c' to enter calibration mode
   - Send 'v' to verify the dock ID exists in Firebase

3. **Calibration Process**:
   - Enter calibration mode with 'c'
   - Place a known weight on the scale
   - Use '+' and '-' to adjust until reading matches the known weight
   - Exit with 'x' to save the calibration

## Firebase Database Structure

The system uses the following Firebase database structure:

```
/docks
    /-ONINaRvlfmil9bk230u
        /weight: [float value in kg]
```

## Troubleshooting

- **Inaccurate Readings**: Enter calibration mode and adjust the factor
- **Firebase Connection Issues**: Verify dock ID exists with 'v' command
- **No Readings**: Check HX711 wiring and connections
