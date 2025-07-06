/* eslint-disable @typescript-eslint/no-unsafe-call */
/* eslint-disable @typescript-eslint/no-unsafe-member-access */
/* eslint-disable @typescript-eslint/no-unsafe-assignment */
import { Injectable } from '@nestjs/common';
import { Schedule } from '@prisma/client';
import { PrismaService } from 'src/prisma.service';

@Injectable()
export class ScheduleService {
  constructor(private prisma: PrismaService) {}

  async create(deviceId: string, schedule: Schedule) {
    await this.prisma.schedule.create({
      data: {
        lockDay: schedule.lockDay,
        lockTime: schedule.lockTime,
        unlockDay: schedule.unlockDay,
        unlockTime: schedule.unlockTime,
        deviceId: deviceId,
      },
    });
  }

  // findAll() {
  //   return `This action returns all schedule`;
  // }

  // findOne(id: number) {
  //   return `This action returns a #${id} schedule`;
  // }

  // update(id: number, updateScheduleDto: UpdateScheduleDto) {
  //   return `This action updates a #${id} schedule`;
  // }

  async remove(id: string) {
    await this.prisma.schedule.delete({
      where: {
        id: id,
      },
    });
  }
}
