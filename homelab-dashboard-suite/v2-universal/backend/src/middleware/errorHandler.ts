import { Request, Response, NextFunction } from 'express';
import { logger } from '../utils/logger';
import { config } from '../core/config';

export class AppError extends Error {
  statusCode: number;
  isOperational: boolean;

  constructor(message: string, statusCode: number) {
    super(message);
    this.statusCode = statusCode;
    this.isOperational = true;
    Error.captureStackTrace(this, this.constructor);
  }
}

export const errorHandler = (
  err: Error | AppError,
  req: Request,
  res: Response,
  next: NextFunction
) => {
  const statusCode = (err as AppError).statusCode || 500;
  const message = err.message || 'Internal Server Error';

  logger.error(`Error: ${message}`, {
    stack: err.stack,
    path: req.path,
    method: req.method,
    statusCode
  });

  if (statusCode >= 500) {
    logger.error('Critical error occurred', { error: err.stack });
  }

  res.status(statusCode).json({
    success: false,
    error: {
      code: `ERR_${statusCode}`,
      message: config.nodeEnv === 'production' && statusCode === 500 
        ? 'Internal Server Error' 
        : message,
      ...(config.nodeEnv !== 'production' && { stack: err.stack })
    }
  });
};

export const createError = (message: string, code: number): AppError => {
  return new AppError(message, code);
};

export const errors = {
  notFound: (resource?: string) => createError(`${resource || 'Resource'} not found`, 404),
  unauthorized: () => createError('Unauthorized access', 401),
  forbidden: () => createError('Access forbidden', 403),
  badRequest: (message: string) => createError(message, 400),
  conflict: (message: string) => createError(message, 409),
  internal: (message: string) => createError(message, 500),
  serviceUnavailable: () => createError('Service temporarily unavailable', 503)
};
