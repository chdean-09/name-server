#include <WiFi.h>
#include <WebServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// Replace with your network credentials
const char* ssid = "qwertypasd";
const char* password = "2444666668888888";

const int doorPin = 12;
const int sensorPin = 19;

// Buzzer pin
const int buzzerPin = 4;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void notifySensorState() {
  int state = digitalRead(sensorPin);
  String json = "{\"sensor\":" + String(state) + "}";
  ws.textAll(json);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;

  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String msg = String((char*)data);

    // Expecting: {"command":"lock"} or {"command":"unlock"}
    DynamicJsonDocument doc(128);
    DeserializationError error = deserializeJson(doc, msg);

    String command = doc["command"];

    if (command == "lock") {
      digitalWrite(doorPin, LOW); // Lock the door
    } else if (command == "unlock") {
      digitalWrite(doorPin, HIGH); // Unlock the door
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT: {
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
  
      // Send current state to new client
      String json = "{\"sensor\":" + String(digitalRead(sensorPin)) +
                    ",\"lock\":" + String(digitalRead(doorPin)) +
                    ",\"buzzer\":" + String(digitalRead(buzzerPin)) + "}";
      client->text(json);  // Send only to the new client
      Serial.println(json);
      break;
    }
    case WS_EVT_DISCONNECT:
      Serial.println("Client disconnected");
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_ERROR:
    case WS_EVT_PONG:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}


void setup() {
  Serial.begin(115200);

  pinMode(doorPin, OUTPUT);
  digitalWrite(doorPin, LOW);

  pinMode(sensorPin, INPUT_PULLDOWN);


  // Set up buzzer pin
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW); // Buzzer OFF initially

  WiFi.mode(WIFI_STA);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  initWebSocket();

  Serial.println("WebSocket initialized.");

  // Define server routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
     request->send(200, "text/plain", "ESP32 Door Lock System with WebSocket");
  });

  // Start server
  server.begin();
}

void loop() {
  ws.cleanupClients();  // Clean up WebSocket clients

  // Read current sensor and lock states
  int sensorState = digitalRead(sensorPin); // LOW = door open, HIGH = closed
  int lockState = digitalRead(doorPin);     // LOW = locked, HIGH = unlocked

  // 🔔 Trigger buzzer if door is open while locked
  bool buzzerOn = (sensorState == LOW && lockState == LOW);
  digitalWrite(buzzerPin, buzzerOn ? HIGH : LOW);

  // 📡 Notify clients if state changes
  static int lastSensorState = -1;
  static int lastLockState = -1;
  static bool lastBuzzerState = false;

  if (sensorState != lastSensorState || lockState != lastLockState || buzzerOn != lastBuzzerState) {
    lastSensorState = sensorState;
    lastLockState = lockState;
    lastBuzzerState = buzzerOn;

    String json = "{\"sensor\":" + String(sensorState) +
                  ",\"lock\":" + String(lockState) +
                  ",\"buzzer\":" + String(buzzerOn) + "}";
    Serial.println(json);
    ws.textAll(json);
  }

  // Small delay is optional, helps reduce loop churn
  delay(50);
}

