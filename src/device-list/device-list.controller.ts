import {
  Controller,
  Get,
  Post,
  Body,
  Patch,
  Param,
  Delete,
} from '@nestjs/common';
import { DeviceListService } from './device-list.service';

@Controller('device-list')
export class DeviceListController {
  constructor(private readonly deviceListService: DeviceListService) {}

  @Post()
  async create(@Body() body: { name: string; userEmail: string }) {
    const { name, userEmail } = body;
    const newDevice = await this.deviceListService.create(name, userEmail);

    return { deviceId: newDevice };
  }

  @Get()
  findAll() {
    return this.deviceListService.findAll();
  }

  @Get(':id')
  findOne(@Param('id') id: string) {
    return this.deviceListService.findOne(id);
  }

  @Patch(':id')
  update(@Param('id') id: string, @Body() body: { newName: string }) {
    const { newName } = body;
    return this.deviceListService.update(id, newName);
  }

  @Delete(':id')
  remove(@Param('id') id: string, @Body() body: { userEmail: string }) {
    const { userEmail } = body;
    return this.deviceListService.remove(id, userEmail);
  }
}
