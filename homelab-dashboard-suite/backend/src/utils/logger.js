/**
 * Winston Logger Configuration
 * 
 * Zentrale Logging-Konfiguration für das gesamte Backend
 */

import winston from 'winston';

const { combine, timestamp, printf, colorize, errors } = winston.format;

// Custom log format
const logFormat = printf(({ level, message, timestamp, source, code, stack }) => {
  let log = `${timestamp} [${level}]: ${message}`;
  
  if (source) {
    log += ` | Source: ${source}`;
  }
  
  if (code) {
    log += ` | Code: ${code}`;
  }
  
  if (stack) {
    log += `\n${stack}`;
  }
  
  return log;
});

// Create logger instance
export const logger = winston.createLogger({
  level: process.env.LOG_LEVEL || 'info',
  format: combine(
    errors({ stack: true }),
    timestamp({ format: 'YYYY-MM-DD HH:mm:ss' }),
    logFormat
  ),
  defaultMeta: { service: 'homelab-aggregator' },
  transports: [
    // Console output with colors
    new winston.transports.Console({
      format: combine(colorize(), logFormat),
    }),
    
    // File output for errors
    new winston.transports.File({
      filename: 'logs/error.log',
      level: 'error',
      maxsize: 5242880, // 5MB
      maxFiles: 5,
    }),
    
    // File output for all logs
    new winston.transports.File({
      filename: 'logs/combined.log',
      maxsize: 5242880, // 5MB
      maxFiles: 5,
    }),
  ],
});

// Create child logger with source context
export function createSourceLogger(source) {
  return logger.child({ source });
}

// Debug helper (only logs in debug mode)
export function debug(logger, message, ...args) {
  if (process.env.DEBUG_MODE === 'true') {
    logger.debug(message, ...args);
  }
}
