#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

// Replace with your network credentials
const char* ssid = "PLDTHOMEFIBR_AP5G";
const char* password = "AndradaFamily321-PLDT-5ghz";

const char* socketUrl = "example.com";

const int doorPin = 12;
const int sensorPin = 19;

// Buzzer pin
const int buzzerPin = 4;

WebSocketsClient webSocket;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("[WS] Disconnected");
      break;
    case WStype_CONNECTED:
      Serial.println("[WS] Connected to server");
      webSocket.sendTXT("{\"sensor\":1,\"lock\":0,\"buzzer\":0}");
      break;
    case WStype_TEXT:
      Serial.printf("[WS] Got message: %s\n", payload);
      // Handle lock/unlock
      if (strcmp((char*)payload, "{\"command\":\"unlock\"}") == 0) {
        digitalWrite(12, HIGH);
      } else if (strcmp((char*)payload, "{\"command\":\"lock\"}") == 0) {
        digitalWrite(12, LOW);
      }
      break;
  }
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

  webSocket.begin(socketUrl, 443, "/ws"); // adjust IP and port
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000); // auto reconnect every 5s

  Serial.println("WebSocket initialized.");

  // Start server
  server.begin();
}

void loop() {
  webSocket.loop(); // Handle WebSocket events

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
    webSocket.sendTXT(json);
  }

  // Small delay is optional, helps reduce loop churn
  delay(50);
}

