#ifndef MPU_SENSOR_TASK_H
#define MPU_SENSOR_TASK_H

#include "Task.h"
#include "constants.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// Forward declarations
class DataLoggingTask;
class BuzzerFeedbackTask;

class MPUSensorTask : public Task {
  public:
    static const uint16_t MASK { MPU_SENSOR_TASK_MASK };
    
    // Constructor
    MPUSensorTask(DataLoggingTask* dataLogger = nullptr);
    
    // Current sensor data
    float accel_x = 0;
    float accel_y = 0;
    float accel_z = 0;
    float yaw = 0;
    float pitch = 0;
    float roll = 0;
    
    // Calibration offsets
    float accel_offset_x = 0;
    float accel_offset_y = 0;
    float accel_offset_z = 0;
    float gyro_offset_x = 0;
    float gyro_offset_y = 0;
    float gyro_offset_z = 0;
    
    // Calibration state
    bool isCalibrated = false;
    bool isCalibrating = false;
    
    // FIFO buffer data
    uint16_t fifoCount = 0;
    
    virtual uint16_t getMask() override {
      return MPUSensorTask::MASK;
    }
    
    virtual void run() override;
    
    // Calibration methods
    void startCalibration();
    bool isCalibrationComplete() const;
    void accumulateCalibrationSample();
    
    // Sensor data methods
    void updateSensorData();
    void resetSensorData();
    
    // FIFO methods
    bool initFIFO();
    void readFIFO();
    void flushFIFO();
    
    // Data logging integration
    void setDataLoggingTask(DataLoggingTask* dataLogger);
    
    // Buzzer feedback integration
    void setBuzzerFeedbackTask(BuzzerFeedbackTask* buzzerTask);
    
  private:
    Adafruit_MPU6050 mpu;
    DataLoggingTask* dataLogger;
    BuzzerFeedbackTask* buzzerTask;
    
    // Calibration accumulation variables
    float accel_sum_x = 0;
    float accel_sum_y = 0;
    float accel_sum_z = 0;
    float gyro_sum_x = 0;
    float gyro_sum_y = 0;
    float gyro_sum_z = 0;
    
    // Internal methods
    void calculateOffsets();
    void applyOffsets();
};

#endif
