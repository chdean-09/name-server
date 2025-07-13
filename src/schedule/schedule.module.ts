import { Module } from '@nestjs/common';
import { ScheduleService } from './schedule.service';
import { ScheduleController } from './schedule.controller';
import { PrismaModule } from 'src/prisma.module';
import { DeviceModule } from 'src/device/device.module';

@Module({
  imports: [PrismaModule, DeviceModule],
  controllers: [ScheduleController],
  providers: [ScheduleService],
})
export class ScheduleModule {}
