#ifndef TASKS_H
#define TASKS_H

#include "Task.h"
#include "Settings.h"
#include "BuzzerFeedbackTask.h"
#include "MPUSensorTask.h"
#include "ButtonControlTask.h"
#include "DataLoggingTask.h"
#include "WebServerTask.h"
#include "WebStreamingTask.h"

// Global task instances - accessible from anywhere
extern MPUSensorTask mpusensorTask;
extern ButtonControlTask buttonControlTask;
extern BuzzerFeedbackTask buzzerFeedbackTask;
extern DataLoggingTask dataLoggingTask;
extern WebServerTask webServerTask;
extern WebStreamingTask webStreamingTask;

// Global task array and count - accessible from main loop()
extern Task* taskList[];
extern const int TASK_COUNT;

// Task dependency setup function
void setupTaskDependencies();

#endif
