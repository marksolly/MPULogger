#include "BuzzerFeedbackTask.h"
#include "Arduino.h"

BuzzerFeedbackTask::BuzzerFeedbackTask() {
  setName(F("BuzzerFeedbackTask"));
  runInterval = 10;  // Check tone state every 10ms for precise timing
  
  // Initialize buzzer pin
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}

void BuzzerFeedbackTask::run() {
  updateTone();
}

void BuzzerFeedbackTask::inhibited() {
  // Stop tone when inhibited to prevent conflicts
  if (toneActive) {
    stopTone();
  }
}

void BuzzerFeedbackTask::playCalibrationStartTone() {
  playCalibrationStartPattern();
}

void BuzzerFeedbackTask::playCalibrationCompleteTone() {
  playCalibrationCompletePattern();
}

void BuzzerFeedbackTask::playRecordingTone(bool isStarting) {
  if (isStarting) {
    playRecordingStartPattern();
  } else {
    playRecordingStopPattern();
  }
}

bool BuzzerFeedbackTask::isPlaying() const {
  return toneActive;
}

void BuzzerFeedbackTask::startTone(uint16_t frequency, unsigned long duration) {
  toneFrequency = frequency;
  toneDuration = duration;
  toneStartTime = millis();
  toneActive = true;
  
  // Start the tone using ESP8266 tone function
  tone(BUZZER_PIN, frequency);
}

void BuzzerFeedbackTask::stopTone() {
  if (toneActive) {
    noTone(BUZZER_PIN);
    toneActive = false;
    currentTone = TONE_NONE;
    toneFrequency = 0;
    toneDuration = 0;
  }
}

void BuzzerFeedbackTask::updateTone() {
  if (toneActive) {
    unsigned long currentTime = millis();
    
    // Check if tone duration has elapsed
    if (currentTime - toneStartTime >= toneDuration) {
      stopTone();
    }
  }
}

void BuzzerFeedbackTask::playCalibrationStartPattern() {
  // Long tone for calibration start
  startTone(TONE_CALIBRATION_START, TONE_DURATION_LONG);
  currentTone = BUZZ_TONE_CALIBRATION_START;
}

void BuzzerFeedbackTask::playCalibrationCompletePattern() {
  // Single short tone for calibration complete
  if (!toneActive) {
    startTone(TONE_CALIBRATION_COMPLETE, TONE_DURATION_SHORT);
    currentTone = BUZZ_TONE_CALIBRATION_COMPLETE;
  }
}

void BuzzerFeedbackTask::playRecordingStartPattern() {
  // Single short beep for recording start
  startTone(TONE_RECORDING_START, TONE_DURATION_SHORT);
  currentTone = BUZZ_TONE_RECORDING_START;
}

void BuzzerFeedbackTask::playRecordingStopPattern() {
  // Two short beeps for recording stop
  if (!toneActive) {
    startTone(TONE_RECORDING_STOP, TONE_DURATION_SHORT);
    currentTone = BUZZ_TONE_RECORDING_STOP;
  }
}
