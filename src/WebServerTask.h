#ifndef WEB_SERVER_TASK_H
#define WEB_SERVER_TASK_H

#include "Task.h"
#include "Settings.h"
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <DNSServer.h>

class WebServerTask : public Task {
  public:
    static const uint16_t MASK { WEB_SERVER_TASK_MASK };
    
    // Constructor
    WebServerTask(Settings& settings);
    
    virtual uint16_t getMask() override {
      return WebServerTask::MASK;
    }
    
    virtual void run() override;
    
    // WiFi and AP management
    bool initCaptivePortal();
    bool initWiFi();
    void handleDNSRequests();
    
    // Server management
    void setupRoutes();
    void beginServer();
    
    // API endpoints
    void handleRoot(AsyncWebServerRequest *request);
    void handleFileList(AsyncWebServerRequest *request);
    void handleFileData(AsyncWebServerRequest *request);
    void handleFileDelete(AsyncWebServerRequest *request);
    void handleSettings(AsyncWebServerRequest *request);
    void handleStatus(AsyncWebServerRequest *request);
    void handleMeta(AsyncWebServerRequest *request);
    void handleTestData(AsyncWebServerRequest *request);
    
    // Static file serving
    void handleStaticFile(AsyncWebServerRequest *request, const String& filename);
    
    // Utility methods
    String getContentType(const String& filename);
    bool fileExists(const String& filename);
    
  private:
    Settings& settings;
    DNSServer dnsServer;
    
    const IPAddress captivePortalIP;
    WiFiEventHandler wifiStaConnectHandler;
    
  public:
    AsyncWebServer server;
    
    // Server state
    bool serverStarted;
    bool apModeEnabled;
    
    // File management helpers
    String listFiles();
    String getFileContent(const String& filename);
    bool deleteFile(const String& filename);
    
    // Response helpers
    void sendJsonResponse(AsyncWebServerRequest *request, const String& json);
    void sendErrorResponse(AsyncWebServerRequest *request, int code, const String& message);
    
    // Request logging
    void logRequest(AsyncWebServerRequest *request);
};

#endif
