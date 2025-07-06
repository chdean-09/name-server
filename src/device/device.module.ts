import { Module } from '@nestjs/common';
import { DeviceGateway } from './device.gateway';
import { DeviceListService } from 'src/device-list/device-list.service';
import { PrismaService } from 'src/prisma.service';
import { DeviceListModule } from 'src/device-list/device-list.module';

@Module({
  imports: [DeviceListModule],
  providers: [DeviceGateway, DeviceListService, PrismaService],
  exports: [DeviceGateway],
})
export class DeviceModule {}
