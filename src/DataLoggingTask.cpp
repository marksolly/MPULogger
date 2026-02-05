#include "DataLoggingTask.h"
#include "Settings.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>

DataLoggingTask::DataLoggingTask(Settings& settings) 
  : Task(), // Use default constructor
    settings(&settings) {
  setName(F("DataLoggingTask"));
  runInterval = 500;  // Check buffer every 500ms for periodic flushes
  
  // Initialize recording state to false
  recording = false;
  
  // Initialize buffer state - follow ff_LogTask strategy
  lastFlushTime = 0;
  currentFileNumber = 0;
  
  // Initialize ramBuffer to empty state
  for(uint16_t i = 0; i < RAM_BUFFER_SIZE; i++) {
    ramBuffer[i].timestamp = 0;  // Marks buffer record as empty
  }
}

// Recording state management methods
bool DataLoggingTask::isRecording() const {
  return recording;
}

void DataLoggingTask::startRecording() {
  if (!recording) {
    recording = true;
    currentFileName = "";  // Reset filename to force generation of new file number
    openLogFile();
    Serial.println(F("DATA_LOG: Recording started"));
  }
}

void DataLoggingTask::stopRecording() {
  if (recording) {
    recording = false;
    closeLogFile();
    Serial.println(F("DATA_LOG: Recording stopped"));
  }
}

void DataLoggingTask::toggleRecording() {
  if (recording) {
    stopRecording();
  } else {
    startRecording();
  }
}

void DataLoggingTask::run() {
  // ff_LogTask strategy: Check if we need to flush any remaining data
  // This handles the case where no new data is coming in but we have buffered data
  bool hasData = false;
  for(uint16_t i = 0; i < RAM_BUFFER_SIZE; i++) {
    if(ramBuffer[i].timestamp > 0) {
      hasData = true;
      break;
    }
  }
  
  // Fallback flush after AUTO_FLUSH_DELAY to prevent data loss
  if(hasData && (millis() - lastFlushTime >= AUTO_FLUSH_DELAY)) {
    writeRamBufferToFlash();
  }
}

void DataLoggingTask::logSensorData(float accel_x, float accel_y, float accel_z, 
                                  float yaw, float pitch, float roll) {
  // Only log data if we are recording - this fixes the timing race condition
  if (!recording) {
    return;
  }
  
  MPULogRecord record;
  record.timestamp = millis();
  record.accel_x = accel_x;
  record.accel_y = accel_y;
  record.accel_z = accel_z;
  record.yaw = yaw;
  record.pitch = pitch;
  record.roll = roll;
  record.setRecording(true);  // Use the flag method from MPULogRecord
  
  // Use ff_LogTask's ramBufferPut strategy for page-based buffering
  ramBufferPut(record);
}

void DataLoggingTask::inhibited() {
  // ff_LogTask strategy: Flush ramBuffer when inhibited to prevent data loss
  bool hasData = false;
  for(uint16_t i = 0; i < RAM_BUFFER_SIZE; i++) {
    if(ramBuffer[i].timestamp > 0) {
      hasData = true;
      break;
    }
  }
  
  if(hasData) {
    writeRamBufferToFlash();
  }
  
  // Close file when inhibited to ensure data is saved
  closeLogFile();
}

bool DataLoggingTask::createNewLogFile() {
  // Close any existing file
  closeLogFile();
  
  // Get next file name
  getNextFileName();
  
  // Open new file
  openLogFile();
  
  // Return true if file was opened successfully
  return currentFile && currentFile.isFile();
}

String DataLoggingTask::getCurrentLogFileName() const {
  return currentFileName;
}

bool DataLoggingTask::deleteLogFile(const String& fileName) {
  if (SPIFFS.exists(fileName)) {
    return SPIFFS.remove(fileName);
  }
  return false;
}

String DataLoggingTask::listLogFiles() {
  String fileList = "[";
  Dir dir = SPIFFS.openDir("/");
  bool first = true;
  
  while (dir.next()) {
    String fileName = dir.fileName();
    if (fileName.startsWith(LOG_FILE_PREFIX) && fileName.endsWith(LOG_FILE_SUFFIX)) {
      if (!first) {
        fileList += ",";
      }
      fileList += "{\"name\":\"" + fileName + "\",\"size\":" + String(dir.fileSize()) + "}";
      first = false;
    }
  }
  
  fileList += "]";
  return fileList;
}


void DataLoggingTask::getNextFileName() {
  uint16_t maxFileNum = 0;
  Dir dir = SPIFFS.openDir("/");
  
  while (dir.next()) {
    String fileName = dir.fileName();
    if (fileName.startsWith(LOG_FILE_PREFIX) && fileName.endsWith(LOG_FILE_SUFFIX)) {
      // Extract number more robustly - handle both padded and unpadded
      int prefixLen = strlen(LOG_FILE_PREFIX);
      int suffixLen = strlen(LOG_FILE_SUFFIX);
      int numStart = prefixLen;
      int numEnd = fileName.length() - suffixLen;
      
      if (numEnd > numStart) {
        String numStr = fileName.substring(numStart, numEnd);
        uint16_t fileNum = numStr.toInt();
        if (fileNum > maxFileNum) {
          maxFileNum = fileNum;
        }
      }
    }
  }
  
  currentFileNumber = maxFileNum + 1;
  // Use consistent naming without zero-padding to match existing files
  currentFileName = String(LOG_FILE_PREFIX) + String(currentFileNumber) + String(LOG_FILE_SUFFIX);
}

void DataLoggingTask::openLogFile() {
  if (currentFileName.length() == 0) {
    getNextFileName();
  }
  
  currentFile = SPIFFS.open(currentFileName, "w");
  if (currentFile) {
    Serial.print(F("Opened log file: "));
    Serial.println(currentFileName);
  } else {
    Serial.println(F("Failed to open log file"));
  }
}

void DataLoggingTask::closeLogFile() {
  if (currentFile && currentFile.isFile()) {
    currentFile.close();
    if (currentFileName.length() > 0) {
      Serial.print(F("Closed log file: "));
      Serial.println(currentFileName);
    }
  }
}

String DataLoggingTask::formatFileSize(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + " B";
  } else if (bytes < 1024 * 1024) {
    return String(bytes / 1024.0, 1) + " KB";
  } else {
    return String(bytes / (1024.0 * 1024.0), 1) + " MB";
  }
}

// ff_LogTask strategy: Pushes a MPULogRecord onto the end of the ramBuffer array
void DataLoggingTask::ramBufferPut(MPULogRecord rec) {
  uint16_t recCount = 0;

  // Add record to ram buffer (circular shift like ff_LogTask)
  for(uint16_t i = 0; i < RAM_BUFFER_SIZE - 1; i++) {
    ramBuffer[i] = ramBuffer[i + 1];
    if(ramBuffer[i].timestamp > 0) recCount++;
  }
  ramBuffer[RAM_BUFFER_SIZE - 1] = rec;

  // Count the number of records in the buffer
  for(uint16_t i = 0; i < RAM_BUFFER_SIZE; i++) {
    if(ramBuffer[i].timestamp > 0) recCount++;
  }

  // ff_LogTask strategy: If the size of the records in the buffer + the size of an additional 
  // record would be greater than the page size of the SPIFFS file system (256 bytes), 
  // then it's time to flush the ram buffer onto flash.
  if((recCount * sizeof(rec)) + sizeof(rec) > 256) {
    writeRamBufferToFlash();
  }
}

// ff_LogTask strategy: Write ram buffer to flash using append mode
void DataLoggingTask::writeRamBufferToFlash() {
  // Only write if we are recording and have a valid file
  if (!recording) {
    return;
  }
  
  // Ensure file is open for writing
  if (!currentFile || !currentFile.isFile()) {
    if (!createNewLogFile()) {
      Serial.println(F("Failed to create log file"));
      return;
    }
  }

  // File should already be open - verify it's valid for writing
  if(!currentFile) {
    Serial.print(F("Log file '"));
    Serial.print(currentFileName);
    Serial.println(F("' is not available for writing."));
    return;
  }

  uint16_t recordsWritten = 0;
  for(uint16_t i = 0; i < RAM_BUFFER_SIZE; i++) {
    if(ramBuffer[i].timestamp > 0) {
      if(ramBuffer[i].writeToFile(currentFile)) {
        ramBuffer[i].timestamp = 0; // Marks buffer record as empty
        recordsWritten++;
      } else {
        Serial.print(F("Failed to write record to '"));
        Serial.print(currentFileName);
        Serial.println(F("'."));
      }
    }
  }  
  
  currentFile.flush();
  
  if(recordsWritten > 0) {
    lastFlushTime = millis();
    Serial.print(F("Wrote "));
    Serial.print(recordsWritten);
    Serial.println(F(" records to log (page-aligned)"));
  }
}
