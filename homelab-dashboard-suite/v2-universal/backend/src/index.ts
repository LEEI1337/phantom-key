import express, { Application, Request, Response, NextFunction } from 'express';
import cors from 'cors';
import helmet from 'helmet';
import rateLimit from 'express-rate-limit';
import { createServer } from 'http';
import { WebSocketServer } from 'ws';
import { config } from './core/config';
import { logger } from './utils/logger';
import { errorHandler } from './middleware/errorHandler';
import { authRoutes } from './api/routes/auth.routes';
import { dataSourceRoutes } from './api/routes/datasource.routes';
import { layoutRoutes } from './api/routes/layout.routes';
import { metricsRoutes } from './api/routes/metrics.routes';
import { healthCheck } from './api/middleware/healthCheck';
import { RedisClient } from './db/redisClient';
import { DatabaseClient } from './db/databaseClient';
import { CollectorManager } from './core/collectorManager';
import { WebSocketHandler } from './core/websocketHandler';

class App {
  private app: Application;
  private httpServer: any;
  private wsServer: WebSocketServer;
  private redisClient: RedisClient;
  private dbClient: DatabaseClient;
  private collectorManager: CollectorManager;
  private wsHandler: WebSocketHandler;

  constructor() {
    this.app = express();
    this.httpServer = createServer(this.app);
    this.wsServer = new WebSocketServer({ server: this.httpServer, path: '/ws' });
    
    this.redisClient = new RedisClient();
    this.dbClient = new DatabaseClient();
    this.collectorManager = new CollectorManager();
    this.wsHandler = new WebSocketHandler(this.wsServer);
    
    this.initializeMiddleware();
    this.initializeRoutes();
    this.initializeErrorHandling();
    this.startServices();
  }

  private initializeMiddleware(): void {
    // Security
    this.app.use(helmet());
    this.app.use(cors({
      origin: process.env.CORS_ORIGIN || '*',
      methods: ['GET', 'POST', 'PUT', 'DELETE'],
      allowedHeaders: ['Content-Type', 'Authorization']
    }));

    // Rate limiting
    const limiter = rateLimit({
      windowMs: parseInt(process.env.RATE_LIMIT_WINDOW_MS || '900000'),
      max: parseInt(process.env.RATE_LIMIT_MAX_REQUESTS || '100'),
      message: 'Too many requests from this IP, please try again later.'
    });
    this.app.use('/api/', limiter);

    // Body parsing
    this.app.use(express.json({ limit: '10mb' }));
    this.app.use(express.urlencoded({ extended: true }));

    // Logging
    this.app.use((req: Request, res: Response, next: NextFunction) => {
      logger.info(`${req.method} ${req.path}`, { ip: req.ip });
      next();
    });
  }

  private initializeRoutes(): void {
    // Health check
    this.app.get('/health', healthCheck);

    // API Routes
    this.app.use('/api/auth', authRoutes);
    this.app.use('/api/sources', dataSourceRoutes);
    this.app.use('/api/layouts', layoutRoutes);
    this.app.use('/api/metrics', metricsRoutes);

    // Root
    this.app.get('/', (req: Request, res: Response) => {
      res.json({
        name: 'Homelab Dashboard Suite',
        version: '2.0.0',
        status: 'running',
        endpoints: {
          health: '/health',
          api: '/api',
          websocket: 'ws://localhost:3000/ws'
        }
      });
    });

    // 404 handler
    this.app.use((req: Request, res: Response) => {
      res.status(404).json({ error: 'Not Found', path: req.path });
    });
  }

  private initializeErrorHandling(): void {
    this.app.use(errorHandler);
  }

  private async startServices(): Promise<void> {
    try {
      // Initialize database connections
      await this.redisClient.connect();
      await this.dbClient.connect();

      // Start collector manager
      await this.collectorManager.start();

      // Connect WebSocket handler to services
      this.wsHandler.setRedisClient(this.redisClient);
      this.wsHandler.setCollectorManager(this.collectorManager);

      // Start HTTP server
      const port = config.port;
      this.httpServer.listen(port, () => {
        logger.info(`Backend server running on port ${port}`);
        logger.info(`WebSocket server available at ws://localhost:${port}/ws`);
      });

      // Graceful shutdown
      process.on('SIGTERM', () => this.gracefulShutdown());
      process.on('SIGINT', () => this.gracefulShutdown());

    } catch (error) {
      logger.error('Failed to start services', error);
      process.exit(1);
    }
  }

  private async gracefulShutdown(): Promise<void> {
    logger.info('Shutting down gracefully...');
    
    try {
      await this.collectorManager.stop();
      await this.redisClient.disconnect();
      await this.dbClient.disconnect();
      
      this.httpServer.close(() => {
        logger.info('HTTP server closed');
        process.exit(0);
      });
    } catch (error) {
      logger.error('Error during shutdown', error);
      process.exit(1);
    }
  }
}

// Start the application
const app = new App();

export default app;
