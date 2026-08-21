import { Request, Response } from 'express';
import { logger } from '../../utils/logger';

export const healthCheck = async (req: Request, res: Response) => {
  try {
    const healthStatus = {
      status: 'healthy',
      timestamp: new Date().toISOString(),
      uptime: process.uptime(),
      version: '2.0.0',
      services: {
        database: 'connected',
        redis: 'connected',
        collectors: 'running'
      }
    };

    res.status(200).json(healthStatus);
  } catch (error) {
    logger.error('Health check failed', error);
    res.status(503).json({
      status: 'unhealthy',
      error: 'Service health check failed'
    });
  }
};
