import { Module } from '@nestjs/common';
import { DeviceListService } from './device-list.service';
import { DeviceListController } from './device-list.controller';
import { PrismaModule } from 'src/prisma.module';
import { DeviceGateway } from 'src/device/device.gateway';

@Module({
  imports: [PrismaModule, DeviceGateway],
  controllers: [DeviceListController],
  providers: [DeviceListService],
})
export class DeviceListModule {}
