import { Module } from '@nestjs/common';
import { AppController } from './app.controller';
import { AppService } from './app.service';
import { DeviceModule } from './device/device.module';
import { DeviceListModule } from './device-list/device-list.module';
import { ScheduleModule } from './schedule/schedule.module';
import { EventEmitterModule } from '@nestjs/event-emitter';

@Module({
  imports: [
    DeviceModule,
    DeviceListModule,
    ScheduleModule,
    EventEmitterModule.forRoot(),
  ],
  controllers: [AppController],
  providers: [AppService],
})
export class AppModule {}
