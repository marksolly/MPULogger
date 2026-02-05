#include "WebStreamingTask.h"
#include "MPUSensorTask.h"
#include "DataLoggingTask.h"

WebStreamingTask::WebStreamingTask(MPUSensorTask& mpuSensor, DataLoggingTask& dataLogger)
  : Task(), // Use default constructor
    mpuSensor(mpuSensor),
    dataLogger(dataLogger),
    events(nullptr),
    lastBroadcast(0),
    broadcastInterval(200),
    clientCount(0) {
  
  // Set the run interval for this specific task
  runInterval = 200; // 200ms interval for real-time updates
  
  // Initialize client array
  for (int i = 0; i < MAX_CLIENTS; i++) {
    clients[i] = nullptr;
    lastActivityTime[i] = 0;
  }
}

WebStreamingTask::~WebStreamingTask() {
  // Clean up all clients
  forceDisconnectAll();
  
  // Clean up event source
  if (events != nullptr) {
    delete events;
    events = nullptr;
  }
}

void WebStreamingTask::run() {
  // Clean up disconnected clients
  cleanupDisconnectedClients();
  
  // Broadcast sensor data to all connected clients
  if (millis() - lastBroadcast >= broadcastInterval && clientCount > 0) {
    broadcastSensorData();
    lastBroadcast = millis();
  }
}

void WebStreamingTask::setupEventSource(AsyncWebServer* server) {
  events = new AsyncEventSource("/events");
  
  // Set up connection handler
  events->onConnect([this](AsyncEventSourceClient* client) {
    Serial.println(F("EventSource client connected"));
    handleClientConnect(client);
    
    client->client()->onDisconnect([](void *arg, AsyncClient *c){
        Serial.println("Client Disconnected");
    }, nullptr);
    
    // Optional: Handle timeouts
    client->client()->onTimeout([](void *arg, AsyncClient *c, uint32_t time){
        Serial.println("Client Timeout");
    }, nullptr);
  });
  
  // Add handler to server
  server->addHandler(events);
  Serial.println(F("EventSource setup complete"));
}

void WebStreamingTask::handleClientConnect(AsyncEventSourceClient* client) {
  if (addClient(client)) {
    // Send initial data to new client
    sendInitialData(client);
    
    // Send connection acknowledgment with client count
    String connectMsg = "{\"type\":\"connected\",\"message\":\"Real-time data streaming active\",\"clients\":" + String(clientCount) + "}";
    client->send(connectMsg.c_str(), "connect", millis());
    
    Serial.print(F("Streaming client connected. Total clients: "));
    Serial.println(clientCount);
  } else {
    // Send rejection message if max clients reached
    client->send("{\"type\":\"error\",\"message\":\"Maximum streaming clients reached\"}", "error", millis());
    client->close();
  }
}

void WebStreamingTask::handleClientDisconnect(AsyncEventSourceClient* client) {
  removeClient(client);
  Serial.print(F("EventSource client disconnected. Total clients: "));
  Serial.println(clientCount);
  
  // Broadcast updated client count to remaining clients
  if (clientCount > 0) {
    broadcastClientCount();
  }
}

String WebStreamingTask::formatSensorData() {
  return createJsonMessage();
}

void WebStreamingTask::broadcastSensorData() {
  String jsonMessage = createJsonMessage();
  
  // Send to all connected clients with error handling
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i] != nullptr && clients[i]->connected()) {
      // Check if send was successful by verifying client is still connected
      clients[i]->send(jsonMessage.c_str(), "sensor_data", millis());
      
      // Update activity time on send attempt
      lastActivityTime[i] = millis();
    }
  }
}

int WebStreamingTask::getConnectedClients() const {
  return clientCount;
}

void WebStreamingTask::sendInitialData(AsyncEventSourceClient* client) {
  // Send current sensor values and recording status
  String initialJson = createJsonMessage();
  client->send(initialJson.c_str(), "initial_data", millis());
}

String WebStreamingTask::formatFloat(float value, int decimalPlaces) {
  char buffer[16];
  dtostrf(value, 1, decimalPlaces, buffer);
  return String(buffer);
}

String WebStreamingTask::createJsonMessage() {
  String json = "{";
  json += "\"timestamp\":" + String(millis()) + ",";
  json += "\"accel\":{";
  json += "\"x\":" + formatFloat(mpuSensor.accel_x, 2) + ",";
  json += "\"y\":" + formatFloat(mpuSensor.accel_y, 2) + ",";
  json += "\"z\":" + formatFloat(mpuSensor.accel_z, 2);
  json += "},";
  json += "\"orientation\":{";
  json += "\"yaw\":" + formatFloat(mpuSensor.yaw, 1) + ",";
  json += "\"pitch\":" + formatFloat(mpuSensor.pitch, 1) + ",";
  json += "\"roll\":" + formatFloat(mpuSensor.roll, 1);
  json += "},";
  json += "\"recording\":" + String(dataLogger.isRecording() ? "true" : "false") + ",";
  json += "\"calibrated\":" + String(mpuSensor.isCalibrated ? "true" : "false") + ",";
  json += "\"fifoCount\":" + String(mpuSensor.fifoCount);
  json += "}";
  
  return json;
}

void WebStreamingTask::cleanupDisconnectedClients() {
  unsigned long currentTime = millis();
  
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i] != nullptr) {
      // Check if client is disconnected or timed out
      bool shouldRemove = !clients[i]->connected();
      
      // Add timeout check - if client hasn't received data for CLIENT_TIMEOUT_MS
      if (!shouldRemove && lastActivityTime[i] > 0) {
        if (currentTime - lastActivityTime[i] > CLIENT_TIMEOUT_MS) {
          Serial.println(F("Client timeout - removing inactive connection"));
          shouldRemove = true;
        }
      }
      
      if (shouldRemove) {
        removeClient(clients[i]);
      }
    }
  }
}

bool WebStreamingTask::addClient(AsyncEventSourceClient* client) {
  // Find empty slot
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i] == nullptr) {
      clients[i] = client;
      lastActivityTime[i] = millis();
      clientCount++;
      return true;
    }
  }
  
  // No empty slots available
  Serial.println(F("Max streaming clients reached"));
  return false;
}

void WebStreamingTask::removeClient(AsyncEventSourceClient* client) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i] == client) {
      clients[i] = nullptr;
      lastActivityTime[i] = 0;
      clientCount--;
      break;
    }
  }
}

void WebStreamingTask::forceDisconnectAll() {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i] != nullptr) {
      Serial.println(F("Force disconnecting existing streaming client"));
      clients[i]->close();
      removeClient(clients[i]);
    }
  }
}

void WebStreamingTask::triggerCleanup() {
  cleanupDisconnectedClients();
}

void WebStreamingTask::broadcastClientCount() {
  String countMsg = "{\"type\":\"client_count\",\"count\":" + String(clientCount) + "}";
  
  // Send to all connected clients
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i] != nullptr && clients[i]->connected()) {
      clients[i]->send(countMsg.c_str(), "client_count", millis());
      lastActivityTime[i] = millis();
    }
  }
}
