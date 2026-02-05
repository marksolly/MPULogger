#include "WebServerTask.h"
#include "WebStreamingTask.h"
#include "Tasks.h"
#include "TestDataGenerator.h"
#include "MPULogRecord.h"
#include "FS.h"
#include "ArduinoJSON/ArduinoJson-v6.18.3.h"

WebServerTask::WebServerTask(Settings& settings) 
  : Task(), // Use default constructor
    settings(settings),
    server(80),
    dnsServer(),
    captivePortalIP(192, 168, 0, 1),
    serverStarted(false),
    apModeEnabled(false) {
  
  // Set the run interval for this specific task
  runInterval = 100; // 100ms interval for responsive DNS handling
}

void WebServerTask::run() {
  if (!serverStarted) {
    beginServer();
    serverStarted = true;
  }
  
  // Handle DNS requests for captive portal
  if (apModeEnabled) {
    handleDNSRequests();
  }
}

bool WebServerTask::initCaptivePortal() {
  Serial.println(F("Begin WiFi AP mode"));
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  
  // Set up event handler for client connections
  wifiStaConnectHandler = WiFi.onSoftAPModeStationConnected([](const WiFiEventSoftAPModeStationConnected& evt) {
    Serial.println(F("WiFi captive client connected"));
  });
  
  // Configure AP
  WiFi.softAPConfig(captivePortalIP, captivePortalIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(settings.hostName, nullptr, 1);
  
  Serial.println(F("Starting captive portal"));
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", captivePortalIP);
  
  apModeEnabled = true;
  return true;
}

void WebServerTask::handleDNSRequests() {
  dnsServer.processNextRequest();
}

void WebServerTask::beginServer() {
  // Initialize captive portal
  initCaptivePortal();
  
  // Set up server routes
  setupRoutes();
  
  // Begin server
  server.begin();
  Serial.println(F("Web server started"));
  Serial.print(F("AP IP address: "));
  Serial.println(captivePortalIP);
}

void WebServerTask::setupRoutes() {
  // Root endpoint
  server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    logRequest(request);
    handleRoot(request);
  });
  
  // API endpoints
  server.on("/api/files", HTTP_GET, [this](AsyncWebServerRequest *request) {
    logRequest(request);
    handleFileList(request);
  });
  
  server.on("/api/settings", HTTP_GET, [this](AsyncWebServerRequest *request) {
    logRequest(request);
    handleSettings(request);
  });
  
  server.on("/api/settings", HTTP_POST, [this](AsyncWebServerRequest *request) {
    logRequest(request);
    // TODO: Handle settings update
    sendJsonResponse(request, "{\"status\":\"ok\"}");
  });
  
  server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
    logRequest(request);
    handleStatus(request);
  });
  
  server.on("/api/meta", HTTP_GET, [this](AsyncWebServerRequest *request) {
    logRequest(request);
    handleMeta(request);
  });
  
  server.on("/api/testdata/generate", HTTP_POST, [this](AsyncWebServerRequest *request) {
    logRequest(request);
    handleTestData(request);
  });
  
  // Static file serving
  server.on("/favicon.ico", HTTP_GET, [this](AsyncWebServerRequest *request) {
    logRequest(request);
    handleStaticFile(request, "favicon.ico");
  });
  
  // Catch-all handler for dynamic file serving and captive portal
  server.onNotFound([this](AsyncWebServerRequest *request) {
    logRequest(request);
    String path = request->url();
    
    if (request->method() == HTTP_GET || request->method() == HTTP_HEAD) {
      // If path ends with "/", append "index.htm"
      if (path.endsWith("/")) {
        path += "index.htm";
      }
      
      // Check for gzip version first, then regular file
      String finalPath = path;
      if (SPIFFS.exists(path + ".gz")) {
        finalPath = path + ".gz";
      } else if (SPIFFS.exists(path)) {
        finalPath = path;
      } else {
        // File not found - serve index for captive portal functionality
        Serial.print(F("HTTP 404: File not found, serving captive portal: "));
        Serial.println(path);
        handleRoot(request);
        return;
      }
      
      String contentType = getContentType(path);
      
      if (request->method() == HTTP_HEAD) {
        // For HEAD requests, just send headers without body
        AsyncWebServerResponse *response = request->beginResponse(200, contentType, "");
        if (finalPath.endsWith(".gz")) {
          response->addHeader("Content-Encoding", "gzip");
        }
        // Add file size header for HEAD requests
        File file = SPIFFS.open(finalPath, "r");
        if (file) {
          response->addHeader("Content-Length", String(file.size()));
          file.close();
        }
        request->send(response);
      } else {
        // FIXED: Pass SPIFFS and path directly - library handles binary correctly
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, finalPath, contentType);
        if (finalPath.endsWith(".gz")) {
          response->addHeader("Content-Encoding", "gzip");
        }
        request->send(response);
      }
      
    } else if (request->method() == HTTP_DELETE) {
      // Handle file deletion via DELETE /filename.ext
      if (path.startsWith("/")) {
        path = path.substring(1); // Remove leading slash
      }
      
      // Security check - prevent directory traversal
      if (path.indexOf('/') != -1 || path.indexOf('\\') != -1) {
        sendErrorResponse(request, 400, "Invalid filename");
        return;
      }
      
      if (deleteFile("/" + path)) {
        sendJsonResponse(request, "{\"status\":\"ok\",\"message\":\"File deleted\"}");
        Serial.print(F("File deleted via DELETE: "));
        Serial.println(path);
      } else {
        sendErrorResponse(request, 500, "Error deleting file");
      }
    } else {
      Serial.print(F("HTTP 405: Method not allowed: "));
      Serial.println(request->methodToString());
      
      // Determine which methods are allowed based on the path
      String allowHeader = "GET, HEAD"; // Default for static files
      if (path.startsWith("/api/")) {
        if (path == "/api/files") {
          allowHeader = "GET";
        } else if (path.startsWith("/api/settings")) {
          allowHeader = "GET, POST";
        } else if (path == "/api/status") {
          allowHeader = "GET";
        } else if (path == "/api/meta") {
          allowHeader = "GET";
        } else if (path == "/api/testdata/generate") {
          allowHeader = "POST";
        } else {
          allowHeader = "GET"; // Default for other API endpoints
        }
      }
      
      AsyncWebServerResponse *response = request->beginResponse(405, "text/plain", "Method Not Allowed");
      response->addHeader("Allow", allowHeader);
      request->send(response);
    }
  });
}

void WebServerTask::logRequest(AsyncWebServerRequest *request) {
  String clientIP = request->client()->remoteIP().toString();
  String method = request->methodToString();
  String url = request->url();
  
  Serial.print(F("WEB REQUEST: "));
  Serial.print(clientIP);
  Serial.print(F(" "));
  Serial.print(method);
  Serial.print(F(" "));
  Serial.println(url);
  
  // Log query parameters if any
  if (request->params() > 0) {
    Serial.print(F("QUERY PARAMS: "));
    for (int i = 0; i < request->params(); i++) {
      if (i > 0) Serial.print(F(", "));
      AsyncWebParameter* p = request->getParam(i);
      Serial.print(p->name());
      Serial.print(F("="));
      Serial.print(p->value());
    }
    Serial.println();
  }
}

void WebServerTask::handleRoot(AsyncWebServerRequest *request) {
  // Trigger cleanup of any stale streaming connections before serving the page
  webStreamingTask.triggerCleanup();
  
  // Serve the main web interface
  handleStaticFile(request, "index.htm");
}

void WebServerTask::handleFileList(AsyncWebServerRequest *request) {
  String json = listFiles();
  Serial.println(F("FILES API RESPONSE:"));
  Serial.println(json);
  sendJsonResponse(request, json);
}


void WebServerTask::handleSettings(AsyncWebServerRequest *request) {
  // Return current settings as JSON
  String json = "{";
  json += "\"hostName\":\"" + String(settings.hostName) + "\",";
  json += "\"sampleRateMs\":" + String(settings.sampleRateMs) + ",";
  json += "\"maxLogFiles\":" + String(settings.maxLogFiles) + ",";
  json += "\"bufferSize\":" + String(settings.bufferSize) + ",";
  json += "\"autoCalibration\":" + String(settings.autoCalibration ? "true" : "false") + ",";
  json += "\"accelRange\":" + String(settings.accelRange) + ",";
  json += "\"gyroRange\":" + String(settings.gyroRange);
  json += "}";
  
  sendJsonResponse(request, json);
}

void WebServerTask::handleStatus(AsyncWebServerRequest *request) {
  // Return system status as JSON
  String json = "{";
  json += "\"uptime\":" + String(millis()) + ",";
  json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"wifiMode\":\"AP\",";
  json += "\"apIp\":\"" + captivePortalIP.toString() + "\",";
  json += "\"connectedClients\":" + String(WiFi.softAPgetStationNum());
  json += "}";
  
  sendJsonResponse(request, json);
}

void WebServerTask::handleMeta(AsyncWebServerRequest *request) {
  // Return metadata about the log structure
  String json = "{";
  json += "\"recordSize\":" + String(MPULogRecord::getRecordSize());
  json += "}";
  
  sendJsonResponse(request, json);
}

void WebServerTask::handleStaticFile(AsyncWebServerRequest *request, const String& filename) {
  String fullPath = "/" + filename;
  
  if (!SPIFFS.exists(fullPath)) {
    Serial.print(F("HTTP 404: Static file not found: "));
    Serial.println(fullPath);
    request->send(404);
    return;
  }
  
  String contentType = getContentType(filename);
  
  // FIXED: Pass SPIFFS and path directly. 
  // The library handles the opening/closing lifecycle safely.
  Serial.print("Responding with request->send(SPIFFS, ");
  Serial.print(fullPath);
  Serial.print(", ");
  Serial.print(contentType);
  Serial.println(")");
  request->send(SPIFFS, fullPath, contentType);
}

String WebServerTask::getContentType(const String& filename) {
  if (filename.endsWith(".html") || filename.endsWith(".htm")) return "text/html";
  if (filename.endsWith(".css")) return "text/css";
  if (filename.endsWith(".js")) return "application/javascript";
  if (filename.endsWith(".png")) return "image/png";
  if (filename.endsWith(".jpg") || filename.endsWith(".jpeg")) return "image/jpeg";
  if (filename.endsWith(".gif")) return "image/gif";
  if (filename.endsWith(".ico")) return "image/x-icon";
  if (filename.endsWith(".json")) return "application/json";
  if (filename.endsWith(".bin")) return "application/octet-stream";
  return "text/plain";
}

bool WebServerTask::fileExists(const String& filename) {
  return SPIFFS.exists(filename);
}

String WebServerTask::listFiles() {
  String json = "{\"files\":[";
  
  Dir dir = SPIFFS.openDir("/");
  bool first = true;
  
  while (dir.next()) {
    if (!first) json += ",";
    
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    
    json += "{";
    json += "\"name\":\"" + fileName + "\",";
    json += "\"size\":" + String(fileSize);
    json += "}";
    
    first = false;
  }
  
  json += "]}";
  return json;
}

String WebServerTask::getFileContent(const String& filename) {
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    return "";
  }
  
  String content = file.readString();
  file.close();
  return content;
}


bool WebServerTask::deleteFile(const String& filename) {
  return SPIFFS.remove(filename);
}

void WebServerTask::sendJsonResponse(AsyncWebServerRequest *request, const String& json) {
  request->send(200, "application/json", json);
}

void WebServerTask::handleTestData(AsyncWebServerRequest *request) {
  // Parse request parameters
  String testType = "combined";  // Default test type
  String filename = "";
  
  if (request->hasParam("type", true)) {
    testType = request->getParam("type", true)->value();
  }
  
  if (request->hasParam("filename", true)) {
    filename = request->getParam("filename", true)->value();
  } else {
    // Generate default filename based on test type
    if (testType == "motion") {
      filename = "test_motion.bin";
    } else if (testType == "static") {
      filename = "test_static.bin";
    } else {
      filename = "test_combined.bin";
    }
  }
  
  // Validate filename
  if (filename.indexOf('/') != -1 || filename.indexOf('\\') != -1 || !filename.endsWith(".bin")) {
    sendErrorResponse(request, 400, "Invalid filename");
    return;
  }
  
  Serial.printf("Generating test data: type=%s, filename=%s\n", testType.c_str(), filename.c_str());
  
  bool success = false;
  
  // Generate test data based on type
  if (testType == "motion") {
    success = TestDataGenerator::generateBasicMotionTest(filename.c_str());
  } else if (testType == "static") {
    success = TestDataGenerator::generateStaticTest(filename.c_str());
  } else if (testType == "combined") {
    success = TestDataGenerator::generateCombinedTest(filename.c_str());
  } else {
    sendErrorResponse(request, 400, "Invalid test type. Use 'motion', 'static', or 'combined'");
    return;
  }
  
  if (success) {
    // Return success response with file info
    String json = "{";
    json += "\"status\":\"ok\",";
    json += "\"message\":\"Test data generated successfully\",";
    json += "\"filename\":\"" + filename + "\",";
    json += "\"type\":\"" + testType + "\"";
    json += "}";
    
    sendJsonResponse(request, json);
    Serial.printf("Test data generation completed: %s\n", filename.c_str());
  } else {
    sendErrorResponse(request, 500, "Failed to generate test data");
    Serial.printf("ERROR: Test data generation failed: %s\n", filename.c_str());
  }
}

void WebServerTask::sendErrorResponse(AsyncWebServerRequest *request, int code, const String& message) {
  String json = "{\"error\":\"" + message + "\"}";
  Serial.print(F("HTTP ERROR "));
  Serial.print(code);
  Serial.print(F(": "));
  Serial.println(message);
  request->send(code, "application/json", json);
}
