/* eslint-disable @typescript-eslint/no-unsafe-return */
/* eslint-disable @typescript-eslint/no-unsafe-argument */
/* eslint-disable @typescript-eslint/no-unsafe-assignment */
/* eslint-disable @typescript-eslint/no-unsafe-member-access */
/* eslint-disable @typescript-eslint/no-unsafe-call */
import {
  Injectable,
  NotFoundException,
  BadRequestException,
} from '@nestjs/common';
import { PrismaService } from '../prisma.service';
import { Device, CommandType, CommandStatus } from '@prisma/client';
import axios from 'axios';

@Injectable()
export class DeviceService {
  constructor(private prisma: PrismaService) {}

  async initiatePairing(
    macAddress: string,
    deviceName: string,
  ): Promise<{ pairingCode: string }> {
    // Generate a unique pairing code
    const pairingCode = Math.random().toString(36).substr(2, 8).toUpperCase();

    // Create or update device with pairing code
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
  ): Promise<Device> {
    const device = await this.prisma.device.findFirst({
      where: { pairingCode },
    });

    if (!device) {
      throw new NotFoundException('Invalid pairing code');
    }

    if (device.isClaimed) {
      throw new BadRequestException('Device already claimed');
    }

    // Update device with owner and WiFi credentials
    const updatedDevice = await this.prisma.device.update({
      where: { id: device.id },
      data: {
        ownerId: userId,
        isClaimed: true,
        wifiSSID: wifiCredentials.ssid,
        pairingCode: null, // Remove pairing code after successful pairing
      },
    });

    // Send WiFi credentials to ESP32 via Bluetooth
    await this.sendWifiCredentialsToDevice(device.macAddress, wifiCredentials);

    return updatedDevice;
  }

  async sendCommandToDevice(
    deviceId: string,
    userId: string,
    command: CommandType,
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

    // Create command record
    const commandRecord = await this.prisma.command.create({
      data: {
        deviceId,
        userId,
        type: command,
        doorId,
        status: CommandStatus.PENDING,
      },
    });

    try {
      // Send command to ESP32
      const action = command === CommandType.LOCK ? 'on' : 'off';
      const response = await axios.get(
        `http://${device.ipAddress}/door?door=${doorId}&state=${action}`,
        {
          timeout: 5000,
          headers: {
            Authorization: `Bearer ${device.apiKey}`,
          },
        },
      );

      // Update command status
      await this.prisma.command.update({
        where: { id: commandRecord.id },
        data: {
          status: CommandStatus.EXECUTED,
          executedAt: new Date(),
          response: response.data,
        },
      });
    } catch (error) {
      // Update command status as failed
      await this.prisma.command.update({
        where: { id: commandRecord.id },
        data: {
          status: CommandStatus.FAILED,
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
      // Get sensor data from ESP32
      const [sensor1, sensor2, sensor3, buzzer] = await Promise.all([
        axios.get(`http://${device.ipAddress}/door_sensor1`, { timeout: 3000 }),
        axios.get(`http://${device.ipAddress}/door_sensor2`, { timeout: 3000 }),
        axios.get(`http://${device.ipAddress}/door_sensor3`, { timeout: 3000 }),
        axios.get(`http://${device.ipAddress}/buzzer`, { timeout: 3000 }),
      ]);

      return {
        device,
        sensors: {
          door1: parseInt(sensor1.data),
          door2: parseInt(sensor2.data),
          door3: parseInt(sensor3.data),
          buzzer: parseInt(buzzer.data),
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

  async getUserDevices(userId: string): Promise<Device[]> {
    return await this.prisma.device.findMany({
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
    // This would normally send BLE commands to the ESP32
    // For now, we'll simulate this - in a real implementation,
    // you'd use a BLE library to send the credentials
    console.log(
      `Sending WiFi credentials to device ${macAddress}:`,
      credentials,
    );
  }
}
