/* eslint-disable @typescript-eslint/no-unsafe-return */
import {
  Controller,
  Get,
  Post,
  Body,
  Param,
  UseGuards,
  Request,
} from '@nestjs/common';
import { DeviceService } from './device.service';
import { CommandType } from '@prisma/client';

@Controller('devices')
export class DeviceController {
  constructor(private readonly deviceService: DeviceService) {}

  @Post('initiate-pairing')
  async initiatePairing(
    @Body() body: { macAddress: string; deviceName: string },
  ) {
    return this.deviceService.initiatePairing(body.macAddress, body.deviceName);
  }

  @Post('complete-pairing')
  async completePairing(
    @Body()
    body: {
      pairingCode: string;
      userId: string;
      wifiCredentials: { ssid: string; password: string };
    },
  ) {
    return this.deviceService.completePairing(
      body.pairingCode,
      body.userId,
      body.wifiCredentials,
    );
  }

  @Post(':deviceId/command')
  async sendCommand(
    @Param('deviceId') deviceId: string,
    @Body() body: { command: CommandType; doorId: number; userId: string },
  ) {
    await this.deviceService.sendCommandToDevice(
      deviceId,
      body.userId,
      body.command,
      body.doorId,
    );
    return { success: true };
  }

  @Post('status/update')
  async updateDeviceStatus(
    @Body() body: { macAddress: string; ipAddress: string },
  ) {
    await this.deviceService.updateDeviceStatus(
      body.macAddress,
      body.ipAddress,
    );
    return { success: true };
  }

  @Get('status/:macAddress')
  async getDeviceStatus(@Param('macAddress') macAddress: string) {
    return this.deviceService.getDeviceStatus(macAddress);
  }

  @Get('user/:userId')
  async getUserDevices(@Param('userId') userId: string) {
    return this.deviceService.getUserDevices(userId);
  }
}
