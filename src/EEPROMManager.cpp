#include "EEPROMManager.h"

EEPROMManager::EEPROMManager() : initialized(false) {
}

bool EEPROMManager::begin() {
    if (!initialized) {
        EEPROM.begin(EEPROM_SIZE);
        initialized = true;
    }
    return true;
}

bool EEPROMManager::loadCalibrationData(CalibrationData& data) {
    if (!initialized) {
        return false;
    }
    
    // Read the struct from EEPROM
    if (!readCalibrationStruct(data)) {
        return false;
    }
    
    // Validate the loaded data
    if (!isCalibrationDataValid(data)) {
        return false;
    }
    
    return true;
}

bool EEPROMManager::saveCalibrationData(const CalibrationData& data) {
    if (!initialized) {
        return false;
    }
    
    // Create a copy to calculate checksum
    CalibrationData dataWithChecksum = data;
    dataWithChecksum.magicNumber = EEPROM_CAL_MAGIC_NUMBER;
    dataWithChecksum.version = EEPROM_CAL_VERSION;
    dataWithChecksum.flags |= EEPROM_CAL_FLAG_VALID;
    dataWithChecksum.checksum = calculateChecksum(dataWithChecksum);
    
    // Write to EEPROM
    return writeCalibrationStruct(dataWithChecksum);
}

bool EEPROMManager::isCalibrationDataValid(const CalibrationData& data) {
    // Check magic number
    if (!isValidMagicNumber(data.magicNumber)) {
        return false;
    }
    
    // Check version
    if (!isValidVersion(data.version)) {
        return false;
    }
    
    // Check valid flag
    if (!(data.flags & EEPROM_CAL_FLAG_VALID)) {
        return false;
    }
    
    // Verify checksum
    CalibrationData tempData = data;
    uint32_t expectedChecksum = calculateChecksum(tempData);
    if (data.checksum != expectedChecksum) {
        return false;
    }
    
    return true;
}

void EEPROMManager::clearCalibrationData() {
    if (!initialized) {
        return;
    }
    
    CalibrationData emptyData = {0};
    writeCalibrationStruct(emptyData);
}

size_t EEPROMManager::getCalibrationDataSize() const {
    return sizeof(CalibrationData);
}

uint32_t EEPROMManager::calculateChecksum(const CalibrationData& data) const {
    uint32_t checksum = 0;
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&data);
    
    // Calculate checksum over all fields except the checksum itself
    size_t checksumOffset = offsetof(CalibrationData, checksum);
    
    for (size_t i = 0; i < checksumOffset; i++) {
        checksum += ptr[i];
        checksum = (checksum << 1) | (checksum >> 31); // Rotate left
    }
    
    return checksum;
}

bool EEPROMManager::isValidMagicNumber(uint32_t magic) const {
    return magic == EEPROM_CAL_MAGIC_NUMBER;
}

bool EEPROMManager::isValidVersion(uint8_t version) const {
    return version == EEPROM_CAL_VERSION;
}

bool EEPROMManager::readCalibrationStruct(CalibrationData& data) {
    EEPROM.get(EEPROM_CAL_BASE_ADDR, data);
    return true;
}

bool EEPROMManager::writeCalibrationStruct(const CalibrationData& data) {
    EEPROM.put(EEPROM_CAL_BASE_ADDR, data);
    if (EEPROM.commit()) {
        return true;
    }
    return false;
}
