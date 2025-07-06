import { Module } from '@nestjs/common';
import { DeviceGateway } from './device.gateway';
import { DeviceListService } from 'src/device-list/device-list.service';
import { PrismaService } from 'src/prisma.service';

@Module({
  providers: [DeviceGateway, DeviceListService, PrismaService],
  exports: [DeviceGateway],
})
export class DeviceModule {}
