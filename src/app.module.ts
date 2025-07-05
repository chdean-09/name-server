import { Module } from '@nestjs/common';
import { AppController } from './app.controller';
import { AppService } from './app.service';
import { DeviceGateway } from './device/device.gateway';
import { DeviceModule } from './device/device.module';

@Module({
  imports: [DeviceModule],
  controllers: [AppController],
  providers: [AppService, DeviceGateway],
})
export class AppModule {}
