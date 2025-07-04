/* eslint-disable @typescript-eslint/no-unused-vars */
/* eslint-disable @typescript-eslint/no-unsafe-argument */
/* eslint-disable @typescript-eslint/no-unsafe-return */
/* eslint-disable @typescript-eslint/no-unsafe-assignment */
/* eslint-disable @typescript-eslint/no-unsafe-call */
/* eslint-disable @typescript-eslint/no-unsafe-member-access */
import {
  Injectable,
  NotFoundException,
  BadRequestException,
} from '@nestjs/common';
import { PrismaService } from '../prisma.service';

@Injectable()
export class DeviceService {
  constructor(private prisma: PrismaService) {}

  async initiatePairing(
    macAddress: string,
    deviceName: string,
  ): Promise<{ pairingCode: string }> {
    const pairingCode = Math.random().toString(36).substr(2, 8).toUpperCase();

    await this.prisma.device.upsert({
      where: { macAddress },
      update: {
        pairingCode,
        name: deviceName,
        updatedAt: new Date(),
      },
      create: {
        macAddress,
        name: deviceName,
        pairingCode,
        apiKey: this.generateApiKey(),
      },
    });

    return { pairingCode };
  }

  async completePairing(
    pairingCode: string,
    userId: string,
    wifiCredentials: { ssid: string; password: string },
  ) {
    const device = await this.prisma.device.findFirst({
      where: { pairingCode },
    });

    if (!device) {
      throw new NotFoundException('Invalid pairing code');
    }

    if (device.isClaimed) {
      throw new BadRequestException('Device already claimed');
    }

    const updatedDevice = await this.prisma.device.update({
      where: { id: device.id },
      data: {
        ownerId: userId,
        isClaimed: true,
        wifiSSID: wifiCredentials.ssid,
        pairingCode: null,
      },
    });

    this.sendWifiCredentialsToDevice(device.macAddress, wifiCredentials);

    return updatedDevice;
  }

  async sendCommandToDevice(
    deviceId: string,
    userId: string,
    command: 'LOCK' | 'UNLOCK',
    doorId: number,
  ): Promise<void> {
    const device = await this.prisma.device.findFirst({
      where: {
        id: deviceId,
        ownerId: userId,
        isClaimed: true,
      },
    });

    if (!device) {
      throw new NotFoundException('Device not found or not owned by user');
    }

    if (!device.isOnline || !device.ipAddress) {
      throw new BadRequestException('Device is offline');
    }

    const commandRecord = await this.prisma.command.create({
      data: {
        deviceId,
        userId,
        type: command,
        doorId,
        status: 'PENDING',
      },
    });

    try {
      const action = command === 'LOCK' ? 'on' : 'off';
      const fetch = (await import('node-fetch')).default;

      const response = await fetch(
        `http://${device.ipAddress}/door?door=${doorId}&state=${action}`,
        {
          method: 'GET',
          headers: {
            Authorization: `Bearer ${device.apiKey}`,
          },
        },
      );

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

      const responseText = await response.text();

      await this.prisma.command.update({
        where: { id: commandRecord.id },
        data: {
          status: 'EXECUTED',
          executedAt: new Date(),
          response: responseText,
        },
      });
    } catch (error) {
      await this.prisma.command.update({
        where: { id: commandRecord.id },
        data: {
          status: 'FAILED',
          response: error.message,
        },
      });
      throw new BadRequestException('Failed to send command to device');
    }
  }

  async updateDeviceStatus(
    macAddress: string,
    ipAddress: string,
  ): Promise<void> {
    await this.prisma.device.updateMany({
      where: { macAddress },
      data: {
        ipAddress,
        isOnline: true,
        lastSeen: new Date(),
      },
    });
  }

  async getDeviceStatus(macAddress: string): Promise<any> {
    const device = await this.prisma.device.findUnique({
      where: { macAddress },
      include: {
        owner: {
          select: { id: true, email: true },
        },
      },
    });

    if (!device) {
      throw new NotFoundException('Device not found');
    }

    if (!device.isOnline || !device.ipAddress) {
      return {
        device,
        sensors: null,
        message: 'Device offline',
      };
    }

    try {
      const fetch = (await import('node-fetch')).default;

      const [sensor1, sensor2, sensor3, buzzer] = await Promise.all([
        fetch(`http://${device.ipAddress}/door_sensor1`),
        fetch(`http://${device.ipAddress}/door_sensor2`),
        fetch(`http://${device.ipAddress}/door_sensor3`),
        fetch(`http://${device.ipAddress}/buzzer`),
      ]);

      return {
        device,
        sensors: {
          door1: parseInt(await sensor1.text()),
          door2: parseInt(await sensor2.text()),
          door3: parseInt(await sensor3.text()),
          buzzer: parseInt(await buzzer.text()),
        },
      };
    } catch (error) {
      return {
        device,
        sensors: null,
        message: 'Failed to fetch sensor data',
      };
    }
  }

  getUserDevices(userId: string) {
    return this.prisma.device.findMany({
      where: { ownerId: userId },
      include: {
        Command: {
          orderBy: { sentAt: 'desc' },
          take: 10,
        },
      },
    });
  }

  private generateApiKey(): string {
    return Math.random().toString(36).substr(2, 32);
  }

  private sendWifiCredentialsToDevice(
    macAddress: string,
    credentials: { ssid: string; password: string },
  ): void {
    console.log(
      `Sending WiFi credentials to device ${macAddress}:`,
      credentials,
    );
  }
}
