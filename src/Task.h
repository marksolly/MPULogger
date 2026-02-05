//SEE Tasks.cpp FOR TASK INSTANTIATION & TO CUSTOMIZE WHICH TASKS RUN

/*
 * https://en.wikipedia.org/wiki/Cooperative_multitasking
 * 
 * This file contains a parent class "Task" and a series of subclasses that inherit 
 * from Task to perform different types of scheduled activities. Each class has a run() 
 * method which is called repeatedly from the Arduino loop().
 * 
 * Each task class must be instantiated and added to the taskList array, which can 
 * be found at the bottom of this file.
 * 
 * Tasks are able to inhibit each other from running, to prevent undesirable simultaneous 
 * activities, Eg: 
 *  - The sensor task can prevent the web server task running during 
 *    critical sensor operations. 
 *  - The buzzer task can prevent other tasks from running during 
 *    tone generation to avoid conflicts.
 * 
 * Each task can specify how often it wants to execute by setting the runInterval
 * property of the class. This can be changed during runtime as needed. Note that
 * runInterval is not guaranteed and is influenced by other tasks that are blocking, 
 * slow or badly behaved. Tasks with a runInterval of 0 are run as often as possible.
 */

#pragma once

//Required for uint8_t style type names
#include <stdint.h>

//Required for F() macro and FlashStringHelper class
#include <Arduino.h>

//Required for IO Pin aliases
#include "constants.h"

class Task {
  public:
    //Number of millis between calls to run()
    unsigned long runInterval { 10000 }; 

    //Last millis() timestamp that the run() method was called.
    unsigned long lastRun = 0;

    //Bit mask that uniquely identifies the task. Must be a power of 2. Eg: 1 = 0b00000001, 2 = 0b00000010, 4 = 0b00000100, 8 = 0b00001000, 16, 32, 64, 128, 256, 512, 1024, 2048. etc...
    static const uint16_t MASK { 0 };

    //Set true if the task runner (main() loop) decides the task is inhibited
    bool isInhibited = false;
    
    static const int NAME_MAX = 20;
    char name[NAME_MAX] = "";
  
    virtual void run() {};

    //Reset any timers that need resetting when millis() overflows
    virtual void millisOverflow() {};

    //Need a getter for mask constant to enable children of this class to be used in polymorphic object pointer arrays
    virtual uint16_t getMask();
    /*{
      return Task::MASK;
    }*/

    //Called instead of run() if this class is inhibited from running. Class should stop any outputs that it controls.
    virtual void inhibited() {};

    //Called after run(). Gives the class an opportunity to apply masks for any other tasks it wants to inhibit.
    //A class stops being inhibited when zero classes return its mask.
    virtual void applyInhibitMask(uint16_t &currentMask){
      //Inhibit nothing by default.
      //Use binary "or" operator "|" to combine masks for classes that should be inhibited.
      //eg: currentMask = currentMask | SensorTask::MASK | LoggingTask::MASK;
    }

    bool isInhibitedByMask(uint16_t &testMask){
      return getMask() & testMask;
    }

    //Helper function that lets the class name be set using the flash string macro F()
    void setName(const __FlashStringHelper *ifsh){
      PGM_P p = reinterpret_cast<PGM_P>(ifsh);
      strncpy_P(name, p, Task::NAME_MAX);
    }    
};
