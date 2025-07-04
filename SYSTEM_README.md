# ESP32 Door Lock System with Bluetooth LE Pairing

This system implements a secure door lock control system using ESP32 microcontrollers with Bluetooth LE pairing and a NestJS backend server.

## Features

### ESP32 Features
- **Bluetooth LE Pairing**: Secure device pairing with mobile app
- **WiFi Configuration**: Receives WiFi credentials via BLE during pairing
- **Device Authentication**: API key-based authentication for all commands
- **3 Door Control**: Independent control of up to 3 doors
- **Door Sensors**: Monitors door open/close status
- **Buzzer Alert**: Alerts when door is opened while locked
- **Health Monitoring**: Periodic health checks to server
- **One-Device-One-Owner**: Each device can only be paired with one account

### Backend Features
- **WebSocket Real-time Communication**: Live updates between app and server
- **Device Management**: Track device status, ownership, and commands
- **Secure Pairing Process**: Temporary pairing codes for secure device claiming
- **Command History**: Track all lock/unlock commands
- **User Management**: Multi-user support with device ownership

## Architecture

```
Mobile App <-> WebSocket <-> NestJS Backend <-> HTTP/WebSocket <-> ESP32 Device
           <-> Bluetooth LE (pairing only) <->
```

## Installation

### Backend Setup

1. Install dependencies:
```bash
npm install
```

2. Set up the database:
```bash
# Create .env file with DATABASE_URL
npx prisma migrate dev --name init
npx prisma generate
```

3. Start the server:
```bash
npm run start:dev
```

### ESP32 Setup

1. Install required Arduino libraries:
   - WiFi
   - WebServer
   - BLEDevice
   - ArduinoJson
   - Preferences
   - HTTPClient

2. Update the server URL in the ESP32 code:
```cpp
const char* serverURL = "http://your-server-domain.com";
```

3. Upload the code to your ESP32

## Pairing Process

1. **ESP32 Initialization**: Device starts BLE advertising with a unique service UUID
2. **Mobile App Discovery**: App discovers ESP32 via BLE scan
3. **Pairing Request**: App sends pairing request with device MAC and name
4. **Server Validation**: Server generates temporary pairing code
5. **Code Exchange**: ESP32 receives pairing code via BLE
6. **WiFi Configuration**: App sends WiFi credentials via BLE
7. **Device Registration**: ESP32 connects to WiFi and registers with server
8. **Pairing Completion**: Device is now paired and can receive commands

## API Endpoints

### Device Management
- `POST /devices/initiate-pairing` - Start pairing process
- `POST /devices/complete-pairing` - Complete pairing with WiFi credentials
- `POST /devices/:deviceId/command` - Send lock/unlock command
- `GET /devices/user/:userId` - Get user's devices
- `GET /devices/status/:macAddress` - Get device status

### WebSocket Events
- `register-user` - Register user for real-time updates
- `register-device` - Register device for commands
- `send-command` - Send lock/unlock command
- `device-status-update` - Receive device status updates
- `pairing-request` - Initiate pairing via WebSocket

## Security Features

1. **One-Time Pairing**: Each device can only be paired once
2. **API Key Authentication**: All HTTP requests require valid API key
3. **Temporary Pairing Codes**: Pairing codes expire after use
4. **Owner Verification**: Only device owners can send commands
5. **Secure BLE Communication**: BLE characteristics protected during pairing

## ESP32 Pin Configuration

- **Relay Pins**: 23, 22, 21 (for doors 1, 2, 3)
- **Door Sensor Pins**: 19, 18, 5 (for doors 1, 2, 3)
- **Buzzer Pin**: 4
- **Status LED**: 2

## Database Schema

### Users
- id, email, passwordHash, devices[], createdAt, updatedAt

### Devices
- id, name, macAddress, ipAddress, wifiSSID, ownerId, isClaimed, isOnline, lastSeen, apiKey

### Commands
- id, deviceId, userId, type (LOCK/UNLOCK), doorId, status, sentAt, executedAt

### PairingSessions
- id, deviceId, userId, isComplete, startedAt, completedAt

## Usage Flow

1. **Device Setup**: Flash ESP32 with the code
2. **Pairing**: Use mobile app to pair with ESP32 via BLE
3. **WiFi Configuration**: App sends WiFi credentials to ESP32
4. **Device Registration**: ESP32 connects to WiFi and registers with server
5. **Command Control**: Send lock/unlock commands via app
6. **Real-time Updates**: Receive device status via WebSocket

## Error Handling

- Device offline detection
- WiFi connection failures
- Command execution failures
- Authentication errors
- Pairing timeout handling

## Future Enhancements

- Multiple WiFi network support
- Over-the-air (OTA) updates
- Advanced scheduling
- Integration with home automation systems
- Mobile push notifications
- Enhanced security features
