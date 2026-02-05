#ifndef BUTTON_CONTROL_TASK_H
#define BUTTON_CONTROL_TASK_H

#include "Task.h"
#include "DataLoggingTask.h"
#include "MPUSensorTask.h"
#include "BuzzerFeedbackTask.h"
#include "constants.h"

// Button states
enum ButtonState {
  IDLE,
  PRESSED,
  DEBOUNCE,
  RELEASED
};

// Button events
enum ButtonEvent {
  NONE,
  SHORT_PRESS,
  LONG_PRESS
};

class ButtonControlTask : public Task {
  public:
    static const uint16_t MASK { BUTTON_CONTROL_TASK_MASK };
    
    // Constructor
    ButtonControlTask(DataLoggingTask& dataLogger, BuzzerFeedbackTask& buzzerTask, MPUSensorTask& sensorTask);
    
    virtual uint16_t getMask() override {
      return ButtonControlTask::MASK;
    }
    
    virtual void run() override;
    
    // Event checking methods
    bool hasEvent() const;
    ButtonEvent getEvent() const;
    void clearEvent();
    
    virtual void inhibited() override;
    
  private:
    DataLoggingTask* dataLogger;
    BuzzerFeedbackTask* buzzerTask;
    MPUSensorTask* sensorTask;
    
    // Button state management
    ButtonState state = IDLE;
    unsigned long pressStartTime = 0;
    unsigned long lastDebounceTime = 0;
    bool currentButtonState = HIGH;  // INPUT_PULLUP means HIGH = not pressed
    
    // Event storage
    ButtonEvent pendingEvent = NONE;
    
    // Internal methods
    void updateButtonState();
    void handleDebounce();
    void handlePress();
    void handleRelease();
    void generateEvent();
};

#endif
