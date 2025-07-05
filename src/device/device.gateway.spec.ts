import { Test, TestingModule } from '@nestjs/testing';
import { DeviceGateway } from './device.gateway';

describe('DeviceGateway', () => {
  let gateway: DeviceGateway;

  beforeEach(async () => {
    const module: TestingModule = await Test.createTestingModule({
      providers: [DeviceGateway],
    }).compile();

    gateway = module.get<DeviceGateway>(DeviceGateway);
  });

  it('should be defined', () => {
    expect(gateway).toBeDefined();
  });
});
