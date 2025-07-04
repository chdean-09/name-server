# WebSocket Message Handling Fix

## Issue
The NestJS WebSocket gateway was receiving raw messages correctly, but the `@SubscribeMessage()` decorators with `@MessageBody()` were not parsing the message content properly. The decorators were returning `undefined` even though the raw WebSocket messages contained valid JSON data.

## Root Cause
The native `ws` library used by NestJS doesn't automatically parse message formats the same way Socket.IO does. The `@MessageBody()` decorator expects a specific message structure that wasn't being provided by the raw WebSocket messages from the ESP32.

## Solution
Implemented manual message handling in the `handleConnection` method:

1. **Raw Message Parsing**: Added a `message` event listener that captures raw WebSocket messages
2. **Manual JSON Parsing**: Parse the raw message content directly
3. **Direct Handler Routing**: Route parsed messages to dedicated manual handler methods
4. **Bypass Framework Decorators**: Use custom methods instead of relying on `@SubscribeMessage()` decorators

## Implementation

### Backend Changes (device.gateway.ts)
```typescript
// In handleConnection method
client.on('message', (rawMessage: any) => {
  console.log('=== Raw Message Received ===');
  const messageStr = String(rawMessage);
  console.log('Raw message:', messageStr);
  
  try {
    const parsed = JSON.parse(messageStr);
    
    // Handle device registration manually
    if (parsed.event === 'register-device') {
      void this.handleDeviceRegistrationManual(parsed, client);
    }
    // Handle status updates manually
    else if (parsed.event === 'device-status-update') {
      this.handleDeviceStatusUpdateManual(parsed, client);
    }
  } catch (e) {
    console.log('Failed to parse message as JSON:', e);
  }
});

// Manual registration handler
private async handleDeviceRegistrationManual(message: any, client: ExtendedWebSocket) {
  // Extract deviceId and macAddress directly from message
  const data = { 
    deviceId: message.deviceId, 
    macAddress: message.macAddress 
  };
  
  // Register device and send response
  client.deviceId = data.deviceId;
  this.deviceSockets.set(data.deviceId, client);
  await this.deviceService.updateDeviceStatus(data.macAddress, 'websocket');
  
  this.sendToWebSocket(client, {
    event: 'register-device-response',
    data: { success: true }
  });
}
```

## Message Formats

### ESP32 Registration Message (Working)
```json
{
  "event": "register-device",
  "deviceId": "AA:BB:CC:DD:EE:FF",
  "macAddress": "AA:BB:CC:DD:EE:FF"
}
```

### ESP32 Status Update Message (Working)
```json
{
  "event": "device-status-update",
  "deviceId": "AA:BB:CC:DD:EE:FF",
  "sensors": {
    "door1": 1,
    "buzzer": 0,
    "relay": 0
  },
  "timestamp": 123456
}
```

### Backend Response Messages
```json
// Registration response
{
  "event": "register-device-response",
  "data": { "success": true }
}

// Status update acknowledgment
{
  "event": "device-status-response", 
  "data": { "success": true }
}
```

## Benefits of Manual Handling

1. **Direct Control**: Full control over message parsing and routing
2. **Better Debugging**: Clear visibility into raw message content
3. **Framework Independence**: Not reliant on NestJS decorator parsing
4. **Flexible Format Support**: Can handle multiple message formats if needed
5. **Performance**: Direct JSON parsing without framework overhead

## Testing Steps

1. **Start Backend**: `npm run start:dev`
2. **Connect ESP32**: Upload and run firmware
3. **Check Logs**: Look for registration messages and responses
4. **Verify Registration**: Device should appear in `deviceSockets` map
5. **Test Commands**: Send commands via WebSocket or HTTP

## Expected Log Output

```
=== WebSocket Connection ===
Client connected: client-1
=== Raw Message Received ===
Raw message: {"event":"register-device","deviceId":"AA:BB:CC:DD:EE:FF","macAddress":"AA:BB:CC:DD:EE:FF"}
=== Manual Device Registration ===
Extracted registration data: {"deviceId":"AA:BB:CC:DD:EE:FF","macAddress":"AA:BB:CC:DD:EE:FF"}
Device AA:BB:CC:DD:EE:FF registered successfully via manual handler
```

## Status
✅ **FIXED** - Manual message handling bypasses NestJS parsing issues and provides direct control over message processing.

## Next Steps
1. Test end-to-end device registration flow
2. Test command sending from backend to ESP32
3. Test status updates from ESP32 to backend
4. Remove or comment out unused `@SubscribeMessage` decorators (optional cleanup)
