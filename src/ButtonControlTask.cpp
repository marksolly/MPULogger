#include "ButtonControlTask.h"

ButtonControlTask::ButtonControlTask(DataLoggingTask& dataLogger, BuzzerFeedbackTask& buzzerTask, MPUSensorTask& sensorTask) 
  : dataLogger(&dataLogger), buzzerTask(&buzzerTask), sensorTask(&sensorTask) {
  setName(F("ButtonControlTask"));
  runInterval = (BUTTON_DEBOUNCE_MS / 4) + 1; //Run interval must always be less than debounce interval
  
  // Initialize button pin
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void ButtonControlTask::run() {
  updateButtonState();
  
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
  return pendingEvent != NONE && (state == RELEASED_INHIBIT || state == IDLE);
}

ButtonEvent ButtonControlTask::getEvent() const {
  if(state == RELEASED_INHIBIT || state == IDLE) return pendingEvent;
  return NONE;
}

void ButtonControlTask::clearEvent() {
  pendingEvent = NONE;
}

void ButtonControlTask::updateButtonState() {
  static bool prevButtonState = HIGH;
  static ButtonState prevMachineState = IDLE;
  bool machineStateChanged = false;
  bool buttonState = digitalRead(BUTTON_PIN);
  unsigned long currentTime = millis();
  
  if(buttonState != prevButtonState){
    Serial.print("BUTTON_PIN Changed Value: ");
    if(buttonState == HIGH) Serial.println("HIGH");
    if(buttonState == LOW) Serial.println("LOW");
    prevButtonState = buttonState;
  }
  
  if(state != prevMachineState){
    machineStateChanged = true;
    prevMachineState = state;
  }

  switch (state) {
    case IDLE:
      if (!buttonState) {  // Button pressed (LOW due to INPUT_PULLUP)
        state = DEBOUNCE_PRESSED;
        lastDebounceTime = currentTime;
        Serial.println(F("BUTTON: State changed to DEBOUNCE_PRESSED"));
      }
      break;
      
    case DEBOUNCE_PRESSED:
      if (!buttonState) {  // Still pressed
        if (currentTime - lastDebounceTime >= BUTTON_DEBOUNCE_MS) {
          state = PRESSED_HOLD;
          pressStartTime = currentTime;
          Serial.println(F("BUTTON: State changed to PRESSED_HOLD"));
        }
      } else {  // Released during debounce - treat as noise
        state = IDLE;
        Serial.println(F("BUTTON: Noise detected, returning to IDLE"));
      }
      break;
      
    case PRESSED_HOLD:
      if (buttonState) {  // Button released
        state = DEBOUNCE_RELEASED;
        lastDebounceTime = currentTime;
        releaseTime = currentTime;
        Serial.println(F("BUTTON: State changed to DEBOUNCE_RELEASED"));
      } else {
        // Check for long press while holding
        if (currentTime - pressStartTime >= CALIBRATION_HOLD_MS) {
          pendingEvent = LONG_PRESS;
          state = RELEASED_INHIBIT;  // Move to release debounce after generating event
          lastDebounceTime = currentTime;
          releaseTime = currentTime;
          Serial.println(F("BUTTON: Long press detected, moving to DEBOUNCE_RELEASED"));
        }
      }
      break;
      
    case DEBOUNCE_RELEASED:
      if (buttonState) {  // Still released
        if (currentTime - lastDebounceTime >= BUTTON_DEBOUNCE_MS) {
          // Release confirmed - check for short press if no long press was generated
          if (pendingEvent == NONE && currentTime - pressStartTime >= BUTTON_DEBOUNCE_MS && 
              currentTime - pressStartTime < CALIBRATION_HOLD_MS) {
            pendingEvent = SHORT_PRESS;
          }
          state = RELEASED_INHIBIT;
          Serial.println(F("BUTTON: State changed to RELEASED_INHIBIT"));
        }
      } else {  // Re-pressed during release debounce - treat as noise
        state = PRESSED_HOLD;
        if(machineStateChanged) Serial.println(F("BUTTON: Noise detected during release debounce, returning to PRESSED_HOLD"));
      }
      break;
      
    case RELEASED_INHIBIT:
      if (buttonState) {  // Still released
        if (currentTime - releaseTime >= BUTTON_RELEASE_INHIBIT_MS) {
          state = IDLE;
          pressStartTime = 0;
          Serial.println(F("BUTTON: Inhibition period complete, returning to IDLE"));
        }
      } else {  // Button pressed during inhibition - ignore and stay in inhibition
        // Reset release time to extend inhibition
        releaseTime = currentTime;
        if(machineStateChanged) Serial.println(F("BUTTON: Press during inhibition ignored, extending inhibition period"));
      }
      break;
  }
}
