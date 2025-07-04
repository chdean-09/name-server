#include <WiFi.h>
#include <WebServer.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WebSocketsClient.h>

// Replace with your server URL - UPDATE THESE TO MATCH YOUR NETWORK
const char* serverURL = "http://10.10.10.100"; // Update this with your actual server IP
const char* websocketURL = "10.10.10.100"; // WebSocket server URL (without http://)
const int websocketPort = 8000; // WebSocket port

// Device MAC address for identification
String deviceMAC;
String deviceAPIKey;
String deviceName; // Will be set dynamically in setup()

// WiFi credentials (will be received via BLE)
String wifiSSID = "";
String wifiPassword = "";

// Relay pins (door locks)
const int relayPins[1] = {12};

// Door sensor pins
const int doorSensorPins[1] = {19};

// Buzzer pin
const int buzzerPin = 4;

// LED pin for status indication
const int statusLED = 2;

// Create web server on port 80
WebServer server(80);

// WebSocket client
WebSocketsClient webSocket;
bool wsConnected = false;

// BLE Configuration
BLEServer* pServer = NULL;
BLECharacteristic* pWiFiCharacteristic = NULL;
BLECharacteristic* pStatusCharacteristic = NULL;
BLECharacteristic* pPairingCharacteristic = NULL;
bool deviceConnected = false;
bool isPaired = false;

// Preferences for storing settings
Preferences preferences;

// UUIDs for BLE service and characteristics
#define SERVICE_UUID           "12345678-1234-1234-1234-123456789abc"
#define WIFI_CHAR_UUID         "87654321-4321-4321-4321-cba987654321"
#define STATUS_CHAR_UUID       "11111111-2222-3333-4444-555555555555"
#define PAIRING_CHAR_UUID      "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee"

// Forward function declarations
void connectToWiFi();
void checkWiFiConnection();
void updateBLEStatus(String status);
void registerWithServer();
void sendHealthCheck();
void sendSensorDataViaWebSocket();
bool authenticateRequest();
void setupWebServer();
void handleWebSocketMessage(String message);
void connectWebSocket();
void setupPins();
void initializeBLE();
void startBLEAdvertising();

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      digitalWrite(statusLED, HIGH);
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      digitalWrite(statusLED, LOW);
      // Restart advertising
      BLEDevice::startAdvertising();
    }
};

class WiFiCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
      String jsonStr = String(pCharacteristic->getValue().c_str());
      
      Serial.println("Received WiFi config: " + jsonStr);
      
      // Quick manual JSON parsing to avoid timeout
      int ssidStart = jsonStr.indexOf("\"ssid\":\"") + 8;
      int ssidEnd = jsonStr.indexOf("\"", ssidStart);
      int passStart = jsonStr.indexOf("\"password\":\"") + 12;
      int passEnd = jsonStr.indexOf("\"", passStart);
      
      if (ssidStart > 7 && ssidEnd > ssidStart && passStart > 11 && passEnd > passStart) {
        wifiSSID = jsonStr.substring(ssidStart, ssidEnd);
        wifiPassword = jsonStr.substring(passStart, passEnd);
        
        Serial.println("Parsed SSID: " + wifiSSID);
        
        // Save to preferences immediately
        preferences.putString("wifi_ssid", wifiSSID);
        preferences.putString("wifi_password", wifiPassword);
        
        // Send quick acknowledgment
        pWiFiCharacteristic->setValue("OK");
        pWiFiCharacteristic->notify();
        
        // Start WiFi connection (non-blocking)
        connectToWiFi();
      } else {
        Serial.println("Failed to parse WiFi credentials");
        pWiFiCharacteristic->setValue("ERROR");
        pWiFiCharacteristic->notify();
      }
    }
};

class PairingCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
      String jsonStr = String(pCharacteristic->getValue().c_str());
      
      Serial.println("Received pairing config: " + jsonStr);
      
      // Quick manual JSON parsing for API key
      int keyStart = jsonStr.indexOf("\"apiKey\":\"") + 10;
      int keyEnd = jsonStr.indexOf("\"", keyStart);
      
      if (keyStart > 9 && keyEnd > keyStart) {
        deviceAPIKey = jsonStr.substring(keyStart, keyEnd);
        isPaired = true;
        
        Serial.println("Device paired with API key");
        
        // Save to preferences immediately
        preferences.putString("api_key", deviceAPIKey);
        preferences.putBool("is_paired", true);
        
        // Send quick confirmation
        pPairingCharacteristic->setValue("PAIRED");
        pPairingCharacteristic->notify();
      } else {
        Serial.println("Failed to parse API key");
        pPairingCharacteristic->setValue("ERROR");
        pPairingCharacteristic->notify();
      }
    }
};

void setup() {
  Serial.begin(115200);

  // Initialize preferences
  preferences.begin("doorlock", false);
  
  // Load saved settings
  wifiSSID = preferences.getString("wifi_ssid", "");
  wifiPassword = preferences.getString("wifi_password", "");
  deviceAPIKey = preferences.getString("api_key", "");
  isPaired = preferences.getBool("is_paired", false);

  // Get device MAC address
  deviceMAC = WiFi.macAddress();
  deviceMAC.replace(":", "");
  
  // Set dynamic device name using last 4 characters of MAC address
  deviceName = "ESP-Lock-" + deviceMAC.substring(8);
  Serial.println("Device name: " + deviceName);
  Serial.println("Device MAC: " + deviceMAC);

  // Set up pins
  setupPins();
  
  // Initialize BLE
  initializeBLE();
  
  // If WiFi credentials are saved, try to connect
  if (wifiSSID.length() > 0) {
    connectToWiFi();
  }
  
  // Start BLE advertising (for pairing or re-pairing)
  if (!isPaired) {
    startBLEAdvertising();
  }
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WSc] Disconnected!\n");
      wsConnected = false;
      break;
      
    case WStype_CONNECTED: {
      Serial.printf("[WSc] Connected to: %s\n", payload);
      wsConnected = true;
      
      // Register device with server - Keep original format that works
      String message = "{\"event\":\"register-device\",\"deviceId\":\"" + deviceMAC + 
                      "\",\"macAddress\":\"" + deviceMAC + "\"}";
      
      Serial.println("Sending registration: " + message);
      webSocket.sendTXT(message);
      break;
    }
      
    case WStype_TEXT:
      Serial.printf("[WSc] Received: %s\n", payload);
      handleWebSocketMessage((char*)payload);
      break;
      
    case WStype_BIN:
      Serial.printf("[WSc] Received binary length: %u\n", length);
      break;
      
    case WStype_PING:
      Serial.printf("[WSc] Received ping\n");
      break;
      
    case WStype_PONG:
      Serial.printf("[WSc] Received pong\n");
      break;
  }
}

void handleWebSocketMessage(String message) {
  Serial.println("Received WebSocket message: " + message);
  
  // Manual JSON parsing to avoid ArduinoJson overhead
  if (message.indexOf("\"event\":\"command\"") > -1) {
    // Extract command type from data object
    int typeStart = message.indexOf("\"type\":\"") + 8;
    int typeEnd = message.indexOf("\"", typeStart);
    
    // Extract doorId from data object
    int doorStart = message.indexOf("\"doorId\":") + 9;
    int doorEnd = message.indexOf(",", doorStart);
    if (doorEnd == -1) doorEnd = message.indexOf("}", doorStart);
    
    if (typeStart > 7 && typeEnd > typeStart && doorStart > 8 && doorEnd > doorStart) {
      String commandType = message.substring(typeStart, typeEnd);
      int doorId = message.substring(doorStart, doorEnd).toInt();
      
      Serial.println("Command: " + commandType + ", Door: " + String(doorId));
      
      if (doorId == 1) { // Only support door 1
        bool isLock = (commandType == "LOCK");
        digitalWrite(relayPins[0], isLock ? LOW : HIGH);
        
        Serial.println("Door " + String(doorId) + (isLock ? " locked" : " unlocked") + " via WebSocket");
        
        // Send simple confirmation
        String response = "{\"event\":\"command-result\",\"success\":true,\"doorId\":" + String(doorId) + "}";
        webSocket.sendTXT(response);
      } else {
        // Send error for invalid door ID
        String response = "{\"event\":\"command-result\",\"success\":false,\"error\":\"Invalid door ID\",\"doorId\":" + String(doorId) + "}";
        webSocket.sendTXT(response);
      }
    } else {
      Serial.println("Failed to parse WebSocket command");
      String response = "{\"event\":\"command-result\",\"success\":false,\"error\":\"Parse error\"}";
      webSocket.sendTXT(response);
    }
  }
}

void connectWebSocket() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  webSocket.begin(websocketURL, websocketPort, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
  
  Serial.println("WebSocket connection initiated...");
}

void setupPins() {
  // Set up relay pin
  pinMode(relayPins[0], OUTPUT);
  digitalWrite(relayPins[0], HIGH); // Start unlocked (assuming active LOW relay)

  // Set up door sensor pin
  pinMode(doorSensorPins[0], INPUT_PULLDOWN);

  // Set up buzzer and status LED
  pinMode(buzzerPin, OUTPUT);
  pinMode(statusLED, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  digitalWrite(statusLED, LOW);
}

void initializeBLE() {
  Serial.println("Initializing BLE...");
  
  BLEDevice::init(deviceName.c_str());
  
  // Set BLE connection parameters for better reliability
  BLEDevice::setMTU(512); // Increase MTU for better throughput
  
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  // WiFi credentials characteristic with larger value size
  pWiFiCharacteristic = pService->createCharacteristic(
                         WIFI_CHAR_UUID,
                         BLECharacteristic::PROPERTY_READ |
                         BLECharacteristic::PROPERTY_WRITE
                       );
  pWiFiCharacteristic->setCallbacks(new WiFiCharacteristicCallbacks());
  pWiFiCharacteristic->setValue("ready");

  // Status characteristic
  pStatusCharacteristic = pService->createCharacteristic(
                         STATUS_CHAR_UUID,
                         BLECharacteristic::PROPERTY_READ |
                         BLECharacteristic::PROPERTY_NOTIFY
                       );
  pStatusCharacteristic->addDescriptor(new BLE2902());
  pStatusCharacteristic->setValue("disconnected");

  // Pairing characteristic
  pPairingCharacteristic = pService->createCharacteristic(
                         PAIRING_CHAR_UUID,
                         BLECharacteristic::PROPERTY_READ |
                         BLECharacteristic::PROPERTY_WRITE |
                         BLECharacteristic::PROPERTY_NOTIFY
                       );
  pPairingCharacteristic->setCallbacks(new PairingCharacteristicCallbacks());
  pPairingCharacteristic->addDescriptor(new BLE2902());
  pPairingCharacteristic->setValue("unpaired");

  pService->start();
  Serial.println("BLE service started");
}

void startBLEAdvertising() {
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();
  Serial.println("BLE advertising started for pairing...");
}

// WiFi connection state
bool wifiConnecting = false;
unsigned long wifiConnectStart = 0;
const unsigned long WIFI_TIMEOUT = 20000; // 20 seconds

void connectToWiFi() {
  if (wifiSSID.length() == 0) return;
  
  Serial.println("Starting WiFi connection to: " + wifiSSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  
  wifiConnecting = true;
  wifiConnectStart = millis();
}

void checkWiFiConnection() {
  if (!wifiConnecting) return;
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnecting = false;
    Serial.println("\nConnected to WiFi.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Set up web server routes
    setupWebServer();
    server.begin();
    
    // Register with backend server
    registerWithServer();
    
    // Connect to WebSocket
    connectWebSocket();
    
    // Update status via BLE if connected
    if (deviceConnected) {
      updateBLEStatus("WIFI_CONNECTED");
    }
  } else if (millis() - wifiConnectStart > WIFI_TIMEOUT) {
    wifiConnecting = false;
    Serial.println("\nWiFi connection timeout");
    
    if (deviceConnected) {
      updateBLEStatus("WIFI_FAILED");
    }
  }
}

void setupWebServer() {

  // Define server routes
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/plain", "ESP32 Door Lock System - " + deviceMAC);
  });

  // Control individual relay (lock/unlock door) - with authentication
  server.on("/door", HTTP_GET, []() {
    if (!authenticateRequest()) {
      server.send(401, "text/plain", "Unauthorized");
      return;
    }
    
    if (server.hasArg("door") && server.hasArg("state")) {
      int door = server.arg("door").toInt();
      String state = server.arg("state");
      if (door == 1) { // Only support door 1
        if (state == "on") {
          digitalWrite(relayPins[0], LOW); // Lock
          server.send(200, "text/plain", "Door locked");
        } else if (state == "off") {
          digitalWrite(relayPins[0], HIGH); // Unlock
          server.send(200, "text/plain", "Door unlocked");
        } else {
          server.send(400, "text/plain", "Invalid state");
        }
      } else {
        server.send(400, "text/plain", "Invalid door - only door 1 supported");
      }
    } else {
      server.send(400, "text/plain", "Missing parameters");
    }
  });

  // Send buzzer status
  server.on("/buzzer", HTTP_GET, []() {
    if (!authenticateRequest()) {
      server.send(401, "text/plain", "Unauthorized");
      return;
    }
    int buzzerState = digitalRead(buzzerPin);
    server.send(200, "text/plain", String(buzzerState));
  });

  // Send door sensor state
  server.on("/door_sensor", HTTP_GET, []() {
    if (!authenticateRequest()) {
      server.send(401, "text/plain", "Unauthorized");
      return;
    }
    int state = digitalRead(doorSensorPins[0]);
    server.send(200, "text/plain", String(state));
  });

  // Health check endpoint
  server.on("/health", HTTP_GET, []() {
    // Manual JSON (more efficient)
    String response = "{\"status\":\"online\",\"deviceMAC\":\"" + deviceMAC + 
                     "\",\"isPaired\":" + (isPaired ? "true" : "false") +
                     ",\"wifiSSID\":\"" + wifiSSID +
                     "\",\"freeHeap\":" + String(ESP.getFreeHeap()) +
                     ",\"uptime\":" + String(millis()) + "}";
    
    server.send(200, "application/json", response);
  });
}

void loop() {
  // Check WiFi connection status (non-blocking)
  checkWiFiConnection();
  
  server.handleClient();
  
  // Handle WebSocket
  if (WiFi.status() == WL_CONNECTED) {
    webSocket.loop();
  }

  // Check sensors and buzzer logic
  int sensorState = digitalRead(doorSensorPins[0]);
  int lockState = digitalRead(relayPins[0]); // LOW = locked, HIGH = unlocked

  // If door is open while locked, activate buzzer
  bool buzzerOn = (sensorState == LOW && lockState == LOW);

  // Control buzzer
  digitalWrite(buzzerPin, buzzerOn ? HIGH : LOW);

  // Send sensor data via WebSocket (every 10 seconds)
  static unsigned long lastSensorUpdate = 0;
  if (millis() - lastSensorUpdate > 10000 && wsConnected) {
    sendSensorDataViaWebSocket();
    lastSensorUpdate = millis();
  }

  // Periodic health check to server (every 30 seconds)
  static unsigned long lastHealthCheck = 0;
  if (millis() - lastHealthCheck > 30000 && WiFi.status() == WL_CONNECTED) {
    sendHealthCheck();
    lastHealthCheck = millis();
  }

  // Handle BLE status updates
  if (deviceConnected) {
    static unsigned long lastBLEUpdate = 0;
    if (millis() - lastBLEUpdate > 5000) {
      updateBLEStatus("RUNNING");
      lastBLEUpdate = millis();
    }
  }
}

bool authenticateRequest() {
  if (!isPaired || deviceAPIKey.length() == 0) {
    return false;
  }
  
  String authHeader = server.header("Authorization");
  String expectedAuth = "Bearer " + deviceAPIKey;
  
  return authHeader == expectedAuth;
}

void registerWithServer() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  HTTPClient http;
  http.begin(String(serverURL) + "/devices/status/update");
  http.addHeader("Content-Type", "application/json");
  
  // Manual JSON string (smaller)
  String payload = "{\"macAddress\":\"" + deviceMAC + 
                  "\",\"ipAddress\":\"" + WiFi.localIP().toString() + "\"}";
  
  int httpResponseCode = http.POST(payload);
  
  if (httpResponseCode > 0) {
    Serial.println("Server registration: " + String(httpResponseCode));
  } else {
    Serial.println("Registration failed");
  }
  
  http.end();
}

void sendHealthCheck() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  HTTPClient http;
  http.begin(String(serverURL) + "/devices/status/update");
  http.addHeader("Content-Type", "application/json");
  
  // Manual JSON string (smaller)
  String payload = "{\"macAddress\":\"" + deviceMAC + 
                  "\",\"ipAddress\":\"" + WiFi.localIP().toString() + "\"}";
  
  int httpResponseCode = http.POST(payload);
  
  if (httpResponseCode > 0) {
    Serial.println("Health check OK");
  }
  
  http.end();
}

void updateBLEStatus(String status) {
  if (!deviceConnected) return;
  
  // Create JSON string manually (more memory efficient)
  String statusJson = "{\"status\":\"" + status + 
                     "\",\"deviceMAC\":\"" + deviceMAC +
                     "\",\"isPaired\":" + (isPaired ? "true" : "false") +
                     ",\"wifiConnected\":" + (WiFi.status() == WL_CONNECTED ? "true" : "false");
  
  if (WiFi.status() == WL_CONNECTED) {
    statusJson += ",\"ipAddress\":\"" + WiFi.localIP().toString() + "\"";
  }
  statusJson += "}";
  
  pStatusCharacteristic->setValue(statusJson.c_str());
  pStatusCharacteristic->notify();
}

void sendSensorDataViaWebSocket() {
  if (!wsConnected) return;
  
  // Create JSON string manually (smaller than ArduinoJson)
  String message = "{\"event\":\"device-status-update\",\"deviceId\":\"" + deviceMAC + 
                  "\",\"sensors\":{\"door1\":" + String(digitalRead(doorSensorPins[0])) +
                  ",\"buzzer\":" + String(digitalRead(buzzerPin)) + 
                  ",\"relay\":" + String(digitalRead(relayPins[0])) +
                  "},\"timestamp\":" + String(millis()) + "}";
  
  webSocket.sendTXT(message);
}
