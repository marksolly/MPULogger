#include "MPUSensorTask.h"
#include "DataLoggingTask.h"
#include "BuzzerFeedbackTask.h"
#include "Settings.h"

// Global settings instance
extern Settings settings;

MPUSensorTask::MPUSensorTask(DataLoggingTask* dataLogger) 
  : dataLogger(dataLogger) {
  setName(F("MPUSensorTask"));
  runInterval = 100;  // 10Hz = 100ms
}

void MPUSensorTask::run() {
  // Check for calibration completion
    if (isCalibrating) {
    if (isCalibrationComplete()) {
      calculateOffsets();
      applyOffsets();
      isCalibrating = false;
      isCalibrated = true;
      calibrationStatus = CALIBRATED;
      
      // Save calibration to EEPROM
      saveCalibration();
      
      Serial.println(F("Calibration complete"));
      
      // Play calibration complete tone
      if (buzzerTask) {
        buzzerTask->playCalibrationCompleteTone();
      }
    } else {
      // Collect calibration sample and accumulate values
      accumulateCalibrationSample();
      fifoCount++;
    }
    return;
  }
  
  // Normal operation - read from FIFO
  readFIFO();
  
  // Update current sensor data for other tasks
  updateSensorData();
}

void MPUSensorTask::startCalibration() {
  isCalibrating = true;
  resetSensorData();
  fifoCount = 0;
  
  // Reset accumulation variables
  accel_sum_x = 0;
  accel_sum_y = 0;
  accel_sum_z = 0;
  gyro_sum_x = 0;
  gyro_sum_y = 0;
  gyro_sum_z = 0;
  
  // Reset FIFO to start fresh calibration
  // Note: Adafruit MPU6050 doesn't have resetFIFO method
  // We'll work with available methods
  
  Serial.println(F("Starting calibration..."));
}

bool MPUSensorTask::isCalibrationComplete() const {
  return fifoCount >= CALIBRATION_SAMPLES;
}

void MPUSensorTask::accumulateCalibrationSample() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  
  // Accumulate raw sensor values for calibration
  accel_sum_x += a.acceleration.x;
  accel_sum_y += a.acceleration.y;
  accel_sum_z += a.acceleration.z;
  gyro_sum_x += g.gyro.x;
  gyro_sum_y += g.gyro.y;
  gyro_sum_z += g.gyro.z;
}

void MPUSensorTask::calculateOffsets() {
  // Calculate average offsets from accumulated samples
  
  // For accelerometer: when the sensor is level, Z should read +9.81 m/s² (gravity)
  // So we subtract the expected gravity from the Z axis average
  accel_offset_x = accel_sum_x / CALIBRATION_SAMPLES;
  accel_offset_y = accel_sum_y / CALIBRATION_SAMPLES;
  accel_offset_z = (accel_sum_z / CALIBRATION_SAMPLES) - 9.81;  // Subtract gravity
  
  // For gyroscope: when stationary, all axes should read 0
  gyro_offset_x = gyro_sum_x / CALIBRATION_SAMPLES;
  gyro_offset_y = gyro_sum_y / CALIBRATION_SAMPLES;
  gyro_offset_z = gyro_sum_z / CALIBRATION_SAMPLES;
  
  Serial.print(F("Calculated offsets - Accel: "));
  Serial.print(accel_offset_x); Serial.print(F(", "));
  Serial.print(accel_offset_y); Serial.print(F(", "));
  Serial.print(accel_offset_z);
  Serial.print(F(" | Gyro: "));
  Serial.print(gyro_offset_x); Serial.print(F(", "));
  Serial.print(gyro_offset_y); Serial.print(F(", "));
  Serial.println(gyro_offset_z);
}

void MPUSensorTask::applyOffsets() {
  // Calibration offsets are now applied in updateSensorData()
  // This method can be used for any hardware-level calibration if needed
  Serial.println(F("Applied calibration offsets"));
}

bool MPUSensorTask::loadSavedCalibration() {
  if (settings.isCalibrationDataAvailable()) {
    settings.getCalibrationData(accel_offset_x, accel_offset_y, accel_offset_z,
                                gyro_offset_x, gyro_offset_y, gyro_offset_z);
    
    isCalibrated = true;
    calibrationStatus = USING_SAVED;
    
    Serial.println(F("Loaded saved calibration from EEPROM"));
    Serial.print(F("Loaded offsets - Accel: "));
    Serial.print(accel_offset_x); Serial.print(F(", "));
    Serial.print(accel_offset_y); Serial.print(F(", "));
    Serial.print(accel_offset_z);
    Serial.print(F(" | Gyro: "));
    Serial.print(gyro_offset_x); Serial.print(F(", "));
    Serial.print(gyro_offset_y); Serial.print(F(", "));
    Serial.println(gyro_offset_z);
    
    return true;
  }
  
  Serial.println(F("No saved calibration found in EEPROM"));
  return false;
}

bool MPUSensorTask::saveCalibration() {
  if (settings.saveCalibrationToEEPROM(accel_offset_x, accel_offset_y, accel_offset_z,
                                       gyro_offset_x, gyro_offset_y, gyro_offset_z)) {
    Serial.println(F("Calibration saved to EEPROM"));
    return true;
  }
  
  Serial.println(F("Failed to save calibration to EEPROM"));
  return false;
}

CalibrationStatus MPUSensorTask::getCalibrationStatus() const {
  return calibrationStatus;
}

void MPUSensorTask::updateSensorData() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  
  // Apply calibration offsets if calibrated
  if (isCalibrated) {
    accel_x = (a.acceleration.x - accel_offset_x) / 9.81;
    accel_y = (a.acceleration.y - accel_offset_y) / 9.81;
    accel_z = (a.acceleration.z - accel_offset_z) / 9.81;
    
    // Convert gyro to degrees and apply offsets
    yaw = (g.gyro.x - gyro_offset_x) * 57.2958;
    pitch = (g.gyro.y - gyro_offset_y) * 57.2958;
    roll = (g.gyro.z - gyro_offset_z) * 57.2958;
  } else {
    accel_x = a.acceleration.x / 9.81;
    accel_y = a.acceleration.y / 9.81;
    accel_z = a.acceleration.z / 9.81;
    
    // Convert gyro to degrees
    yaw = g.gyro.x * 57.2958;
    pitch = g.gyro.y * 57.2958;
    roll = g.gyro.z * 57.2958;
  }
  
  // Always pass sensor data to data logger - DataLoggingTask will decide whether to log
  if (dataLogger) {
    dataLogger->logSensorData(accel_x, accel_y, accel_z, yaw, pitch, roll);
  }
}

void MPUSensorTask::resetSensorData() {
  accel_x = 0;
  accel_y = 0;
  accel_z = 0;
  yaw = 0;
  pitch = 0;
  roll = 0;
}

bool MPUSensorTask::initFIFO() {
  if (!mpu.begin()) {
    Serial.println(F("Failed to find MPU6050 chip"));
    return false;
  }
  
  // Set ranges and bandwidth
  mpu.setAccelerometerRange(MPU6050_ACCEL_RANGE);
  mpu.setGyroRange(MPU6050_GYRO_RANGE);
  mpu.setFilterBandwidth(MPU6050_BANDWIDTH);
  
  // Try to load saved calibration on initialization
  loadSavedCalibration();
  
  // Note: Adafruit MPU6050 library has limited FIFO support
  // We'll use direct sensor reading instead
  
  Serial.println(F("MPU6050 initialized"));
  return true;
}

void MPUSensorTask::readFIFO() {
  // Since Adafruit MPU6050 library has limited FIFO support,
  // we'll just handle any FIFO processing needed
  // This is called from run() at 100ms intervals
  
  // Note: updateSensorData() is called separately in run() to avoid double logging
}

void MPUSensorTask::flushFIFO() {
  // Clear any remaining FIFO data
  // Note: Adafruit MPU6050 doesn't have resetFIFO method
  // No action needed for now
}

void MPUSensorTask::setDataLoggingTask(DataLoggingTask* dataLogger) {
  this->dataLogger = dataLogger;
}

void MPUSensorTask::setBuzzerFeedbackTask(BuzzerFeedbackTask* buzzerTask) {
  this->buzzerTask = buzzerTask;
}
