# BLE Timeout Fix Documentation

## Problem
The ESP32 was experiencing "configuration failed. write timeout" errors when receiving WiFi credentials via BLE. This was caused by blocking operations in BLE characteristic callbacks.

## Root Causes
1. **ArduinoJson parsing in callbacks**: JSON parsing with `DynamicJsonDocument` was slow and blocking
2. **Blocking WiFi connection**: The `WiFi.begin()` followed by a `while` loop blocked the BLE callback
3. **Long delays in callbacks**: `delay(100)` in the callback was causing timeouts
4. **No MTU optimization**: Default MTU was too small for reliable large data transfer

## Solutions Implemented

### 1. Manual JSON Parsing
Replaced `ArduinoJson` with lightweight manual string parsing:
```cpp
// Old: ArduinoJson parsing (slow, blocking)
DynamicJsonDocument doc(1024);
deserializeJson(doc, value);

// New: Fast manual parsing
int ssidStart = jsonStr.indexOf("\"ssid\":\"") + 8;
int ssidEnd = jsonStr.indexOf("\"", ssidStart);
```

### 2. Non-blocking WiFi Connection
Implemented a state machine approach:
- `connectToWiFi()`: Just starts the connection (non-blocking)
- `checkWiFiConnection()`: Called in main loop to check status
- No more blocking while loops in callbacks

### 3. Immediate Acknowledgments
All BLE callbacks now send immediate responses:
```cpp
pWiFiCharacteristic->setValue("OK");
pWiFiCharacteristic->notify();
```

### 4. BLE Optimizations
- Increased MTU to 512 bytes: `BLEDevice::setMTU(512)`
- Set initial values for all characteristics
- Better error handling with "ERROR" responses

### 5. Removed Delays
- No more `delay()` calls in BLE callbacks
- All time-consuming operations moved to main loop

## Testing Recommendations

### 1. BLE Connection Test
```javascript
// Test basic BLE connection
await device.gatt.connect();
console.log('Connected to device');
```

### 2. WiFi Credential Transfer Test
```javascript
const credentials = JSON.stringify({
  ssid: "YourWiFiNetwork",
  password: "YourPassword"
});

await wifiCharacteristic.writeValue(new TextEncoder().encode(credentials));
const response = await wifiCharacteristic.readValue();
console.log('Response:', new TextDecoder().decode(response));
```

### 3. Monitor Serial Output
Watch for these messages:
- "Received WiFi config: ..."
- "Parsed SSID: ..."
- "Starting WiFi connection to: ..."
- "Connected to WiFi." or "WiFi connection timeout"

## Expected Behavior

1. **Fast BLE Responses**: All characteristic writes should get immediate acknowledgments
2. **No Timeouts**: BLE operations should complete within 3-5 seconds
3. **Proper State Updates**: Status characteristic should update with connection progress
4. **Non-blocking Operations**: Device remains responsive during WiFi connection

## If Issues Persist

### 1. Check MTU Negotiation
- Some phones may not support 512-byte MTU
- Monitor for MTU negotiation messages in serial output

### 2. Verify JSON Format
- Ensure JSON is properly formatted: `{"ssid":"NetworkName","password":"Password"}`
- Check for special characters in WiFi credentials

### 3. Signal Strength
- Keep phone close to ESP32 during BLE transfer
- Ensure stable power supply to ESP32

### 4. Phone-specific Issues
- Test with different phones/browsers
- Some devices have BLE implementation quirks

## Code Structure Changes

- **WiFi State Machine**: Added `wifiConnecting`, `wifiConnectStart` variables
- **Non-blocking Functions**: `connectToWiFi()` and `checkWiFiConnection()`
- **Optimized Callbacks**: Fast parsing and immediate responses
- **Better Error Handling**: Clear error messages for debugging

This fix should resolve the BLE timeout issues and provide a more robust pairing experience.
