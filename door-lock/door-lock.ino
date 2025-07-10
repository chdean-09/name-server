#include <WiFi.h>
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <Preferences.h>

Preferences prefs;
SocketIOclient socketIO;

const int doorPin = 12;
const int sensorPin = 19;
const int buzzerPin = 4;

// BLE Service/Characteristic UUIDs
#define SERVICE_UUID        "f92a1d0c-cd74-40b9-b7c1-806d8fe67c81"
#define CHAR_UUID           "4deee94e-85f5-4caf-a388-49182aaa1adb"

BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;


bool credentialsReceived = false;
String ssid = "";
String password = "";
String deviceName = "";
String userEmail = "";
String deviceId = "";
bool shouldUnpair = false;

void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case sIOtype_DISCONNECT:
            Serial.printf("[IOc] Disconnected!\n");
            break;
        case sIOtype_CONNECT:
            Serial.printf("[IOc] Connected to url: %s\n", payload);

            // join default namespace (no auto join in Socket.IO V3)
            socketIO.send(sIOtype_CONNECT, "/");

            if (deviceId.length() == 0) {
              // send register_device
              DynamicJsonDocument event(512);
              JsonArray message = event.to<JsonArray>();
              message.add("register_device");
              JsonObject payload = message.createNestedObject();
              payload["deviceName"] = deviceName;
              payload["userEmail"] = userEmail;

              String jsonString;
              serializeJson(event, jsonString);
              socketIO.sendEVENT(jsonString);
            }

            break;
        case sIOtype_EVENT:
        {
            char * sptr = NULL;
            int id = strtol((char *)payload, &sptr, 10);
            Serial.printf("[IOc] get event: %s id: %d\n", payload, id);
            if(id) {
                payload = (uint8_t *)sptr;
            }
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, payload, length);
            if(error) {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.c_str());
                return;
            }

            String eventName = doc[0];
            Serial.printf("[IOc] event name: %s\n", eventName.c_str());

            if (eventName == "command") {
              JsonObject payload = doc[1];
              String command = payload["command"];

              Serial.println("[IOc] Command received: " + command);

              if (command == "lock") {
                digitalWrite(doorPin, LOW);
              } else if (command == "unlock") {
                digitalWrite(doorPin, HIGH);
              }
            }

            if (eventName == "register_device") {
              JsonObject payload = doc[1];
              bool success = payload["success"];

              if (success) {
                String receivedDeviceId = payload["deviceId"];
                String receivedName = payload["deviceName"];

                Serial.println("✅ Device registered with backend.");
                Serial.println("Device ID: " + receivedDeviceId);
                Serial.println("Name: " + receivedName);

                // Save to flash
                prefs.begin("smartlock", false);
                prefs.putString("deviceId", receivedDeviceId);
                prefs.putString("deviceName", receivedName);
                prefs.end();

                deviceId = receivedDeviceId; // So we can use it right away
                
                // Read current sensor and lock states
                int sensorState = digitalRead(sensorPin); // LOW = door open, HIGH = closed
                int lockState = digitalRead(doorPin);     // LOW = locked, HIGH = unlocked

                // 🔔 Trigger buzzer if door is open while locked
                bool buzzerOn = (sensorState == LOW && lockState == LOW);

                sendDeviceStatus(socketIO, deviceId, deviceName, sensorState, lockState, buzzerOn);
              } else {
                String errorMsg = payload["error"] | "Unknown error";
                Serial.println("❌ Failed toregisterdevice: " + errorMsg);
              }
            }

            if (eventName == "unpair_device") {
              Serial.println("[IOc] Unpairing device...");

              // Clear stored preferences
              prefs.begin("smartlock", false);
              prefs.clear();
              prefs.end();

              // Reset internal state
              deviceId = "";
              ssid = "";
              password = "";
              deviceName = "";
              credentialsReceived = false;

              // Mark for reset in loop()
              shouldUnpair = true;

              // Clean disconnect (optional but recommended)
              socketIO.disconnect();
            }

            if (eventName == "request_status") {
              // Read current sensor and lock states
              int sensorState = digitalRead(sensorPin); // LOW = door open, HIGH = closed
              int lockState = digitalRead(doorPin);     // LOW = locked, HIGH = unlocked

              // 🔔 Trigger buzzer if door is open while locked
              bool buzzerOn = (sensorState == LOW && lockState == LOW);

              sendDeviceStatus(socketIO, deviceId, deviceName, sensorState, lockState, buzzerOn);
            }
        }
            break;
        case sIOtype_ACK:
            Serial.printf("[IOc] get ack: %u\n", length);
            break;
        case sIOtype_ERROR:
            Serial.printf("[IOc] get error: %u\n", length);
            break;
        case sIOtype_BINARY_EVENT:
            Serial.printf("[IOc] get binary: %u\n", length);
            break;
        case sIOtype_BINARY_ACK:
            Serial.printf("[IOc] get binary ack: %u\n", length);
            break;
    }
}

class SmartLockBLECallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pChar) override {
    std::string value = pChar->getValue().c_str();  // This line should work if getValue() returns std::string

    // Use value.c_str() to convert std::string to const char*
    DynamicJsonDocument doc(256);
    DeserializationError err = deserializeJson(doc, value.c_str());

    if (!err) {
      ssid = doc["ssid"].as<String>();
      password = doc["pass"].as<String>();
      deviceName = doc["deviceName"].as<String>();
      userEmail = doc["userEmail"].as<String>();
      credentialsReceived = true;

      Serial.println("✅ Received credentials via BLE:");
      Serial.println("SSID: " + ssid);
      Serial.println("PASS: " + password);
      Serial.println("Device Name: " + deviceName);
      Serial.println("User Email: " + userEmail);

      prefs.begin("smartlock", false);
      prefs.putString("ssid", ssid);
      prefs.putString("password", password);
      prefs.putString("deviceName", deviceName);
      prefs.putString("userEmail", userEmail);
      prefs.end();
    } else {
      Serial.println("❌ Failed to parse BLE WiFi credentials.");
    }
  }
};


void setupBLE() {
  BLEDevice::init("NAME-Smartlock-" + String((uint32_t)ESP.getEfuseMac(), HEX));
  pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  pCharacteristic = pService->createCharacteristic(
    CHAR_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );

  pCharacteristic->setCallbacks(new SmartLockBLECallbacks());

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();

  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();
  Serial.println("🔵 BLE advertising started.");
}

void connectToWiFi() {
  BLEDevice::deinit(true); // Turn off BLE
  delay(1000);

  prefs.begin("smartlock", true);
  ssid = prefs.getString("ssid", "");
  password = prefs.getString("password", "");
  deviceName = prefs.getString("deviceName", "");
  prefs.end();

  Serial.println("📶 Connecting to WiFi...");
  WiFi.begin(ssid.c_str(), password.c_str());

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry++ < 20) {
    delay(1000);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n❌ WiFi failed to connect. Check credentials.");
  }
}

void setupSocket() {
  socketIO.beginSSL("name-server-production.up.railway.app", 443, "/socket.io/?EIO=4");

  socketIO.onEvent(socketIOEvent);

  socketIO.setReconnectInterval(5000); // Reconnect every 5 seconds if disconnected

  Serial.println("WebSocket initialized.");
}

void sendDeviceStatus(SocketIOclient& socket, const String& deviceId, const String& deviceName, int sensorState, int lockState, bool buzzerOn) {
  DynamicJsonDocument event(256);
  JsonArray message = event.to<JsonArray>();
  message.add("heartbeat");

  JsonObject payload = message.createNestedObject();
  payload["userEmail"] = userEmail;
  payload["deviceId"] = deviceId;
  payload["deviceName"] = deviceName;
  payload["sensor"] = sensorState == HIGH ? "closed" : "open";
  payload["lock"]   = lockState == HIGH ? "unlocked" : "locked";
  payload["buzzer"] = buzzerOn ? "on" : "off";

  String jsonString;
  serializeJson(event, jsonString);
  socket.sendEVENT(jsonString);

  Serial.println("📤 Sent status update:");
  Serial.println(jsonString);
}


// Setup function runs once at startup
void setup() {
  Serial.begin(115200);

  pinMode(doorPin, OUTPUT); 
  digitalWrite(doorPin, LOW); // Start locked
  pinMode(sensorPin, INPUT_PULLDOWN); // Use pull-down to detect door open (LOW) or closed (HIGH)
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW); // Start with buzzer off

  // 🔐 Restore credentials from flash
  prefs.begin("smartlock", true);
  ssid = prefs.getString("ssid", "");
  password = prefs.getString("password", "");
  deviceName = prefs.getString("deviceName", "");
  deviceId = prefs.getString("deviceId", "");
  userEmail = prefs.getString("userEmail", "");
  prefs.end();

  // ✅ If credentials were saved, consider them received
  if (ssid != "" && password != "" && deviceName != "") {
    credentialsReceived = true;
  }

  if (deviceId.length() > 0) {
    connectToWiFi();
    if (WiFi.status() == WL_CONNECTED) {
      setupSocket();
    }
  } else {
    setupBLE(); // Only if unpaired
  }
}

unsigned long lastHeartbeat = 0;

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input == "reset") {
      Serial.println("🧹 Clearing preferences...");
      prefs.begin("smartlock", false);
      prefs.clear();
      prefs.end();
      Serial.println("✅ Preferences cleared.");
      ESP.restart(); // Optional but useful
    }
  }

  if (shouldUnpair) {
    shouldUnpair = false;

    // Fully stop Wi-Fi and WebSocket
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(500);

    setupBLE();  // restart BLE advertising
    return;
  }

  // Skip main logic if not paired yet
  if (!credentialsReceived) {
    delay(100);
    return;
  }


  // Stop BLE and connect WiFi once credentials are received
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
    if (WiFi.status() == WL_CONNECTED) {
      setupSocket();
    } else {
      return;
    }
  }

  socketIO.loop(); // Handle WebSocket events

  // Read current sensor and lock states
  int sensorState = digitalRead(sensorPin); // LOW = door open, HIGH = closed
  int lockState = digitalRead(doorPin);     // LOW = locked, HIGH = unlocked

  // 🔔 Trigger buzzer if door is open while locked
  bool buzzerOn = (sensorState == LOW && lockState == LOW);
  digitalWrite(buzzerPin, buzzerOn ? HIGH : LOW);

  unsigned long now = millis();
  if (now - lastHeartbeat > 3000) { // every 3 seconds
    lastHeartbeat = now;

    sendDeviceStatus(socketIO, deviceId, deviceName, sensorState, lockState, buzzerOn);
  }

  // 📡 Notify clients if state changes
  static int lastSensorState = -1;
  static int lastLockState = -1;
  static bool lastBuzzerState = false;

  if (sensorState != lastSensorState || lockState != lastLockState || buzzerOn != lastBuzzerState) {
    lastSensorState = sensorState;
    lastLockState = lockState;
    lastBuzzerState = buzzerOn;

    sendDeviceStatus(socketIO, deviceId, deviceName, sensorState, lockState, buzzerOn);
  }

  // Small delay is optional, helps reduce loop churn
  delay(50);
}

