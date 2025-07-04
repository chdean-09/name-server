# ESP32 Door Lock Mobile App Integration Prompt

I need to create a mobile app (React Native/Capacitor) that seamlessly integrates with my ESP32 door lock backend system. The backend is already implemented with WebSocket-first communication and HTTP fallback.

## Backend Details

### WebSocket Server
- **URL**: `ws://localhost:8000` (development)
- **Library**: Native `ws` with NestJS `WsAdapter`
- **Message Format**: `{ event: string, data: any }`
- **Auto-reconnection**: Required on client side

### HTTP API Base URL
- **Development**: `http://localhost:8000`
- **Production**: Replace with your domain

## Required App Features

### 1. WebSocket Connection Management
Create a WebSocket service that:
- Connects to `ws://localhost:8000`
- Handles automatic reconnection with exponential backoff
- Manages connection state (connecting, connected, disconnected, error)
- Queues messages when disconnected and sends when reconnected

### 2. Device Registration Flow
**WebSocket Message**: 
```json
{
  "event": "register-user",
  "data": {
    "userId": "unique-user-id"
  }
}
```

**Expected Response**:
```json
{
  "event": "register-user-response", 
  "data": {
    "success": true
  }
}
```

### 3. Door Control Commands
**WebSocket Message**:
```json
{
  "event": "send-command",
  "data": {
    "deviceId": "esp32-device-id",
    "command": "lock", // or "unlock"
    "doorId": 1, // 1, 2, or 3
    "userId": "user-id"
  }
}
```

**Expected Response**:
```json
{
  "event": "command-response",
  "data": {
    "success": true,
    "method": "websocket"
  }
}
```

**HTTP Fallback**: 
- **Endpoint**: `POST /device/command`
- **Body**: Same as WebSocket data
- **Headers**: `Content-Type: application/json`

### 4. Real-time Status Updates
**Listen for WebSocket Message**:
```json
{
  "event": "status-update",
  "data": {
    "deviceId": "esp32-device-id",
    "sensors": {
      "door1": 1, // 1 = locked, 0 = unlocked
      "door2": 0,
      "door3": 0,
      "buzzer": 0
    },
    "timestamp": "2025-07-04T12:00:00.000Z"
  }
}
```

### 5. Device Pairing Flow
**WebSocket Message**:
```json
{
  "event": "pairing-request",
  "data": {
    "macAddress": "AA:BB:CC:DD:EE:FF",
    "deviceName": "ESP32-DoorLock-001"
  }
}
```

**Expected Response**:
```json
{
  "event": "pairing-response",
  "data": {
    "success": true,
    "pairingCode": "123456"
  }
}
```

## HTTP API Endpoints (Fallback)

### Device Management
```
GET    /device/status/:deviceId     - Get device status
POST   /device/command             - Send lock/unlock command
POST   /device/pair                - Initiate device pairing
GET    /device/user/:userId        - Get user's devices
```

### Command Endpoint Details
**POST /device/command**
```json
{
  "deviceId": "esp32-device-id",
  "userId": "user-id", 
  "command": "LOCK", // or "UNLOCK"
  "doorId": 1
}
```

**Response**:
```json
{
  "success": true,
  "message": "Command sent successfully",
  "method": "http"
}
```

## App Requirements

### Core Features Needed:
1. **Connection Status Indicator** - Show WebSocket connection state
2. **Device List** - Show paired ESP32 devices
3. **Door Controls** - Lock/Unlock buttons for each door (1-3)
4. **Real-time Status** - Live door status updates
5. **Pairing Flow** - BLE discovery → Pairing → WiFi config
6. **Offline Mode** - Queue commands when offline, sync when online
7. **Error Handling** - User-friendly error messages

### Service Architecture:
```
App Component
    ↓
Device Hook (useDeviceControl)
    ↓
Hybrid Device Service
    ↓
WebSocket Service ←→ HTTP Service
```

### State Management:
- Device connection states
- Door lock states (locked/unlocked for doors 1-3)  
- WebSocket connection status
- Command queue for offline mode
- User authentication state

### UI Components Needed:
1. **Connection Status Bar** - WebSocket status indicator
2. **Device Card** - Shows device info and door controls
3. **Door Control Button** - Lock/unlock with loading states
4. **Pairing Screen** - BLE device discovery and pairing
5. **Settings Screen** - Device management

### Error Scenarios to Handle:
- WebSocket disconnection during command
- HTTP API timeout/failure
- Device offline/unreachable
- Invalid device credentials
- Network connectivity issues

### Example React Hook Pattern:
```typescript
const { 
  devices, 
  connectionStatus, 
  sendCommand, 
  isLoading, 
  error 
} = useDeviceControl(userId);

// Usage
await sendCommand('esp32-001', 'lock', 1);
```

### Platform-specific Notes:
- **Capacitor**: Use native HTTP plugin for better reliability
- **React Native**: Use WebSocket polyfill if needed
- **Both**: Implement background app state handling

## WebSocket Connection Best Practices:
1. **Reconnection Logic**: Exponential backoff (1s, 2s, 4s, 8s, max 30s)
2. **Heartbeat**: Send ping every 30s, reconnect if no pong
3. **Message Queuing**: Store failed messages, retry on reconnect
4. **State Sync**: Request device status on reconnect

## Testing Scenarios:
1. Send lock/unlock commands via WebSocket
2. Test HTTP fallback when WebSocket down
3. Verify real-time status updates
4. Test offline→online command queue sync
5. Test connection recovery after network loss

The backend is ready and fully implemented. Focus on creating a robust, user-friendly mobile app that provides seamless door control with real-time feedback and reliable offline handling.
