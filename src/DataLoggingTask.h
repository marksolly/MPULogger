#ifndef DATA_LOGGING_TASK_H
#define DATA_LOGGING_TASK_H

#include "Task.h"
#include "MPULogRecord.h"
#include "constants.h"
#include <FS.h>

// Forward declaration
class Settings;

class DataLoggingTask : public Task {
  public:
    static const uint16_t MASK { DATA_LOGGING_TASK_MASK };
    
    // Constructor
    DataLoggingTask(Settings& settings);
    
    virtual uint16_t getMask() override {
      return DataLoggingTask::MASK;
    }
    
    virtual void run() override;
    
    // File management methods
    bool createNewLogFile();
    String getCurrentLogFileName() const;
    bool deleteLogFile(const String& fileName);
    String listLogFiles();
    
    // Recording state management methods
    bool isRecording() const;
    void startRecording();
    void stopRecording();
    void toggleRecording();
    
    // Event-driven data logging
    void logSensorData(float accel_x, float accel_y, float accel_z, 
                      float yaw, float pitch, float roll);
    
    // File management for sensor task
    void openLogFile();
    void closeLogFile();
    
    virtual void inhibited() override;
    
  private:
    Settings* settings;
    
    // Recording state
    bool recording = false;
    
    // Constants for page-based buffering (ff_LogTask strategy)
    static const uint8_t MAX_LOG_FILES = 10;
    static const unsigned int RAM_BUFFER_SIZE = (256 / sizeof(MPULogRecord)) + 2;  // One page + margin
    static const unsigned long AUTO_FLUSH_DELAY = 5000UL;  // Fallback flush delay to prevent data loss
    
    // File management
    String currentFileName;
    File currentFile;
    uint16_t currentFileNumber;
    
    // RAM Buffer management (ff_LogTask approach)
    MPULogRecord ramBuffer[RAM_BUFFER_SIZE];
    unsigned long lastFlushTime;
    
    // Internal methods
    void ramBufferPut(MPULogRecord rec);  // Add record to buffer with page-size checking
    void writeRamBufferToFlash();          // Write full page to SPIFFS
    void getNextFileName();
    String formatFileSize(size_t bytes);
};

#endif
