import { NestFactory } from '@nestjs/core';
import { WsAdapter } from '@nestjs/platform-ws';
import { AppModule } from './app.module';

async function bootstrap() {
  const app = await NestFactory.create(AppModule);

  // Use native WebSocket adapter
  app.useWebSocketAdapter(new WsAdapter(app));

  // Enable CORS for HTTP requests
  app.enableCors({
    origin: '*',
    methods: ['GET', 'POST', 'PUT', 'DELETE'],
    allowedHeaders: ['Content-Type', 'Authorization'],
  });

  await app.listen(process.env.PORT ?? 8000);
  console.log(
    `🚀 Server running on http://localhost:${process.env.PORT ?? 8000}`,
  );
  console.log(
    `📡 WebSocket server running on ws://localhost:${process.env.PORT ?? 8000}`,
  );
}
bootstrap();
