/* eslint-disable @typescript-eslint/no-unsafe-assignment */

import { Injectable } from '@nestjs/common';
import { Schedule } from '@prisma/client';
import { PrismaService } from 'src/prisma.service';
import { Cron, CronExpression } from '@nestjs/schedule';
import { DeviceGateway } from 'src/device/device.gateway';

@Injectable()
export class ScheduleService {
  constructor(
    private prisma: PrismaService,
    private deviceGateway: DeviceGateway,
  ) {}

  @Cron(CronExpression.EVERY_MINUTE)
  async handleCron() {
    const now = new Date();
    const hours = now.getHours().toString().padStart(2, '0');
    const minutes = now.getMinutes().toString().padStart(2, '0');
    const currentTime = `${hours}:${minutes}`;

    // Get current day as "Mon", "Tue", etc.
    const daysOfWeek = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];
    const today = daysOfWeek[now.getDay()];

    // Find all enabled schedules for this time and day
    const schedules = await this.prisma.schedule.findMany({
      where: {
        isEnabled: true,
        time: currentTime,
        days: { has: today },
      },
      include: { device: true },
    });

    for (const schedule of schedules) {
      const command = schedule.type === 'LOCK' ? 'lock' : 'unlock';
      // Emit to device via gateway
      this.deviceGateway.emitToDevice(
        schedule.device.userEmail,
        schedule.deviceId,
        'command',
        { command: command },
      );
      console.log(
        `⏰ Sent scheduled ${command} to device ${schedule.deviceId} at ${currentTime} (${today})`,
      );
    }
  }

  async create(deviceId: string, schedule: Schedule) {
    await this.prisma.schedule.create({
      data: {
        days: schedule.days,
        time: schedule.time,
        type: schedule.type,
        deviceId: deviceId,
      },
    });
  }

  // findAll() {
  //   return `This action returns all schedule`;
  // }

  // findOne(id: string) {
  //   return `This action returns a #${id} schedule`;
  // }

  async update(id: string, isEnabled: boolean) {
    await this.prisma.schedule.update({
      where: { id: id },
      data: { isEnabled: isEnabled },
    });
  }

  async remove(id: string) {
    await this.prisma.schedule.delete({
      where: {
        id: id,
      },
    });
  }
}
