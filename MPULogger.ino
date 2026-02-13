#include <stdint.h>
#include <cmath>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <FS.h>
#include <EEPROM.h>

// Project-specific includes
#include "src/constants.h"
#include "src/Settings.h"
#include "src/EEPROMManager.h"
#include "src/Task.h"
#include "src/Tasks.h"

// Global objects
Settings settings;
EEPROMManager eepromManager;

// Global CPU utilization variables
float cpuUtilization = 0.0;
unsigned long lastCpuUpdate = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  
  // Initialize EEPROM Manager
  eepromManager.begin();
  
  // Initialize SPIFFS
  Serial.println(F("Init SPIFFS"));
  if (SPIFFS.begin()) {
    settings.readFromFile();
  } else {
    Serial.println(F("SPIFFS mount failed"));
  }
  
  // Initialize I2C
  Wire.begin(MPU_SDA_PIN, MPU_SCL_PIN);
  
  // Set up task dependencies after construction
  setupTaskDependencies();
  
  // Initialize MPU sensor
  if (!mpusensorTask.initFIFO()) {
    Serial.println(F("MPU6050 initialization failed"));
  }
}

void loop() {
  static unsigned long lastSlowLoop = 0;
  uint16_t taskMask = 0;
  static uint16_t lastTaskMask = 0;
  
  // CPU utilization tracking
  static unsigned long totalTaskTime = 0;
  static unsigned long measurementStartTime = 0;

  // Handle millis() overflow
  if(lastSlowLoop > millis()) {
    for(int i = 0; i < TASK_COUNT; i++) {
      taskList[i]->millisOverflow();
    }
  }

  // Reset measurement every 1 second
  if (millis() - lastCpuUpdate > 1000) {
    measurementStartTime = micros();
    totalTaskTime = 0;
    lastCpuUpdate = millis();
  }

  // Check for tasks due to run
  for(int i = 0; i < TASK_COUNT; i++) {
    if(!taskList[i]->isInhibitedByMask(taskMask) && !taskList[i]->isInhibitedByMask(lastTaskMask)) {
      if(millis() - taskList[i]->lastRun > taskList[i]->runInterval) {
        taskList[i]->isInhibited = false;
        taskList[i]->lastRun = millis();
        
        unsigned long taskStart = micros();
        taskList[i]->run();
        totalTaskTime += (micros() - taskStart);
      }
      taskList[i]->applyInhibitMask(taskMask);
    } else {
      taskList[i]->isInhibited = true;
      taskList[i]->inhibited();
    }
  }

  // Calculate CPU utilization
  unsigned long totalWindowTime = micros() - measurementStartTime;
  if (totalWindowTime > 0) {
    cpuUtilization = (totalTaskTime * 100.0) / totalWindowTime;
  }

  lastSlowLoop = millis();
  lastTaskMask = taskMask;
}
