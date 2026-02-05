#ifndef TEST_DATA_GENERATOR_H
#define TEST_DATA_GENERATOR_H

#include <Arduino.h>
#include "MPULogRecord.h"

class TestDataGenerator {
public:
    // Generate sine wave with specified parameters
    static float sineWave(float time, float frequency, float amplitude, float offset);
    
    // Generate test dataset with specified duration and record count
    static bool generateTestDataset(const char* filename, int durationSeconds, int recordCount);
    
    // Basic motion test: different sine waves for each sensor axis
    static bool generateBasicMotionTest(const char* filename, int durationSeconds = 60);
    
    // Static test: constant values at zero, minimum and maximum
    static bool generateStaticTest(const char* filename, int durationSeconds = 30);
    
    // Combined test: 60 seconds motion + 1 second static validation
    static bool generateCombinedTest(const char* filename);
    
private:
    // Helper to write MPULogRecord to file
    static bool writeTestRecord(File &file, MPULogRecord &record);
    
    // Helper to open file for writing
    static File openFileForWriting(const char* filename);
    
    // Helper to close file and verify integrity
    static bool closeAndVerifyFile(File &file, int expectedRecords);
    
    // Constants for test data generation
    static constexpr float GRAVITY = 9.81f;  // m/s²
    static constexpr int SAMPLE_RATE_HZ = 10;  // 10Hz sampling
    static constexpr int MS_PER_SAMPLE = 1000 / SAMPLE_RATE_HZ;  // 100ms
};

#endif
