import { Module } from '@nestjs/common';
import { DeviceListService } from './device-list.service';
import { DeviceListController } from './device-list.controller';

@Module({
  controllers: [DeviceListController],
  providers: [DeviceListService],
})
export class DeviceListModule {}
