#ifndef WEB_STREAMING_TASK_H
#define WEB_STREAMING_TASK_H

#include "Task.h"
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>

// Forward declarations
class MPUSensorTask;
class DataLoggingTask;

class WebStreamingTask : public Task {
  public:
    static const uint16_t MASK { WEB_STREAMING_TASK_MASK };
    
    // Constructor and destructor
    WebStreamingTask(MPUSensorTask& mpuSensor, DataLoggingTask& dataLogger);
    ~WebStreamingTask();
    
    virtual uint16_t getMask() override {
      return WebStreamingTask::MASK;
    }
    
    virtual void run() override;
    
    // Event source management
    void setupEventSource(AsyncWebServer* server);
    void handleClientConnect(AsyncEventSourceClient* client);
    void handleClientDisconnect(AsyncEventSourceClient* client);
    
    // Data formatting
    String formatSensorData();
    void broadcastSensorData();
    
    // Client management
    int getConnectedClients() const;
    void sendInitialData(AsyncEventSourceClient* client);
    void forceDisconnectAll();
    void triggerCleanup();
    void broadcastClientCount(); // Broadcast updated client count to all clients
    
  private:
    MPUSensorTask& mpuSensor;
    DataLoggingTask& dataLogger;
    AsyncEventSource* events;
    
    // Streaming state
    unsigned long lastBroadcast;
    unsigned long broadcastInterval;
    
    // Data formatting helpers
    String formatFloat(float value, int decimalPlaces);
    String createJsonMessage();
    
    // Client management
    static const int MAX_CLIENTS = WEB_CLIENT_MAX;
    AsyncEventSourceClient* clients[MAX_CLIENTS];
    int clientCount;
    
    // Connection tracking for timeout cleanup
    unsigned long lastActivityTime[MAX_CLIENTS];
    static const unsigned long CLIENT_TIMEOUT_MS = 5000; // 5 seconds timeout
    
    // Utility methods
    void cleanupDisconnectedClients();
    bool addClient(AsyncEventSourceClient* client);
    void removeClient(AsyncEventSourceClient* client);
};

#endif
