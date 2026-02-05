#include "ButtonControlTask.h"

ButtonControlTask::ButtonControlTask(DataLoggingTask& dataLogger, BuzzerFeedbackTask& buzzerTask, MPUSensorTask& sensorTask) 
  : dataLogger(&dataLogger), buzzerTask(&buzzerTask), sensorTask(&sensorTask) {
  setName(F("ButtonControlTask"));
  runInterval = BUTTON_DEBOUNCE_MS;
  
  // Initialize button pin
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void ButtonControlTask::run() {
  updateButtonState();
  generateEvent();
  
  if (hasEvent()) {
    ButtonEvent event = getEvent();
    
    switch (event) {
      case SHORT_PRESS:
        Serial.println(F("BUTTON: Short press detected - toggling recording"));
        // Toggle recording state using DataLoggingTask
        dataLogger->toggleRecording();
        buzzerTask->playRecordingTone(!dataLogger->isRecording());
        Serial.print(F("BUTTON: Recording state changed to: "));
        Serial.println(dataLogger->isRecording() ? F("ON") : F("OFF"));
        break;
        
      case LONG_PRESS:
        Serial.println(F("BUTTON: Long press detected - starting calibration"));
        // Start calibration using MPUSensorTask
        sensorTask->startCalibration();
        buzzerTask->playCalibrationStartTone();
        break;
        
      default:
        break;
    }
    
    clearEvent();
  }
}

void ButtonControlTask::inhibited() {
  // Inhibit button processing when inhibited
  // No action needed
}

bool ButtonControlTask::hasEvent() const {
  return pendingEvent != NONE;
}

ButtonEvent ButtonControlTask::getEvent() const {
  return pendingEvent;
}

void ButtonControlTask::clearEvent() {
  pendingEvent = NONE;
}

void ButtonControlTask::updateButtonState() {
  bool buttonState = digitalRead(BUTTON_PIN);
  unsigned long currentTime = millis();
  
  switch (state) {
    case IDLE:
      if (!buttonState) {
        state = PRESSED;
        pressStartTime = currentTime;
        lastDebounceTime = currentTime;
        Serial.println(F("BUTTON: State changed to PRESSED"));
      }
      break;
      
    case PRESSED:
      if (buttonState) {
        if (currentTime - lastDebounceTime >= BUTTON_DEBOUNCE_MS) {
          state = DEBOUNCE;
          lastDebounceTime = currentTime;
        }
      } else {
        if (currentTime - pressStartTime >= CALIBRATION_HOLD_MS) {
          pendingEvent = LONG_PRESS;
        }
      }
      break;
      
    case DEBOUNCE:
      if (buttonState) {
        state = RELEASED;
        // Check for short or long press after release
        if (currentTime - pressStartTime >= CALIBRATION_HOLD_MS) {
          pendingEvent = LONG_PRESS;
        } else if (currentTime - pressStartTime >= BUTTON_DEBOUNCE_MS && 
                   currentTime - pressStartTime < CALIBRATION_HOLD_MS) {
          pendingEvent = SHORT_PRESS;
        }
      }
      break;
      
    case RELEASED:
      if (!buttonState) {
        state = IDLE;
        pressStartTime = 0;
        Serial.println(F("BUTTON: State returned to IDLE"));
      }
      break;
  }
  
  currentButtonState = buttonState;
}

void ButtonControlTask::generateEvent() {
  // Event is already generated in updateButtonState()
  // No additional action needed
}

void ButtonControlTask::handleDebounce() {
  // Debounce logic handled in state machine
}

void ButtonControlTask::handlePress() {
  // Press handling handled in state machine
}

void ButtonControlTask::handleRelease() {
  // Release handling handled in state machine
}
