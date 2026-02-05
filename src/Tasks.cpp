#include "Tasks.h"
#include "Settings.h"
#include "MPUSensorTask.h"
#include "ButtonControlTask.h"
#include "BuzzerFeedbackTask.h"
#include "TestDataGenerator.h"

// Task instances with dependency injection
extern Settings settings;

MPUSensorTask mpusensorTask;
DataLoggingTask dataLoggingTask(settings);
ButtonControlTask buttonControlTask(dataLoggingTask, buzzerFeedbackTask, mpusensorTask);
BuzzerFeedbackTask buzzerFeedbackTask;
WebServerTask webServerTask(settings);
WebStreamingTask webStreamingTask(mpusensorTask, dataLoggingTask);

// Set up circular dependency after construction
void setupTaskDependencies() {
  mpusensorTask.setDataLoggingTask(&dataLoggingTask);
  
  // Set up web streaming with web server
  webStreamingTask.setupEventSource(&webServerTask.server);
}

// Static array with automatic compile-time count
Task* taskList[] = {
    &mpusensorTask,
    &buttonControlTask,
    &buzzerFeedbackTask,
    &dataLoggingTask,
    &webServerTask,
    &webStreamingTask
};

// Compile-time task count using sizeof()
const int TASK_COUNT = sizeof(taskList) / sizeof(Task*);
