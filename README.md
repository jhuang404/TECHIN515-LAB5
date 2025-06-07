# TECHIN515 Lab 5 - Edge-Cloud Offloading

This project implements an edge-cloud offloading strategy for gesture recognition using an ESP32 microcontroller and Microsoft Azure cloud services. The system performs inference locally when confidence is high and offloads computation to the cloud when confidence is low.

## Project Structure

```
.
├── ESP32_to_cloud/             # ESP32 Arduino code
│   └── ESP32_to_cloud.ino      # Main ESP32 sketch
├── trainer_scripts/            # Training scripts
│   ├── train.ipynb             # Model training script
│   └── register_model.ipynb    # Model registration script
├── inference_scripts/          # Inference scripts
│   └── score.py                # Azure ML inference script
├── app/                        # Web app for model deployment
│   ├── wand_model.h5           # Trained model (generated after training)
│   ├── app.py                  # Flask web application
│   └── requirements.txt        # Python dependencies
├── data/                       # Training data directory
│   ├── O/                      # O-shape gesture samples
│   ├── V/                      # V-shape gesture samples
│   ├── Z/                      # Z-shape gesture samples
│   └── S/                      # S-shape gesture samples
└── README.md                   # This file
```

## Features

- **Local Inference**: ESP32 performs gesture classification locally using a simplified neural network
- **Cloud Offloading**: When local confidence is below threshold (80%), raw sensor data is sent to cloud for inference
- **Real-time Processing**: 50Hz sampling rate with 100-sample sequences
- **Visual Feedback**: RGB LED indicates gesture recognition results with different colors and blink patterns
- **Robust Communication**: HTTP POST requests with JSON payload for cloud communication

## Hardware Requirements

- ESP32 development board
- MPU6050 accelerometer/gyroscope sensor
- RGB LED (or 3 separate LEDs for Red, Green, Blue)
- Breadboard and jumper wires

## Software Requirements

- Arduino IDE with ESP32 board support
- Python 3.8+ with required packages
- Microsoft Azure account
- Required Arduino libraries:
  - Adafruit MPU6050
  - Adafruit Sensor
  - ArduinoJson
  - WiFi (built-in)
  - HTTPClient (built-in)

## Setup Instructions

### 1. Azure Setup

1. Create Resource Group in Azure portal
2. Create Machine Learning workspace
3. Create compute instance for training
4. Upload training data to Azure Blob storage
5. Train model using `train.ipynb` in Azure ML Studio
6. Register model using `register_model.ipynb`

### 2. Local Web App Setup

```bash
cd app/
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
pip install -r requirements.txt
python app.py
```

### 3. ESP32 Configuration

1. Update WiFi credentials in `ESP32_to_cloud.ino`
2. Update `serverUrl` to point to your local web app
3. Upload code to ESP32 using Arduino IDE

## Usage

1. Start the Flask web application
2. Power on the ESP32 device
3. Perform gestures with the magic wand
4. Observe LED feedback and serial monitor output

## LED Indicators

- **Yellow**: Recording gesture
- **Green**: V gesture detected
- **Blue**: O gesture detected
- **Magenta**: Z gesture detected
- **White**: S gesture detected
- **Red**: Error or connection issue
- **Blink Pattern**: Indicates confidence level
  - Solid: High confidence (>90%)
  - Slow blink: Medium confidence (70-90%)
  - Fast blink: Low confidence (<70%)

## Configuration Parameters

- `CONFIDENCE_THRESHOLD`: 80.0 (adjustable threshold for cloud offloading)
- `SEQUENCE_LENGTH`: 100 samples per gesture
- `SAMPLE_RATE`: 50Hz sampling frequency
- `GESTURE_THRESHOLD`: 2.0g acceleration threshold to start recording

## API Endpoints

### Local Flask App

- `GET /`: Health check and API information
- `POST /predict`: Gesture prediction endpoint
- `GET /health`: Service health status
- `GET /model-info`: Model information

### Request Format

```json
{
  "data": [x1, y1, z1, x2, y2, z2, ..., x100, y100, z100]
}
```

### Response Format

```json
{
  "gesture": "V",
  "confidence": 85.67,
  "all_predictions": {
    "V": 85.67,
    "O": 10.23,
    "Z": 3.45,
    "S": 0.65
  }
}
```

## Model Architecture

The neural network model uses:
- Input layer: 300 features (100 samples × 3 axes)
- Hidden layer 1: 128 neurons with ReLU activation + 30% dropout
- Hidden layer 2: 64 neurons with ReLU activation + 20% dropout
- Output layer: 4 neurons with softmax activation (number of gesture classes)

## Data Flow

1. **Sensor Reading**: MPU6050 captures accelerometer data at 50Hz
2. **Local Processing**: ESP32 performs simple feature extraction and classification
3. **Decision Making**: If confidence < 80%, send to cloud; otherwise use local result
4. **Cloud Processing**: Flask app receives raw data, normalizes it, and runs full neural network
5. **Response**: Cloud returns gesture prediction with confidence score
6. **Actuation**: LED provides visual feedback based on recognized gesture

## Troubleshooting

### Common Issues

1. **WiFi Connection Failed**
   - Check SSID and password
   - Ensure 2.4GHz network (ESP32 doesn't support 5GHz)

2. **HTTP Request Failed**
   - Verify server URL and port
   - Check firewall settings
   - Ensure Flask app is running

3. **Low Accuracy**
   - Collect more training data
   - Increase sequence length
   - Adjust gesture threshold

4. **Model Loading Error**
   - Verify model file path
   - Check TensorFlow version compatibility
   - Ensure all dependencies are installed

## Performance Optimization

- **Local Inference**: Simplified model reduces computation time
- **Adaptive Offloading**: Only sends data to cloud when necessary
- **Efficient Communication**: JSON compression for network requests
- **Memory Management**: Fixed-size buffers prevent memory fragmentation

## Contributing

1. Fork the repository
2. Create feature branch
3. Make changes with appropriate tests
4. Submit pull request with detailed description

## License

This project is developed for educational purposes as part of TECHIN515 coursework.
