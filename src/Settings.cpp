#include "Settings.h"
#include "ArduinoJSON/ArduinoJson-v6.18.3.h"

Settings::Settings() {
  setDefaults();
}

bool Settings::readFromFile() {
  File file = SPIFFS.open(configFileName, "r");
  if (!file) {
    Serial.println(F("Settings file not found, using defaults"));
    setDefaults();
    return false;
  }
  
  String jsonStr = file.readString();
  file.close();
  
  return applyFromJSON(jsonStr);
}

bool Settings::writeToFile() {
  File file = SPIFFS.open(configFileName, "w");
  if (!file) {
    Serial.println(F("Failed to open settings file for writing"));
    return false;
  }
  
  String jsonStr = toJSON();
  size_t bytesWritten = file.print(jsonStr);
  file.close();
  
  return bytesWritten == jsonStr.length();
}

void Settings::setDefaults() {
  strcpy(hostName, "MPULogger");
  sampleRateMs = 100;
  maxLogFiles = 10;
  bufferSize = 32;
  autoCalibration = false;
  accelRange = 8.0;
  gyroRange = 500.0;
  
  Serial.println(F("Settings set to defaults"));
}

bool Settings::applyFromJSON(const String& jsonStr) {
  StaticJsonDocument<JSON_MEMORY_ALLOC> doc;
  
  DeserializationError error = deserializeJson(doc, jsonStr);
  if (error) {
    Serial.print(F("JSON parse error: "));
    Serial.println(error.c_str());
    return false;
  }
  
  // Apply settings from JSON
  if (doc.containsKey("hostName")) {
    strlcpy(hostName, doc["hostName"], sizeof(hostName));
  }
  if (doc.containsKey("sampleRateMs")) {
    sampleRateMs = doc["sampleRateMs"];
  }
  if (doc.containsKey("maxLogFiles")) {
    maxLogFiles = doc["maxLogFiles"];
  }
  if (doc.containsKey("bufferSize")) {
    bufferSize = doc["bufferSize"];
  }
  if (doc.containsKey("autoCalibration")) {
    autoCalibration = doc["autoCalibration"];
  }
  if (doc.containsKey("accelRange")) {
    accelRange = doc["accelRange"];
  }
  if (doc.containsKey("gyroRange")) {
    gyroRange = doc["gyroRange"];
  }
  
  Serial.println(F("Settings loaded from JSON"));
  return true;
}

String Settings::toJSON() {
  StaticJsonDocument<JSON_MEMORY_ALLOC> doc;
  
  doc["hostName"] = hostName;
  doc["sampleRateMs"] = sampleRateMs;
  doc["maxLogFiles"] = maxLogFiles;
  doc["bufferSize"] = bufferSize;
  doc["autoCalibration"] = autoCalibration;
  doc["accelRange"] = accelRange;
  doc["gyroRange"] = gyroRange;
  
  String output;
  serializeJson(doc, output);
  return output;
}
