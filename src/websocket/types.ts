import * as WebSocket from 'ws';

// Custom WebSocket interface with additional properties
export interface ExtendedWebSocket extends WebSocket {
  id?: string;
  userId?: string;
  deviceId?: string;
  isAlive?: boolean;
}
