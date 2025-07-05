/* eslint-disable @typescript-eslint/no-unsafe-assignment */
/* eslint-disable @typescript-eslint/no-unsafe-member-access */
/* eslint-disable @typescript-eslint/no-unsafe-call */
import {
  WebSocketGateway,
  WebSocketServer,
  SubscribeMessage,
  MessageBody,
  ConnectedSocket,
  OnGatewayConnection,
  OnGatewayDisconnect,
  OnGatewayInit,
  WsResponse,
} from '@nestjs/websockets';
import { Server } from 'ws';
import * as WebSocket from 'ws';
import { DeviceService } from '../device/device.service';

// Use any to avoid type issues with ws library for now
type ExtendedWebSocket = any;

@WebSocketGateway({
  cors: {
    origin: '*',
  },
})
export class DeviceGateway
  implements OnGatewayConnection, OnGatewayDisconnect, OnGatewayInit
{
  @WebSocketServer()
  server: Server;

  private userSockets = new Map<string, ExtendedWebSocket>();
  private deviceSockets = new Map<string, ExtendedWebSocket>();
  private clientCounter = 0;

  constructor(private deviceService: DeviceService) {}

  handleConnection(client: ExtendedWebSocket) {
    client.id = `client-${++this.clientCounter}`;
    client.isAlive = true;
    console.log('=== WebSocket Connection ===');
    console.log('Client connected:', client.id);
    console.log('Client URL:', client.url);
    console.log('Client protocol:', client.protocol);
    console.log('Total clients:', this.clientCounter);

    // Set up ping/pong for connection health
    client.on('pong', () => {
      client.isAlive = true;
    });

    // Add message logging and manual handling
    client.on('message', (rawMessage: any) => {
      console.log('=== Raw Message Received ===');
      console.log('Raw message type:', typeof rawMessage);
      const messageStr = String(rawMessage);
      console.log('Raw message:', messageStr);
      
      try {
        const parsed = JSON.parse(messageStr);
        console.log('Parsed message:', JSON.stringify(parsed, null, 2));
        
        // Handle device registration manually since @MessageBody() isn't working
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
  }

  handleDisconnect(client: ExtendedWebSocket) {
    console.log('Client disconnected:', client.id);

    // Clean up user and device mappings
    for (const [userId, socket] of this.userSockets) {
      if (socket.id === client.id) {
        this.userSockets.delete(userId);
        break;
      }
    }
    for (const [deviceId, socket] of this.deviceSockets) {
      if (socket.id === client.id) {
        this.deviceSockets.delete(deviceId);
        break;
      }
    }
  }

  @SubscribeMessage('register-user')
  handleUserRegistration(
    @MessageBody() data: { userId: string },
    @ConnectedSocket() client: ExtendedWebSocket,
  ): WsResponse<any> {
    client.userId = data.userId;
    this.userSockets.set(data.userId, client);
    console.log(`User ${data.userId} registered`);
    return { event: 'register-user-response', data: { success: true } };
  }

  @SubscribeMessage('register-device')
  async handleDeviceRegistration(
    @MessageBody() payload: any,
    @ConnectedSocket() client: ExtendedWebSocket,
  ): Promise<WsResponse<any>> {
    try {
      console.log('=== Device Registration Debug ===');
      console.log('Raw payload received:', payload);
      console.log('Payload type:', typeof payload);
      
      // The issue is that @MessageBody() is not getting the data correctly
      // Let's handle this by extracting from the full message instead
      let data: { deviceId: string; macAddress: string };
      
      if (!payload) {
        console.error(
          'Payload is undefined - this indicates a NestJS message parsing issue',
        );
        return {
          event: 'register-device-response',
          data: { success: false, error: 'Message parsing failed' },
        };
      }
      
      // If payload is the direct data (deviceId and macAddress)
      if (payload.deviceId && payload.macAddress) {
        data = payload;
        console.log('Using direct payload format');
      }
      // If payload has nested data
      else if (
        payload.data &&
        payload.data.deviceId &&
        payload.data.macAddress
      ) {
        data = payload.data;
        console.log('Using nested data format');
      }
      // If we somehow got the full message object
      else if (payload.event === 'register-device' && payload.deviceId) {
        data = { deviceId: payload.deviceId, macAddress: payload.macAddress };
        console.log('Using event object format');
      } else {
        console.error('Unknown payload format:', JSON.stringify(payload));
        return {
          event: 'register-device-response',
          data: { success: false, error: 'Invalid message format' },
        };
      }
      
      console.log('Processed data:', JSON.stringify(data));
      
      if (!data || !data.deviceId || !data.macAddress) {
        console.error('Invalid device registration data:', data);
        return {
          event: 'register-device-response',
          data: { success: false, error: 'Invalid registration data' },
        };
      }

      client.deviceId = data.deviceId;
      this.deviceSockets.set(data.deviceId, client);

      // Update device status to online
      await this.deviceService.updateDeviceStatus(data.macAddress, 'websocket');
      console.log(`Device ${data.deviceId} registered successfully`);

      return { event: 'register-device-response', data: { success: true } };
    } catch (error) {
      console.error('Error in handleDeviceRegistration:', error);
      return {
        event: 'register-device-response',
        data: { success: false, error: 'Internal server error' },
      };
    }
  }

  @SubscribeMessage('send-command')
  async handleSendCommand(
    @MessageBody()
    data: {
      deviceId: string;
      command: string;
      doorId: number;
      userId: string;
    },
  ): Promise<WsResponse<any>> {
    try {
      // Validate command type
      const commandType = data.command === 'lock' ? 'LOCK' : 'UNLOCK';
      console.log(
        `Sending command ${commandType} to device ${data.deviceId} for door ${data.doorId}`,
      );

      // Send command through device service (HTTP fallback)
      await this.deviceService.sendCommandToDevice(
        data.deviceId,
        data.userId,
        commandType,
        data.doorId,
      );

      // Also send via WebSocket if device is connected
      const deviceSocket = this.deviceSockets.get(data.deviceId);
      if (deviceSocket && deviceSocket.readyState === WebSocket.OPEN) {
        this.sendToWebSocket(deviceSocket, {
          event: 'command',
          data: {
            type: commandType,
            doorId: data.doorId,
            timestamp: new Date().toISOString(),
          },
        });
      }

      return {
        event: 'command-response',
        data: { success: true, method: 'websocket' },
      };
    } catch (error: any) {
      return {
        event: 'command-response',
        data: { success: false, error: error.message, method: 'websocket' },
      };
    }
  }

  @SubscribeMessage('device-status-update')
  handleDeviceStatusUpdate(
    @MessageBody()
    data: {
      deviceId: string;
      sensors: { door1: number; door2: number; door3: number; buzzer: number };
    },
  ): WsResponse<any> {
    // Broadcast status to all users
    const statusUpdate = {
      event: 'status-update',
      data: {
        deviceId: data.deviceId ?? "<unknown-device-id>",
        sensors: data.sensors,
        timestamp: new Date().toISOString(),
      },
    };

    // Send to all connected users
    for (const [, userSocket] of this.userSockets) {
      if (userSocket.readyState === WebSocket.OPEN) {
        this.sendToWebSocket(userSocket, statusUpdate);
      }
    }

    return { event: 'device-status-response', data: { success: true } };
  }

  @SubscribeMessage('pairing-request')
  async handlePairingRequest(
    @MessageBody() data: { macAddress: string; deviceName: string },
  ): Promise<WsResponse<any>> {
    try {
      const result = await this.deviceService.initiatePairing(
        data.macAddress,
        data.deviceName,
      );
      return {
        event: 'pairing-response',
        data: { success: true, pairingCode: result.pairingCode },
      };
    } catch (error: any) {
      return {
        event: 'pairing-response',
        data: { success: false, error: error.message },
      };
    }
  }

  // Manual message handlers to work around @MessageBody() parsing issues
  private async handleDeviceRegistrationManual(
    message: any,
    client: ExtendedWebSocket,
  ) {
    try {
      console.log('=== Manual Device Registration ===');
      console.log('Full message:', JSON.stringify(message, null, 2));
      
      // Extract data from the message
      let data: { deviceId: string; macAddress: string };
      
      // Try different message formats
      if (message.deviceId && message.macAddress) {
        // Direct format: { event: 'register-device', deviceId: '...', macAddress: '...' }
        data = { deviceId: message.deviceId, macAddress: message.macAddress };
        console.log('Using direct message format');
      } else if (
        message.data &&
        message.data.deviceId &&
        message.data.macAddress
      ) {
        // Nested format: { event: 'register-device', data: { deviceId: '...', macAddress: '...' } }
        data = message.data;
        console.log('Using nested data format');
      } else {
        console.error('Invalid device registration message format:', message);
        this.sendToWebSocket(client, {
          event: 'register-device-response',
          data: { success: false, error: 'Invalid message format' },
        });
        return;
      }
      
      console.log('Extracted registration data:', data);
      
      if (!data.deviceId || !data.macAddress) {
        console.error('Missing required fields:', data);
        this.sendToWebSocket(client, {
          event: 'register-device-response',
          data: { success: false, error: 'Missing deviceId or macAddress' },
        });
        return;
      }

      // Register the device
      client.deviceId = data.deviceId;
      this.deviceSockets.set(data.deviceId, client);

      // Update device status to online
      await this.deviceService.updateDeviceStatus(data.macAddress, 'websocket');
      console.log(
        `Device ${data.deviceId} registered successfully via manual handler`,
      );

      // Send success response
      this.sendToWebSocket(client, {
        event: 'register-device-response',
        data: { success: true },
      });
    } catch (error) {
      console.error('Error in manual device registration:', error);
      this.sendToWebSocket(client, {
        event: 'register-device-response',
        data: { success: false, error: 'Internal server error' },
      });
    }
  }

  private handleDeviceStatusUpdateManual(
    message: any,
    client: ExtendedWebSocket,
  ) {
    try {
      console.log('=== Manual Device Status Update ===');
      console.log('Full message:', JSON.stringify(message, null, 2));
      
      // Extract data from the message
      let data: { deviceId: string; sensors: any };
      
      if (message.deviceId && message.sensors) {
        // Direct format
        data = { deviceId: message.deviceId, sensors: message.sensors };
      } else if (
        message.data &&
        message.data.deviceId &&
        message.data.sensors
      ) {
        // Nested format
        data = message.data;
      } else {
        console.error('Invalid status update message format:', message);
        return;
      }
      
      console.log('Extracted status data:', data);
      
      // Broadcast status to all users
      const statusUpdate = {
        event: 'status-update',
        data: {
          deviceId: data.deviceId,
          sensors: data.sensors,
          timestamp: new Date().toISOString(),
        },
      };

      // Send to all connected users
      for (const [, userSocket] of this.userSockets) {
        if (userSocket.readyState === WebSocket.OPEN) {
          this.sendToWebSocket(userSocket, statusUpdate);
        }
      }

      // Send acknowledgment back to device
      this.sendToWebSocket(client, {
        event: 'device-status-response',
        data: { success: true },
      });
    } catch (error) {
      console.error('Error in manual status update:', error);
    }
  }

  // Helper method to send data via WebSocket
  private sendToWebSocket(socket: ExtendedWebSocket, message: any) {
    if (socket.readyState === WebSocket.OPEN) {
      socket.send(JSON.stringify(message));
    }
  }

  // Method to send notifications to specific users
  notifyUser(userId: string, message: any) {
    const socket = this.userSockets.get(userId);
    if (socket && socket.readyState === WebSocket.OPEN) {
      this.sendToWebSocket(socket, {
        event: 'notification',
        data: message,
      });
    }
  }

  // Method to send commands to specific devices
  sendToDevice(deviceId: string, message: any): boolean {
    const socket = this.deviceSockets.get(deviceId);
    if (socket && socket.readyState === WebSocket.OPEN) {
      this.sendToWebSocket(socket, {
        event: 'command',
        data: message,
      });
      return true;
    }
    return false;
  }

  // Health check - ping clients periodically
  startHealthCheck() {
    setInterval(() => {
      if (this.server && this.server.clients) {
        this.server.clients.forEach((client: ExtendedWebSocket) => {
          if (!client.isAlive) {
            client.terminate();
            return;
          }
          client.isAlive = false;
          client.ping();
        });
      }
    }, 30000); // Every 30 seconds
  }

  afterInit() {
    console.log('WebSocket Gateway initialized');
    this.startHealthCheck();
  }
}
