import { Module } from '@nestjs/common';
import { DeviceGateway } from './device.gateway';

@Module({
  providers: [DeviceGateway],
})
export class DeviceModule {}
