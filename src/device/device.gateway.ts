/* eslint-disable @typescript-eslint/no-unused-vars */
import { forwardRef, Inject } from '@nestjs/common';
import {
  ConnectedSocket,
  MessageBody,
  SubscribeMessage,
  WebSocketGateway,
  WebSocketServer,
} from '@nestjs/websockets';
import { Server, Socket } from 'socket.io';
import { DeviceListService } from 'src/device-list/device-list.service';

interface DeviceState {
  lock: string; // "locked" | "unlocked"
  sensor: string; // "open" | "closed"
  buzzer: string; // "on" | "off"
}

@WebSocketGateway({
  cors: {
    origin: '*',
  },
})
export class DeviceGateway {
  constructor(
    @Inject(forwardRef(() => DeviceListService))
    private readonly deviceListService: DeviceListService,
  ) {}
  @WebSocketServer()
  server: Server;

  private heartbeatTimers = new Map<string, NodeJS.Timeout>();
  private heartbeatInterval = 10000; // 10 seconds

  handleConnection(client: Socket) {
    console.log('🔗 Client connected:', client.id);
  }

  handleDisconnect(client: Socket) {
    console.log('🔌 Client disconnected:', client.id);
  }

  emitToDevice(deviceId: string, eventName: string, payload: any) {
    this.server.to(deviceId).emit(eventName, payload);
  }

  @SubscribeMessage('device_status')
  handleStatus(
    @MessageBody() data: DeviceState,
    @ConnectedSocket() client: Socket,
  ) {
    console.log('📡 Device status:', data);
    // You could also broadcast it to other users:
    client.broadcast.emit('device_status', data);
  }

  @SubscribeMessage('command')
  handleCommand(
    @MessageBody() data: { command: 'lock' | 'unlock' },
    @ConnectedSocket() client: Socket,
  ) {
    console.log('📡 Command received:', data);
    // Send it back to ESP or all clients
    client.broadcast.emit('command', data);
  }

  @SubscribeMessage('heartbeat')
  handleHeartbeat(
    @MessageBody() data: { deviceName: string; deviceId: string },
    @ConnectedSocket() client: Socket,
  ) {
    const { deviceName, deviceId } = data;
    console.log(`❤️ heartbeat from ${deviceName}`);

    // Emit to all clients that the device is online
    this.server.emit('device_status', {
      deviceId,
      deviceName,
      online: true,
    });

    // Clear previous timer if exists
    const prevTimer = this.heartbeatTimers.get(deviceName);
    if (prevTimer) clearTimeout(prevTimer);

    // Set new timer
    const timeout = setTimeout(() => {
      console.log(`❌ ${deviceName} is now OFFLINE`);
      this.server.emit('device_status', {
        deviceId,
        deviceName,
        online: false,
      });
      this.heartbeatTimers.delete(deviceName);
    }, this.heartbeatInterval);

    this.heartbeatTimers.set(deviceName, timeout);
  }

  @SubscribeMessage('register_device')
  async handleRegisterDevice(
    @MessageBody() data: { deviceName: string },
    @ConnectedSocket() client: Socket,
  ) {
    const deviceId = await this.deviceListService.create(data.deviceName);
    console.log(`📦 Registering device: ${data.deviceName} → ID: ${deviceId}`);

    await client.join(deviceId);

    client.emit('register_device', {
      deviceId,
      deviceName: data.deviceName,
    });
  }
}
