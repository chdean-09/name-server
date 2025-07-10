/* eslint-disable @typescript-eslint/no-unused-vars */
import {
  ConnectedSocket,
  MessageBody,
  SubscribeMessage,
  WebSocketGateway,
  WebSocketServer,
} from '@nestjs/websockets';
import { OnEvent } from '@nestjs/event-emitter';
import { Server, Socket } from 'socket.io';
import { DeviceListService } from 'src/device-list/device-list.service';

interface Heartbeat {
  userEmail: string;
  deviceId: string;
  deviceName: string;
  lock: 'locked' | 'unlocked';
  sensor: 'open' | 'closed';
  buzzer: 'on' | 'off';
}

@WebSocketGateway({
  cors: {
    origin: '*',
  },
})
export class DeviceGateway {
  constructor(private readonly deviceListService: DeviceListService) {}
  @WebSocketServer()
  server: Server;

  private heartbeatTimers = new Map<string, NodeJS.Timeout>();

  handleConnection(client: Socket) {
    console.log('🔗 Client connected:', client.id);
  }

  handleDisconnect(client: Socket) {
    console.log('🔌 Client disconnected:', client.id);

    // Clean up heartbeat timers for disconnected devices
    for (const [deviceName, timer] of this.heartbeatTimers.entries()) {
      if (client.rooms.has(deviceName)) {
        clearTimeout(timer);
        this.heartbeatTimers.delete(deviceName);
        console.log(`🧹 Cleaned up heartbeat timer for ${deviceName}`);
        break;
      }
    }
  }

  // emit functions to mobile and device clients
  emitToMobile(
    userEmail: string,
    deviceId: string,
    eventName: string,
    payload: any,
  ) {
    this.server.to(`${userEmail}-device-${deviceId}`).emit(eventName, payload);
  }

  emitToDevice(
    userEmail: string,
    deviceId: string,
    eventName: string,
    payload: any,
  ) {
    this.server.to(`${userEmail}-device-${deviceId}`).emit(eventName, payload);
  }

  // join rooms for mobile and device clients
  @SubscribeMessage('join_as_mobile')
  async handleJoinAsMobile(
    @MessageBody() data: { userEmail: string; deviceId: string },
    @ConnectedSocket() client: Socket,
  ) {
    const { userEmail, deviceId } = data;

    await client.join(`${userEmail}-device-${deviceId}`);

    console.log(
      `🔧 Mobile joined device room: ${userEmail}-device-${deviceId}`,
    );

    this.emitToDevice(userEmail, deviceId, 'request_status', {});
  }

  @SubscribeMessage('join_as_device')
  async handleJoinAsDevice(
    @MessageBody() data: { deviceId: string; userEmail: string },
    @ConnectedSocket() client: Socket,
  ) {
    const { deviceId, userEmail } = data;
    await client.join(`${userEmail}-device-${deviceId}`);

    console.log(`🔧 Device joined room: ${userEmail}-device-${deviceId}`);
  }

  // register and unpair logic
  @SubscribeMessage('register_device')
  async handleRegisterDevice(
    @MessageBody() data: { deviceName: string; userEmail: string },
    @ConnectedSocket() client: Socket,
  ) {
    try {
      const deviceId = await this.deviceListService.create(data.deviceName);
      console.log('Data reveived from device:', data);

      // Join device rooms
      await client.join(`${data.userEmail}-device-${deviceId}`);

      console.log(
        `Device has joined room ${data.userEmail}-device-${deviceId}`,
      );

      // Respond to the device that registered
      client.emit('register_device', {
        deviceId,
        deviceName: data.deviceName,
        success: true,
      });
    } catch (error) {
      console.error('Failed to register device:', error);
      client.emit('register_device', {
        success: false,
        error: 'Failed to register device',
      });
    }
  }

  @OnEvent('device_removed')
  handleDeviceRemoved(payload: { userEmail: string; deviceId: string }) {
    console.log(
      `📡 Device ${payload.deviceId} was removed, sending unpair signal`,
    );
    this.emitToDevice(payload.userEmail, payload.deviceId, 'unpair_device', {});
  }

  // command received from the mobile app
  // and will be sent to the esp32 client
  @SubscribeMessage('command')
  handleCommand(
    @MessageBody()
    data: {
      userEmail: string;
      command: 'lock' | 'unlock';
      deviceId: string;
    },
    @ConnectedSocket() client: Socket,
  ) {
    console.log('📡 Command received:', data);

    this.emitToDevice(data.userEmail, data.deviceId, 'command', {
      command: data.command,
    });
  }

  // heartbeat received from the esp32 client
  // and will be sent to mobile client
  @SubscribeMessage('heartbeat')
  handleHeartbeat(
    @MessageBody() data: Heartbeat,
    @ConnectedSocket() client: Socket,
  ) {
    const { deviceName, deviceId, userEmail } = data;
    console.log(`❤️ heartbeat from ${deviceName}`);

    // Emit to mobile clients only that the device is online
    this.emitToMobile(userEmail, deviceId, 'heartbeat', {
      deviceId,
      deviceName,
      online: true,
      lock: data.lock,
      sensor: data.sensor,
      buzzer: data.buzzer,
    });

    // Clear previous timer if exists
    const prevTimer = this.heartbeatTimers.get(deviceId);
    if (prevTimer) clearTimeout(prevTimer);

    // Set new timer
    const timeout = setTimeout(() => {
      console.log(`❌ ${deviceName} is now OFFLINE`);
      this.emitToMobile(userEmail, deviceId, 'heartbeat', {
        deviceId,
        deviceName,
        online: false,
        lock: data.lock,
        sensor: data.sensor,
        buzzer: data.buzzer,
      });
      this.heartbeatTimers.delete(deviceId);
    }, 3); // 3 seconds

    this.heartbeatTimers.set(deviceId, timeout);
  }
}
