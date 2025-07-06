import { Module } from '@nestjs/common';
import { DeviceListService } from './device-list.service';
import { DeviceListController } from './device-list.controller';
import { PrismaModule } from 'src/prisma.module';

@Module({
  imports: [PrismaModule],
  controllers: [DeviceListController],
  providers: [DeviceListService],
})
export class DeviceListModule {}
