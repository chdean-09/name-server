import { Module } from '@nestjs/common';
import { DeviceGateway } from './device.gateway';
import { DeviceListModule } from 'src/device-list/device-list.module';

@Module({
  imports: [DeviceListModule],
  providers: [DeviceGateway],
  exports: [DeviceGateway],
})
export class DeviceModule {}
