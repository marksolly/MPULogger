# MPU6050 Data Logger

A high-performance ESP8266-based data logging system for MPU6050 IMU sensors with real-time web interface and advanced data visualization capabilities.

## Features

### Core Functionality

- **Real-time Sensor Data**: 10Hz sampling of acceleration (X, Y, Z in G) and orientation (yaw, pitch, roll in degrees)
- **FIFO Buffer Management**: Uses MPU6050 internal FIFO to prevent data drops during multitasking
- **Task-Based Architecture**: Cooperative multitasking system with inhibition masks for task coordination
- **SPIFFS Storage**: Buffered, block-aligned writes to flash storage with numbered file rotation
- **High Performance**: Handles datasets up to 50,000+ data points efficiently

### User Interface

- **Captive Portal**: ESP8266 creates its own WiFi network - no external network required
- **Real-time Streaming**: Live sensor data visualization in web browser
- **Historical Data Viewer**: Interactive charts with zoom/pan capabilities for large datasets
- **Mobile Responsive**: Touch-friendly interface works on phones and tablets
- **File Management**: View, delete, and manage recorded data files

### Control System

- **Single Button Control**:
  - Brief press (< 1s): Start/stop recording
  - Long press (3+s): Zero calibration
- **Audio Feedback**: Different piezo buzzer tones for each operation
- **Test Data Generation**: Built-in test data generation for system validation

## Hardware Requirements

### Required Components

- **ESP8266 Development Board** (NodeMCU, Wemos D1 Mini, etc.)
- **MPU6050 IMU Sensor** (6-DOF accelerometer and gyroscope)
- **Push Button** (momentary switch)
- **Piezo Buzzer** (for audio feedback)
- **Breadboard and Jumper Wires**

### Pin Configuration


| ESP8266 Pin | Connection  | Function      |
| ------------- | ------------- | --------------- |
| D1 (GPIO5)  | MPU6050 SCL | I2C Clock     |
| D2 (GPIO4)  | MPU6050 SDA | I2C Data      |
| D3 (GPIO0)  | Button      | Control Input |
| D8 (GPIO15) | Buzzer      | Audio Output  |

### Wiring Diagram

```
ESP8266          MPU6050
-------          --------
D1 (GPIO5)  <---> SCL
D2 (GPIO4)  <---> SDA
3.3V        <---> VCC
GND         <---> GND

ESP8266          Button/Buzzer
-------          --------------
D3 (GPIO0)  <---> Button --> GND
D8 (GPIO15) <---> Buzzer --> GND
3.3V        <---> Button (pull-up)
```

## Software Requirements

### Arduino IDE Setup

1. **Arduino IDE 1.8.19+** or **Arduino IDE 2.0+**
2. **ESP8266 Board Manager**: `esp8266` version 3.0.0 or later
3. **Required Libraries** (install via Library Manager):
   - `ESPAsyncWebServer` by me-no-dev
   - `ESPAsyncTCP` by me-no-dev
   - `Adafruit MPU6050 Library` by Adafruit
   - `Adafruit Unified Sensor` by Adafruit

## Installation and Setup

### 1. Clone or Download

```bash
git clone https://github.com/username/MPULogger.git
cd MPULogger
```

### 2. Upload to ESP8266

1. Open `MPULogger.ino` in Arduino IDE
2. Select your ESP8266 board from Tools > Board
3. Select the correct COM port
4. Click Upload

### 3. First Time Setup

1. Power on the device
2. Connect to WiFi network: `MPULogger`
3. Open browser and navigate to `192.168.0.1`
4. The web interface will load automatically

### 4. Calibration

- Press and hold the button until long tone is played.
- Wait for another tone indicating calibration complete
- The device is now ready for accurate measurements

## Usage Instructions

### Recording Data

1. **Start Recording**: Briefly press the button (single short beep)
2. **Stop Recording**: Briefly press the button again (two short beeps)
3. **View Data**: Connect to `MPULogger` WiFi and open `192.168.0.1`

### Web Interface

#### Main Dashboard (`192.168.0.1`)

- Real-time sensor readings
- System status indicators
- Quick access to visualization tools

#### Real-time Streaming (`/stream.html`)

- Live plots of all 6 sensor values
- Shows last 10 seconds of data
- Automatic refresh every 200ms

#### Historical Viewer (`/viewer.html`)

- Load and visualize recorded data files
- Interactive charts with zoom/pan
- Handle large datasets (50,000+ points)
- Export capabilities

#### File Management

- View all recorded files (`/api/files`)
- Delete unwanted recordings
- Monitor storage usage

### Test Data Generation

The system includes built-in test data generation for validation:

- **Motion Test**: 60 seconds of sine wave motion patterns
- **Static Test**: 30 seconds of constant values
- **Combined Test**: 61 seconds with motion + static validation

## Project Structure

```
MPULogger/
├── MPULogger.ino                 # Main sketch file
├── README.md                     # This file
├── src/                          # Source code
│   ├── constants.h               # GPIO pins and configuration
│   ├── Task.h                    # Base task class
│   ├── Settings.h/.cpp           # Configuration management
│   ├── MPULogRecord.h            # Binary data structure
│   ├── Tasks.h/.cpp              # Task registration
│   ├── MPUSensorTask.h/.cpp      # MPU6050 sensor handling
│   ├── ButtonControlTask.h/.cpp  # Button debouncing and control
│   ├── BuzzerFeedbackTask.h/.cpp # Audio feedback system
│   ├── DataLoggingTask.h/.cpp   # SPIFFS file management
│   ├── WebServerTask.h/.cpp      # Web server and API
│   ├── WebStreamingTask.h/.cpp   # Real-time data streaming
│   ├── TestDataGenerator.h/.cpp  # Test data generation
│   └── ArduinoJSON/              # JSON library (header-only)
└── data/                         # Web interface files
    ├── index.htm                 # Main dashboard
    ├── stream.html               # Real-time streaming
    ├── viewer.html               # Historical data viewer
    ├── uPlot.iife.min.js         # Chart library
    └── uPlot.min.css             # Chart styles
```

## Data Format

### Binary Log Structure

Each data record is 32 bytes:

```cpp
struct MPULogRecord {
    unsigned long timestamp;      // Milliseconds since boot (4 bytes)
    float accel_x;               // X acceleration in G (4 bytes)
    float accel_y;               // Y acceleration in G (4 bytes)
    float accel_z;               // Z acceleration in G (4 bytes)
    float yaw;                   // Yaw angle in degrees (4 bytes)
    float pitch;                 // Pitch angle in degrees (4 bytes)
    float roll;                  // Roll angle in degrees (4 bytes)
    uint8_t flags;               // Status flags (1 byte)
    uint8_t padding;             // Alignment (1 byte)
};
```

### File Naming

- Files are stored as `/mpulog001.bin`, `/mpulog002.bin`, etc.
- Automatic rotation when maximum file count is reached
- Binary format for efficient storage and fast loading

### Performance Characteristics

- **Data Rate**: 320 bytes/second at 10Hz sampling
- **Storage Capacity**: ~1.2MB per hour of recording
- **Maximum Dataset**: 50,000+ points (~1.65MB)
- **Memory Usage**: ~1KB RAM buffer for data logging

## API Endpoints

### REST API

- `GET /api/files` - List all data files
- `GET /api/settings` - Get system configuration
- `POST /api/settings` - Update configuration
- `GET /api/status` - System status (uptime, heap, etc.)
- `POST /api/testdata/generate` - Generate test data

### Real-time Events

- Server-Sent Events at `/events`
- JSON format with sensor data and system status
- 200ms update interval for smooth real-time display

## Configuration

### Settings (`/api/settings`)

```json
{
  "hostName": "MPULogger",
  "sampleRateMs": 100,
  "maxLogFiles": 10,
  "bufferSize": 32,
  "autoCalibration": false,
  "accelRange": 8.0,
  "gyroRange": 500.0
}
```

### Key Parameters

- `sampleRateMs`: Sampling interval in milliseconds (100 = 10Hz)
- `maxLogFiles`: Maximum number of log files to keep
- `bufferSize`: Records in RAM buffer before writing to flash
- `autoCalibration`: Enable automatic calibration on startup

## Troubleshooting

### Common Issues

#### Device Not Connecting

1. Check wiring connections
2. Verify ESP8266 is powered (3.3V)
3. Try pressing reset button
4. Check Serial Monitor for error messages

#### WiFi Not Appearing

1. Wait 30-60 seconds after power-on
2. Check for conflicting networks
3. Restart device (power cycle)
4. Verify D3 button is not stuck

#### Data Recording Issues

1. Ensure SPIFFS is formatted (first-time setup)
2. Check available flash memory
3. Verify MPU6050 connections
4. Calibrate sensor before recording

#### Web Interface Problems

1. Clear browser cache
2. Try different browser
3. Check JavaScript is enabled
4. Verify connection to `MPULogger` network

### Serial Monitor Output

Connect Serial Monitor at 115200 baud for debug information:

- Initialization status
- Sensor calibration results
- File system operations
- Error messages and warnings

## Development

### Architecture Overview

The system uses a cooperative multitasking architecture with the following tasks:


| Task               | Priority     | Function                 |
| -------------------- | -------------- | -------------------------- |
| MPUSensorTask      | Highest      | 10Hz sensor sampling     |
| ButtonControlTask  | High         | Button handling          |
| WebStreamingTask   | Medium       | Real-time data streaming |
| DataLoggingTask    | Medium-low   | File operations          |
| BuzzerFeedbackTask | Event-driven | Audio feedback           |
| WebServerTask      | Lowest       | HTTP server              |

### Adding New Features

1. Create new task class inheriting from `Task`
2. Add task to `Tasks.cpp` registration
3. Update constants.h if new pins needed
4. Add web interface components if required

### Small MCU Optimization

- Uses binary format for efficient storage
- Circular buffer for RAM management
- Block-aligned SPIFFS writes
- Client-side binary decoding for web interface

## Acknowledgments

- **MossESP Framework** - Task-based architecture inspiration
- **Adafruit** - MPU6050 library and sensor drivers
- **uPlot.js** - High-performance charting library
- **ESP8266 Community** - AsyncWebServer and networking support

## Version History

- **v1.0.0** - Initial release with core functionality
  - Basic sensor logging and web interface
  - Real-time streaming and historical data viewer
  - Test data generation capabilities
