import { Module } from '@nestjs/common';
import { DeviceService } from './device.service';
import { DeviceController } from './device.controller';
import { PrismaService } from '../prisma.service';

@Module({
  controllers: [DeviceController],
  providers: [DeviceService, PrismaService],
  exports: [DeviceService],
})
export class DeviceModule {}
