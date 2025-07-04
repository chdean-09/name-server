import { Module } from '@nestjs/common';
import { AppController } from './app.controller';
import { AppService } from './app.service';
import { DeviceModule } from './device/device.module';
import { PrismaModule } from './prisma.module';
import { DeviceGateway } from './websocket/device.gateway';

@Module({
  imports: [PrismaModule, DeviceModule],
  controllers: [AppController],
  providers: [AppService, DeviceGateway],
})
export class AppModule {}
