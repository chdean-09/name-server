#include <WiFi.h>
#include <WebServer.h>

// Replace with your network credentials
const char* ssid = "OPPO A18";
const char* password = "q9pzjmgg";

// Relay pins (door locks)
const int relayPins[3] = {23, 22, 21};

// Door sensor pins
const int doorSensorPins[3] = {19, 18, 5};

// Buzzer pin
const int buzzerPin = 4;

// Create web server on port 80
WebServer server(80);

void setup() {
  Serial.begin(115200);

  // Set up relay pins
  for (int i = 0; i < 3; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW); // Assuming active HIGH relay
  }

  // Set up door sensor pins
  for (int i = 0; i < 3; i++) {
    pinMode(doorSensorPins[i], INPUT_PULLDOWN);
  }

  // Set up buzzer pin
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW); // Buzzer OFF initially

  WiFi.mode(WIFI_STA);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Define server routes
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/plain", "ESP32 Door Lock System");
  });

  // Control individual relay (lock/unlock door)
  server.on("/door", HTTP_GET, []() {
    if (server.hasArg("door") && server.hasArg("state")) {
      int door = server.arg("door").toInt();
      String state = server.arg("state");
      if (door >= 1 && door <= 3) {
        if (state == "on") {
          digitalWrite(relayPins[door - 1], LOW); // Lock
          server.send(200, "text/plain", "Door " + String(door) + " locked");
        } else if (state == "off") {
          digitalWrite(relayPins[door - 1], HIGH); // Unlock
          server.send(200, "text/plain", "Door " + String(door) + " unlocked");
        } else {
          server.send(400, "text/plain", "Invalid state");
        }
      } else {
        server.send(400, "text/plain", "Invalid door");
      }
    } else {
      server.send(400, "text/plain", "Missing parameters");
    }
  });

  //Send buzzer status
  server.on("/buzzer", HTTP_GET, []() {
    int buzzerState = digitalRead(buzzerPin);
    server.send(200, "text/plain", String(buzzerState)); // 1=ON, 0=OFF
  });

  // Send door sensor 1 state
  server.on("/door_sensor1", HTTP_GET, []() {
    int state = digitalRead(doorSensorPins[0]);
    server.send(200, "text/plain", String(state));
  });

  // Send door sensor 2 state
  server.on("/door_sensor2", HTTP_GET, []() {
    int state = digitalRead(doorSensorPins[1]);
    server.send(200, "text/plain", String(state));
  });

  // Send door sensor 3 state
  server.on("/door_sensor3", HTTP_GET, []() {
    int state = digitalRead(doorSensorPins[2]);
    server.send(200, "text/plain", String(state));
  });

  // Start server
  server.begin();
}

void loop() {
  server.handleClient();

  // Check sensors and buzzer logic
  bool buzzerOn = false;
  for (int i = 0; i < 3; i++) {
    int sensorState = digitalRead(doorSensorPins[i]);
    int lockState = digitalRead(relayPins[i]); // LOW = locked, HIGH = unlocked

    // If door is open while locked, activate buzzer
    if (sensorState == LOW && lockState == LOW) {
      buzzerOn = true;
    }
  }

  // Control buzzer
  digitalWrite(buzzerPin, buzzerOn ? HIGH : LOW);
}
