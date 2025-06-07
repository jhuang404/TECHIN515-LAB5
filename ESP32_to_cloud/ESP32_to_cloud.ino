#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";


const char* serverUrl = "http://127.0.0.1:8000/predict";  

// Confidence threshold for cloud offloading
#define CONFIDENCE_THRESHOLD 80.0

// LED pins
#define LED_RED 2
#define LED_GREEN 4
#define LED_BLUE 5

// MPU6050 sensor
Adafruit_MPU6050 mpu;

// Gesture detection parameters
#define SEQUENCE_LENGTH 100
#define SAMPLE_RATE 50  // 50Hz sampling rate
#define GESTURE_THRESHOLD 2.0  // acceleration threshold to start recording

// Data storage
float features[SEQUENCE_LENGTH * 3];  // x, y, z for each sample
int sample_count = 0;
bool recording = false;
unsigned long last_sample_time = 0;

// Local gesture classes (should match your trained model)
const char* gesture_labels[] = {"V", "O", "Z", "S"};
const int num_classes = 4;

// Simple neural network weights (you would need to extract these from your trained model)
// For demonstration purposes, using placeholder values
// In a real implementation, you'd need to convert your TensorFlow model to C++
float input_weights[SEQUENCE_LENGTH * 3][64];
float hidden_weights[64][32];
float output_weights[32][4];
float input_bias[64];
float hidden_bias[32];
float output_bias[4];

void setup() {
  Serial.begin(115200);
  
  // Initialize LEDs
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  
  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  
  Serial.println("Magic Wand ready!");
  turnOffAllLEDs();
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  
  // Calculate total acceleration
  float total_accel = sqrt(a.acceleration.x * a.acceleration.x + 
                          a.acceleration.y * a.acceleration.y + 
                          a.acceleration.z * a.acceleration.z);
  
  // Check if gesture is starting
  if (!recording && total_accel > GESTURE_THRESHOLD) {
    startRecording();
  }
  
  // Record samples if recording is active
  if (recording && millis() - last_sample_time >= (1000 / SAMPLE_RATE)) {
    recordSample(a.acceleration.x, a.acceleration.y, a.acceleration.z);
    last_sample_time = millis();
  }
  
  // Check if recording is complete
  if (recording && sample_count >= SEQUENCE_LENGTH) {
    processGesture();
  }
  
  delay(10);
}

void startRecording() {
  recording = true;
  sample_count = 0;
  Serial.println("Starting gesture recording...");
  setLED(255, 255, 0); // Yellow LED to indicate recording
}

void recordSample(float x, float y, float z) {
  if (sample_count < SEQUENCE_LENGTH) {
    features[sample_count * 3] = x;
    features[sample_count * 3 + 1] = y;
    features[sample_count * 3 + 2] = z;
    sample_count++;
  }
}

void processGesture() {
  recording = false;
  Serial.println("Processing gesture...");
  
  // Perform local inference
  int predicted_class;
  float confidence = performLocalInference(predicted_class);
  
  Serial.print("Local inference - Gesture: ");
  Serial.print(gesture_labels[predicted_class]);
  Serial.print(", Confidence: ");
  Serial.print(confidence);
  Serial.println("%");
  
  // Check if confidence is below threshold
  if (confidence < CONFIDENCE_THRESHOLD) {
    Serial.println("Low confidence - sending raw data to server...");
    sendRawDataToServer();
  } else {
    Serial.println("High confidence - using local inference");
    actuateLED(gesture_labels[predicted_class], confidence);
  }
}

float performLocalInference(int &predicted_class) {
  // Simplified local inference (placeholder implementation)
  // In a real implementation, you would need to implement the neural network forward pass
  
  // Normalize features (simple standardization)
  float mean_vals[3] = {0, 0, 0};
  for (int i = 0; i < SEQUENCE_LENGTH; i++) {
    mean_vals[0] += features[i * 3];
    mean_vals[1] += features[i * 3 + 1];
    mean_vals[2] += features[i * 3 + 2];
  }
  mean_vals[0] /= SEQUENCE_LENGTH;
  mean_vals[1] /= SEQUENCE_LENGTH;
  mean_vals[2] /= SEQUENCE_LENGTH;
  
  // Simple pattern matching (placeholder)
  // This is a very simplified approach - in reality you'd implement the full neural network
  float pattern_scores[4] = {0};
  
  // Calculate simple features
  float total_variation = 0;
  for (int i = 1; i < SEQUENCE_LENGTH; i++) {
    total_variation += abs(features[i * 3] - features[(i-1) * 3]);
  }
  
  // Simple heuristic classification (replace with actual model inference)
  if (total_variation > 20) {
    predicted_class = 0; // V gesture
    return 85.0; // High confidence for demo
  } else if (total_variation > 10) {
    predicted_class = 1; // O gesture
    return 75.0; // Medium confidence
  } else {
    predicted_class = 2; // Z gesture
    return 60.0; // Low confidence - will trigger cloud offloading
  }
}

void sendRawDataToServer() {
  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");

  // Build JSON array from features[]
  DynamicJsonDocument doc(8192);
  JsonArray data_array = doc.createNestedArray("data");
  
  for (int i = 0; i < SEQUENCE_LENGTH * 3; i++) {
    data_array.add(features[i]);
  }

  String jsonPayload;
  serializeJson(doc, jsonPayload);

  int httpResponseCode = http.POST(jsonPayload);
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Server response: " + response);

    // Parse the JSON response
    DynamicJsonDocument responseDoc(256);
    DeserializationError error = deserializeJson(responseDoc, response);
    if (!error) {
      const char* gesture = responseDoc["gesture"];
      float confidence = responseDoc["confidence"];

      Serial.println("Server Inference Result:");
      Serial.print("Gesture: ");
      Serial.println(gesture);
      Serial.print("Confidence: ");
      Serial.print(confidence);
      Serial.println("%");
      
      // Actuate LED based on server response
      actuateLED(gesture, confidence);
    } else {
      Serial.print("Failed to parse server response: ");
      Serial.println(error.c_str());
      setLED(255, 0, 0); // Red LED for error
    }
  } else {
    Serial.printf("Error sending POST: %s\n", http.errorToString(httpResponseCode).c_str());
    setLED(255, 0, 0); // Red LED for error
  }

  http.end();
}

void actuateLED(const char* gesture, float confidence) {
  // Turn off all LEDs first
  turnOffAllLEDs();
  
  // Set LED color based on gesture
  if (strcmp(gesture, "V") == 0) {
    setLED(0, 255, 0); // Green for V
  } else if (strcmp(gesture, "O") == 0) {
    setLED(0, 0, 255); // Blue for O
  } else if (strcmp(gesture, "Z") == 0) {
    setLED(255, 0, 255); // Magenta for Z
  } else if (strcmp(gesture, "S") == 0) {
    setLED(255, 255, 255); // White for S
  } else {
    setLED(255, 165, 0); // Orange for unknown
  }
  
  // Blink pattern based on confidence
  if (confidence > 90) {
    // Solid light for high confidence
    delay(2000);
  } else if (confidence > 70) {
    // Slow blink for medium confidence
    for (int i = 0; i < 3; i++) {
      turnOffAllLEDs();
      delay(300);
      actuateLED(gesture, 100); // Recursive call with high confidence to avoid infinite recursion
      delay(300);
    }
  } else {
    // Fast blink for low confidence
    for (int i = 0; i < 5; i++) {
      turnOffAllLEDs();
      delay(150);
      actuateLED(gesture, 100); // Recursive call with high confidence to avoid infinite recursion
      delay(150);
    }
  }
  
  turnOffAllLEDs();
}

void setLED(int red, int green, int blue) {
  analogWrite(LED_RED, red);
  analogWrite(LED_GREEN, green);
  analogWrite(LED_BLUE, blue);
}

void turnOffAllLEDs() {
  setLED(0, 0, 0);
}