#ifndef BUZZER_FEEDBACK_TASK_H
#define BUZZER_FEEDBACK_TASK_H

#include "Task.h"
#include "constants.h"

// Buzzer tone patterns
enum BuzzerTone {
  TONE_NONE,
  BUZZ_TONE_CALIBRATION_START,
  BUZZ_TONE_CALIBRATION_COMPLETE,
  BUZZ_TONE_RECORDING_START,
  BUZZ_TONE_RECORDING_STOP
};

class BuzzerFeedbackTask : public Task {
  public:
    static const uint16_t MASK { BUZZER_FEEDBACK_TASK_MASK };
    
    // Constructor
    BuzzerFeedbackTask();
    
    virtual uint16_t getMask() override {
      return BuzzerFeedbackTask::MASK;
    }
    
    virtual void run() override;
    
    // Tone playback methods
    void playCalibrationStartTone();
    void playCalibrationCompleteTone();
    void playRecordingTone(bool isStarting);
    
    // Status methods
    bool isPlaying() const;
    
    virtual void inhibited() override;
    
  private:
    // Tone state management
    BuzzerTone currentTone = TONE_NONE;
    unsigned long toneStartTime = 0;
    unsigned long toneDuration = 0;
    uint16_t toneFrequency = 0;
    bool toneActive = false;
    
    // Internal methods
    void startTone(uint16_t frequency, unsigned long duration);
    void stopTone();
    void updateTone();
    
    // Tone pattern definitions
    void playCalibrationStartPattern();
    void playCalibrationCompletePattern();
    void playRecordingStartPattern();
    void playRecordingStopPattern();
};

#endif
