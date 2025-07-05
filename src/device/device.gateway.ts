import {
  ConnectedSocket,
  MessageBody,
  SubscribeMessage,
  WebSocketGateway,
} from '@nestjs/websockets';
import { Socket } from 'socket.io';

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
    client.broadcast.emit('message', data);
  }
}
