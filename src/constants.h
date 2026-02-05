#ifndef _PROJECT_CONSTANTS

// GPIO Pin Definitions
#define BUTTON_PIN D3
#define BUZZER_PIN D8
#define MPU_SDA_PIN D2
#define MPU_SCL_PIN D1

// MPU6050 Configuration
#define MPU6050_ADDR 0x68
#define MPU6050_ACCEL_RANGE MPU6050_RANGE_8_G
#define MPU6050_GYRO_RANGE MPU6050_RANGE_500_DEG
#define MPU6050_BANDWIDTH MPU6050_BAND_21_HZ

// Task Mask Values (must be powers of 2)
#define MPU_SENSOR_TASK_MASK 1      // 0b00000001
#define BUTTON_CONTROL_TASK_MASK 2    // 0b00000010
#define BUZZER_FEEDBACK_TASK_MASK 4  // 0b00000100
#define DATA_LOGGING_TASK_MASK 8    // 0b00001000
#define WEB_SERVER_TASK_MASK 16      // 0b00010000
#define WEB_STREAMING_TASK_MASK 32   // 0b00100000

// SPIFFS Configuration
#define SPIFFS_BLOCK_SIZE 256
#define LOG_FILE_PREFIX "/mpulog"
#define LOG_FILE_SUFFIX ".bin"

// Timing Configuration
#define BUTTON_DEBOUNCE_MS 50
#define CALIBRATION_SAMPLES 100
#define CALIBRATION_HOLD_MS 3000   // 3 seconds

// Audio Feedback Frequencies (Hz)
#define TONE_CALIBRATION_START 500
#define TONE_CALIBRATION_COMPLETE 800
#define TONE_RECORDING_START 1000
#define TONE_RECORDING_STOP 600
#define TONE_DURATION_SHORT 100
#define TONE_DURATION_LONG 500

// Memory Configuration
#define WEB_CLIENT_MAX 4             // Maximum web streaming clients

#define _PROJECT_CONSTANTS
#endif
