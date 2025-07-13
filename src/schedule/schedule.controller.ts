import { Controller, Post, Body, Param, Delete, Patch } from '@nestjs/common';
import { ScheduleService } from './schedule.service';
import { Schedule } from '@prisma/client';

@Controller('schedule')
export class ScheduleController {
  constructor(private readonly scheduleService: ScheduleService) {}

  @Post()
  create(@Body() body: { deviceId: string; schedule: Schedule }) {
    const { deviceId, schedule } = body;
    return this.scheduleService.create(deviceId, schedule);
  }

  // @Get()
  // findAll() {
  //   return this.scheduleService.findAll();
  // }

  // @Get(':id')
  // findOne(@Param('id') id: string) {
  //   return this.scheduleService.findOne(id);
  // }

  @Patch(':id')
  update(@Param('id') id: string, @Body() body: { isEnabled: boolean }) {
    return this.scheduleService.update(id, body.isEnabled);
  }

  @Delete(':id')
  remove(@Param('id') id: string) {
    return this.scheduleService.remove(id);
  }
}
