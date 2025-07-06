import { Test, TestingModule } from '@nestjs/testing';
import { DeviceListService } from './device-list.service';

describe('DeviceListService', () => {
  let service: DeviceListService;

  beforeEach(async () => {
    const module: TestingModule = await Test.createTestingModule({
      providers: [DeviceListService],
    }).compile();

    service = module.get<DeviceListService>(DeviceListService);
  });

  it('should be defined', () => {
    expect(service).toBeDefined();
  });
});
