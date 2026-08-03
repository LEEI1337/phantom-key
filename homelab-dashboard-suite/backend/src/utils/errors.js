/**
 * Error Codes and Error Handling Utilities
 * 
 * Einheitliches Error-Code-System für das gesamte Projekt
 */

export const ErrorCode = {
  // General Errors
  UNKNOWN: 'ERR_UNKNOWN',
  INVALID_CONFIG: 'ERR_INVALID_CONFIG',
  INIT_FAILED: 'ERR_INIT_FAILED',
  
  // Connection Errors
  CONNECTION_FAILED: 'ERR_CONNECTION_FAILED',
  CONNECTION_TIMEOUT: 'ERR_CONNECTION_TIMEOUT',
  CONNECTION_LOST: 'ERR_CONNECTION_LOST',
  
  // Authentication Errors
  AUTH_FAILED: 'ERR_AUTH_FAILED',
  AUTH_EXPIRED: 'ERR_AUTH_EXPIRED',
  INVALID_CREDENTIALS: 'ERR_INVALID_CREDENTIALS',
  
  // Source-specific Errors
  SOURCE_UNAVAILABLE: 'ERR_SOURCE_UNAVAILABLE',
  SOURCE_NOT_FOUND: 'ERR_SOURCE_NOT_FOUND',
  SOURCE_TIMEOUT: 'ERR_SOURCE_TIMEOUT',
  
  // Data Errors
  DATA_INVALID: 'ERR_DATA_INVALID',
  DATA_MISSING: 'ERR_DATA_MISSING',
  DATA_PARSE_ERROR: 'ERR_DATA_PARSE_ERROR',
  
  // Storage Errors
  STORAGE_FULL: 'ERR_STORAGE_FULL',
  STORAGE_WRITE_FAILED: 'ERR_STORAGE_WRITE_FAILED',
  STORAGE_READ_FAILED: 'ERR_STORAGE_READ_FAILED',
  
  // Aggregation Errors
  AGGREGATION_FAILED: 'ERR_AGGREGATION_FAILED',
  PARTIAL_DATA: 'ERR_PARTIAL_DATA',
  
  // Network Errors
  NETWORK_UNREACHABLE: 'ERR_NETWORK_UNREACHABLE',
  DNS_RESOLUTION_FAILED: 'ERR_DNS_RESOLUTION_FAILED',
  SSL_CERTIFICATE_ERROR: 'ERR_SSL_CERTIFICATE_ERROR',
  
  // Rate Limiting
  RATE_LIMIT_EXCEEDED: 'ERR_RATE_LIMIT_EXCEEDED',
  TOO_MANY_REQUESTS: 'ERR_TOO_MANY_REQUESTS',
  
  // Validation Errors
  VALIDATION_FAILED: 'ERR_VALIDATION_FAILED',
  MISSING_PARAMETER: 'ERR_MISSING_PARAMETER',
  INVALID_PARAMETER: 'ERR_INVALID_PARAMETER',
};

/**
 * Custom Application Error Class
 */
export class AppError extends Error {
  constructor(code, message, options = {}) {
    super(message);
    this.name = 'AppError';
    this.code = code;
    this.status = options.status || 500;
    this.source = options.source || null;
    this.timestamp = Date.now();
    this.details = options.details || null;
    
    if (options.cause) {
      this.cause = options.cause;
    }
    
    Error.captureStackTrace(this, this.constructor);
  }
  
  toJSON() {
    return {
      name: this.name,
      code: this.code,
      message: this.message,
      status: this.status,
      source: this.source,
      timestamp: this.timestamp,
      details: this.details,
    };
  }
}

/**
 * Create specific error types
 */
export const createError = {
  connectionFailed: (source, details) => 
    new AppError(ErrorCode.CONNECTION_FAILED, `Connection to ${source} failed`, {
      status: 503,
      source,
      details,
    }),
  
  authFailed: (source, details) => 
    new AppError(ErrorCode.AUTH_FAILED, `Authentication failed for ${source}`, {
      status: 401,
      source,
      details,
    }),
  
  sourceUnavailable: (source, details) => 
    new AppError(ErrorCode.SOURCE_UNAVAILABLE, `Source ${source} is unavailable`, {
      status: 503,
      source,
      details,
    }),
  
  dataParseError: (source, details) => 
    new AppError(ErrorCode.DATA_PARSE_ERROR, `Failed to parse data from ${source}`, {
      status: 502,
      source,
      details,
    }),
  
  rateLimitExceeded: (source, details) => 
    new AppError(ErrorCode.RATE_LIMIT_EXCEEDED, `Rate limit exceeded for ${source}`, {
      status: 429,
      source,
      details,
    }),
  
  storageWriteFailed: (details) => 
    new AppError(ErrorCode.STORAGE_WRITE_FAILED, 'Failed to write to storage', {
      status: 500,
      details,
    }),
  
  validationFailed: (field, message) => 
    new AppError(ErrorCode.VALIDATION_FAILED, `Validation failed for ${field}: ${message}`, {
      status: 400,
      details: { field },
    }),
};

/**
 * Retry utility with exponential backoff
 */
export async function retryWithBackoff(fn, options = {}) {
  const {
    maxRetries = 3,
    baseDelay = 1000,
    maxDelay = 30000,
    factor = 2,
    onRetry = null,
  } = options;
  
  let lastError;
  let delay = baseDelay;
  
  for (let attempt = 1; attempt <= maxRetries + 1; attempt++) {
    try {
      return await fn();
    } catch (err) {
      lastError = err;
      
      if (attempt > maxRetries) {
        break;
      }
      
      if (onRetry) {
        onRetry(err, attempt);
      }
      
      // Add jitter
      const jitter = Math.random() * 0.1 * delay;
      await new Promise(resolve => setTimeout(resolve, delay + jitter));
      
      // Exponential backoff
      delay = Math.min(delay * factor, maxDelay);
    }
  }
  
  throw lastError;
}

/**
 * Check if error is retryable
 */
export function isRetryableError(error) {
  const retryableCodes = [
    ErrorCode.CONNECTION_FAILED,
    ErrorCode.CONNECTION_TIMEOUT,
    ErrorCode.CONNECTION_LOST,
    ErrorCode.SOURCE_UNAVAILABLE,
    ErrorCode.NETWORK_UNREACHABLE,
    ErrorCode.RATE_LIMIT_EXCEEDED,
  ];
  
  return retryableCodes.includes(error?.code);
}
