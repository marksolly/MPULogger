#include "TestDataGenerator.h"
#include <FS.h>
#include "math.h"

float TestDataGenerator::sineWave(float time, float frequency, float amplitude, float offset) {
    return offset + amplitude * sin(2.0 * M_PI * frequency * time);
}

bool TestDataGenerator::generateTestDataset(const char* filename, int durationSeconds, int recordCount) {
    File file = openFileForWriting(filename);
    if (!file) {
        return false;
    }
    
    int recordsWritten = 0;
    unsigned long startTime = millis();
    
    for (int i = 0; i < recordCount; i++) {
        MPULogRecord record;
        float timeInSeconds = (float)i / SAMPLE_RATE_HZ;
        
        record.timestamp = startTime + (i * MS_PER_SAMPLE);
        
        // Generate different sine waves for each axis
        record.accel_x = sineWave(timeInSeconds, 0.5, 2.0, 0.0);
        record.accel_y = sineWave(timeInSeconds, 0.3, 1.5, 0.0);
        record.accel_z = sineWave(timeInSeconds, 0.7, 1.0, GRAVITY);
        record.yaw = sineWave(timeInSeconds, 0.2, 30.0, 0.0);
        record.pitch = sineWave(timeInSeconds, 0.4, 20.0, 0.0);
        record.roll = sineWave(timeInSeconds, 0.6, 15.0, 0.0);
        record.flags = 0;
        
        if (!writeTestRecord(file, record)) {
            file.close();
            return false;
        }
        
        recordsWritten++;
        
        // Add small delay to prevent overwhelming the system
        if (i % 100 == 0) {
            delay(1);
        }
    }
    
    return closeAndVerifyFile(file, recordsWritten);
}

bool TestDataGenerator::generateBasicMotionTest(const char* filename, int durationSeconds) {
    int recordCount = durationSeconds * SAMPLE_RATE_HZ;
    return generateTestDataset(filename, durationSeconds, recordCount);
}

bool TestDataGenerator::generateStaticTest(const char* filename, int durationSeconds) {
    File file = openFileForWriting(filename);
    if (!file) {
        return false;
    }
    
    int recordsWritten = 0;
    unsigned long startTime = millis();
    int recordCount = durationSeconds * SAMPLE_RATE_HZ;
    
    for (int i = 0; i < recordCount; i++) {
        MPULogRecord record;
        
        record.timestamp = startTime + (i * MS_PER_SAMPLE);
        
        // Test different static values for codec verification
        if (i < recordCount / 3) {
            // Zero values
            record.accel_x = 0.0;
            record.accel_y = 0.0;
            record.accel_z = 0.0;
            record.yaw = 0.0;
            record.pitch = 0.0;
            record.roll = 0.0;
        } else if (i < 2 * recordCount / 3) {
            // Minimum values
            record.accel_x = -4.0;
            record.accel_y = -4.0;
            record.accel_z = -4.0;
            record.yaw = -180.0;
            record.pitch = -90.0;
            record.roll = -180.0;
        } else {
            // Maximum values
            record.accel_x = 4.0;
            record.accel_y = 4.0;
            record.accel_z = 16.0;  // Including gravity
            record.yaw = 180.0;
            record.pitch = 90.0;
            record.roll = 180.0;
        }
        
        record.flags = 0;
        
        if (!writeTestRecord(file, record)) {
            file.close();
            return false;
        }
        
        recordsWritten++;
        
        // Add small delay
        if (i % 100 == 0) {
            delay(1);
        }
    }
    
    return closeAndVerifyFile(file, recordsWritten);
}

bool TestDataGenerator::generateCombinedTest(const char* filename) {
    File file = openFileForWriting(filename);
    if (!file) {
        return false;
    }
    
    int recordsWritten = 0;
    unsigned long startTime = millis();
    
    // 60 seconds of motion data (600 records at 10Hz)
    int motionRecords = 60 * SAMPLE_RATE_HZ;
    
    for (int i = 0; i < motionRecords; i++) {
        MPULogRecord record;
        float timeInSeconds = (float)i / SAMPLE_RATE_HZ;
        
        record.timestamp = startTime + (i * MS_PER_SAMPLE);
        
        // Motion patterns as specified in the project plan
        record.accel_x = sineWave(timeInSeconds, 0.5, 2.0, 0.0);
        record.accel_y = sineWave(timeInSeconds, 0.3, 1.5, 0.0);
        record.accel_z = sineWave(timeInSeconds, 0.7, 1.0, GRAVITY);
        record.yaw = sineWave(timeInSeconds, 0.2, 30.0, 0.0);
        record.pitch = sineWave(timeInSeconds, 0.4, 20.0, 0.0);
        record.roll = sineWave(timeInSeconds, 0.6, 15.0, 0.0);
        record.flags = 0;
        
        if (!writeTestRecord(file, record)) {
            file.close();
            return false;
        }
        
        recordsWritten++;
        
        // Add small delay
        if (i % 100 == 0) {
            delay(1);
        }
    }
    
    // 1 second of static validation data (10 records)
    for (int i = 0; i < SAMPLE_RATE_HZ; i++) {
        MPULogRecord record;
        
        record.timestamp = startTime + ((motionRecords + i) * MS_PER_SAMPLE);
        
        // Zero-point values for codec verification
        record.accel_x = 0.0;
        record.accel_y = 0.0;
        record.accel_z = GRAVITY;
        record.yaw = 0.0;
        record.pitch = 0.0;
        record.roll = 0.0;
        record.flags = 0;
        
        if (!writeTestRecord(file, record)) {
            file.close();
            return false;
        }
        
        recordsWritten++;
    }
    
    return closeAndVerifyFile(file, recordsWritten);
}

bool TestDataGenerator::writeTestRecord(File &file, MPULogRecord &record) {
    return record.writeToFile(file);
}

File TestDataGenerator::openFileForWriting(const char* filename) {
  String fullPath = "/";
  fullPath += filename;
  File file = SPIFFS.open(fullPath, "w");
  if (!file) {
    Serial.printf("Failed to open file %s for writing\n", fullPath.c_str());
    return File();
  }
  
  Serial.printf("Opened file %s for test data generation\n", fullPath.c_str());
  return file;
}

bool TestDataGenerator::closeAndVerifyFile(File &file, int expectedRecords) {
  size_t fileSize = file.size();
  String fileName = file.name();
  file.close();
  
  size_t expectedSize = expectedRecords * sizeof(MPULogRecord);
  
  Serial.printf("Test data generation complete. Records: %d, File size: %d bytes (expected: %d bytes)\n", 
                  expectedRecords, fileSize, expectedSize);
  Serial.printf("File created: %s\n", fileName.c_str());
  
  if (fileSize != expectedSize) {
    Serial.printf("ERROR: File size mismatch! Expected %d bytes, got %d bytes\n", expectedSize, fileSize);
    return false;
  }
  
  return true;
}
