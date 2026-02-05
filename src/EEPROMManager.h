#ifndef EEPROM_MANAGER_H
#define EEPROM_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>
#include "constants.h"

// Packed struct to ensure consistent memory layout
struct __attribute__((packed)) CalibrationData {
    uint32_t magicNumber;      // 0x43414C45 ("CALE")
    uint8_t version;           // Data structure version
    uint8_t flags;             // Validity flags
    float accelOffsetX;        // Accelerometer X offset
    float accelOffsetY;        // Accelerometer Y offset  
    float accelOffsetZ;        // Accelerometer Z offset
    float gyroOffsetX;         // Gyroscope X offset
    float gyroOffsetY;         // Gyroscope Y offset
    float gyroOffsetZ;         // Gyroscope Z offset
    uint32_t checksum;         // Data integrity checksum
};

class EEPROMManager {
  public:
    EEPROMManager();
    
    // Initialize EEPROM system
    bool begin();
    
    // Calibration data operations
    bool loadCalibrationData(CalibrationData& data);
    bool saveCalibrationData(const CalibrationData& data);
    
    // Validation operations
    bool isCalibrationDataValid(const CalibrationData& data);
    void clearCalibrationData();
    
    // Utility methods
    size_t getCalibrationDataSize() const;
    
  private:
    bool initialized;
    
    // Checksum calculation
    uint32_t calculateChecksum(const CalibrationData& data) const;
    
    // Data validation helpers
    bool isValidMagicNumber(uint32_t magic) const;
    bool isValidVersion(uint8_t version) const;
    
    // EEPROM operations
    bool readCalibrationStruct(CalibrationData& data);
    bool writeCalibrationStruct(const CalibrationData& data);
};

#endif
