#ifndef MPULOGRECORD_H
#define MPULOGRECORD_H

#include <Arduino.h>
#include <FS.h>

class MPULogRecord {
  public:
    static const uint8_t FLAG_RECORDING = 1;
    static const uint8_t FLAG_CALIBRATED = 2;
  
    unsigned long timestamp = 0;     // Millis since boot (4 bytes)
    float accel_x = 0;            // X acceleration in G (4 bytes)
    float accel_y = 0;            // Y acceleration in G (4 bytes)
    float accel_z = 0;            // Z acceleration in G (4 bytes)
    float yaw = 0;                // Yaw angle in degrees (4 bytes)
    float pitch = 0;              // Pitch angle in degrees (4 bytes)
    float roll = 0;               // Roll angle in degrees (4 bytes)
    uint8_t flags = 0;           // Recording flags (1 byte)
    uint8_t padding = 0;          // Padding to ensure 32-byte alignment (1 byte)
    
    // Total: 32 bytes per record
    
    bool readFromFile(File &file){
      return file.readBytes(reinterpret_cast<char *>(this), sizeof(*this)) == sizeof(*this);
    }

    bool writeToFile(File &file){
      return file.write(reinterpret_cast<char *>(this), sizeof(*this)) == sizeof(*this);
    }
    
    // Utility functions
    void clear() {
      timestamp = 0;
      accel_x = 0;
      accel_y = 0;
      accel_z = 0;
      yaw = 0;
      pitch = 0;
      roll = 0;
      flags = 0;
      padding = 0;
    }
    
    bool isEmpty() const {
      return timestamp == 0;
    }
    
    void setRecording(bool recording) {
      if (recording) {
        flags |= FLAG_RECORDING;
      } else {
        flags &= ~FLAG_RECORDING;
      }
    }
    
    bool isRecording() const {
      return flags & FLAG_RECORDING;
    }
    
    void setCalibrated(bool calibrated) {
      if (calibrated) {
        flags |= FLAG_CALIBRATED;
      } else {
        flags &= ~FLAG_CALIBRATED;
      }
    }
    
    bool isCalibrated() const {
      return flags & FLAG_CALIBRATED;
    }
    
    // Static method to get record size
    static size_t getRecordSize() {
      return sizeof(MPULogRecord);
    }
};

#endif
