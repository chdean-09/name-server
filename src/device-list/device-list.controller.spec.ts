import { Test, TestingModule } from '@nestjs/testing';
import { DeviceListController } from './device-list.controller';
import { DeviceListService } from './device-list.service';

describe('DeviceListController', () => {
  let controller: DeviceListController;

  beforeEach(async () => {
    const module: TestingModule = await Test.createTestingModule({
      controllers: [DeviceListController],
      providers: [DeviceListService],
    }).compile();

    controller = module.get<DeviceListController>(DeviceListController);
  });

  it('should be defined', () => {
    expect(controller).toBeDefined();
  });
});
