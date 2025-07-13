/* eslint-disable @typescript-eslint/no-unsafe-call */
import { Injectable } from '@nestjs/common';
import { EventEmitter2 } from '@nestjs/event-emitter';
import { PrismaService } from 'src/prisma.service';

@Injectable()
export class DeviceListService {
  constructor(
    private prisma: PrismaService,
    private eventEmitter: EventEmitter2,
  ) {}

  async create(name: string, userEmail: string) {
    const newDevice = await this.prisma.device.create({
      data: {
        name,
        userEmail,
      },
    });

    return newDevice.id;
  }

  async findAll() {
    const data = await this.prisma.device.findMany({
      include: {
        schedule: true,
      },
    });
    return data;
  }

  async findOne(id: string) {
    const device = await this.prisma.device.findUnique({
      where: {
        id: id,
      },
      include: {
        schedule: true,
      },
    });

    if (!device) {
      throw new Error('Device not found');
    }

    return device;
  }

  async update(id: string, newName: string) {
    await this.prisma.device.update({
      where: {
        id: id,
      },
      data: {
        name: newName,
      },
    });
  }

  async remove(id: string, userEmail: string) {
    await this.prisma.device.delete({
      where: {
        id: id,
      },
    });

    this.eventEmitter.emit('device_removed', {
      userEmail: userEmail,
      deviceId: id,
    });
  }
}
