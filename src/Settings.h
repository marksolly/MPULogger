#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include <FS.h>
#include "ArduinoJSON/ArduinoJson-v6.18.3.h"
#include "EEPROMManager.h"

class Settings {
  public:
    char hostName[20] = "MPULogger";
    uint16_t sampleRateMs = 100;        // 10Hz = 100ms
    uint8_t maxLogFiles = 10;           // Maximum log files to keep
    uint32_t bufferSize = 32;           // Records in RAM buffer
    bool autoCalibration = false;        // Auto-calibrate on startup
    float accelRange = 8.0;             // MPU6050 accelerometer range
    float gyroRange = 500.0;            // MPU6050 gyroscope range
    
    // Constructor
    Settings();
    
    // File operations
    bool readFromFile();
    bool writeToFile();
    
    // Configuration helpers
    void setDefaults();
    
    // Calibration storage operations
    bool loadCalibrationFromEEPROM();
    bool saveCalibrationToEEPROM(float accelX, float accelY, float accelZ, 
                                 float gyroX, float gyroY, float gyroZ);
    bool isCalibrationDataAvailable();
    void getCalibrationData(float& accelX, float& accelY, float& accelZ,
                           float& gyroX, float& gyroY, float& gyroZ);
    
  private:
    static const unsigned int JSON_MEMORY_ALLOC = 1024;
    const char* configFileName = "/settings.json";
    
    bool applyFromJSON(const String& jsonStr);
    String toJSON();
    
    // EEPROM manager instance
    EEPROMManager eepromManager;
};

#endif
