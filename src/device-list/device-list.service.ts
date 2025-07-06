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

  async create(name: string) {
    const newDevice = await this.prisma.device.create({
      data: {
        name,
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

  // findOne(id: number) {
  //   return `This action returns a #${id} deviceList`;
  // }

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

  async remove(id: string) {
    await this.prisma.device.delete({
      where: {
        id: id,
      },
    });

    this.eventEmitter.emit('device.removed', { deviceId: id });
  }
}
