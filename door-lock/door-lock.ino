#include <WiFi.h>
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>

SocketIOclient socketIO;

const int doorPin = 12;
const int sensorPin = 19;
const int buzzerPin = 4;

// Replace with your network credentials
const char* ssid = "PLDTHOMEFIBR_AP";
const char* password = "AndradaFamily321-PLDT-2.4ghz";


void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case sIOtype_DISCONNECT:
            Serial.printf("[IOc] Disconnected!\n");
            break;
        case sIOtype_CONNECT:
            Serial.printf("[IOc] Connected to url: %s\n", payload);

            // join default namespace (no auto join in Socket.IO V3)
            socketIO.send(sIOtype_CONNECT, "/");
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

            // Message Includes a ID for a ACK (callback)
            if(id) {
                // create JSON message for Socket.IO (ack)
                DynamicJsonDocument docOut(1024);
                JsonArray array = docOut.to<JsonArray>();

                // add payload (parameters) for the ack (callback function)
                JsonObject param1 = array.createNestedObject();
                param1["now"] = millis();

                // JSON to String (serializion)
                String output;
                output += id;
                serializeJson(docOut, output);

                // Send event
                socketIO.send(sIOtype_ACK, output);
            }

            if (eventName == "command") {
              JsonObject payload = doc[1];
              String command = payload["command"];

              if (command == "lock") {
                digitalWrite(doorPin, LOW);
              } else if (command == "unlock") {
                digitalWrite(doorPin, HIGH);
              }
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

void setup() {
  Serial.begin(115200);

  // Set up pins
  pinMode(doorPin, OUTPUT);
  digitalWrite(doorPin, LOW);

  pinMode(sensorPin, INPUT_PULLDOWN);

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

  // server address, port and URL
  socketIO.beginSSL("name-server-production.up.railway.app", 443, "/socket.io/?EIO=4");

  // event handler
  socketIO.onEvent(socketIOEvent);

  socketIO.setReconnectInterval(5000); // Reconnect every 5 seconds if disconnected

  Serial.println("WebSocket initialized.");
}

void loop() {
  socketIO.loop(); // Handle WebSocket events

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");
    WiFi.begin(ssid, password);
    delay(5000);
  }


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

    // // create JSON message for Socket.IO (event)
    // DynamicJsonDocument doc(1024);
    // JsonArray array = doc.to<JsonArray>();

    // add event name
    // Hint: socket.on('device_status', ....
    // array.add("device_status");

    // // add payload (parameters) for the event
    // JsonObject payload = array.createNestedObject();
    // payload["sensor"] = sensorState == HIGH ? "1" : "0";
    // payload["lock"]   = lockState == HIGH ? "1" : "0";
    // payload["buzzer"] = buzzerOn ? "1" : "0";

    // // JSON to String (serialization)
    // String output;
    // serializeJson(doc, output);

    // // Send event
    // ✅ Build event payload
    DynamicJsonDocument payloadDoc(256);
    JsonObject payload = payloadDoc.to<JsonObject>();
    payload["sensor"] = sensorState == HIGH ? "1" : "0";
    payload["lock"]   = lockState == HIGH ? "1" : "0";
    payload["buzzer"] = buzzerOn ? "1" : "0";

    // ✅ Wrap it inside an array for socket.io
    DynamicJsonDocument eventDoc(512);
    JsonArray message = eventDoc.to<JsonArray>();
    message.add("device_status"); // Event name
    message.add(payload);         // Payload object

    // ✅ Serialize and send
    String jsonString;
    serializeJson(eventDoc, jsonString);
    socketIO.sendEVENT(jsonString);

    Serial.println(jsonString);
  }

  // Small delay is optional, helps reduce loop churn
  delay(50);
}

