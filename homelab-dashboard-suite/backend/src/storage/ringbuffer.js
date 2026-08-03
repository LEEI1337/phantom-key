/**
 * Ring Buffer Storage for Time-Series Data
 * 
 * Speichert die letzten 24-48 Stunden an Daten in einem zirkulären Puffer.
 * Optimiert für ESP32/CYD mit begrenztem Speicher.
 */

import { createSourceLogger } from '../utils/logger.js';
import { ErrorCode, AppError, createError } from '../utils/errors.js';

const logger = createSourceLogger('RingBuffer');

export class RingBuffer {
  constructor(options = {}) {
    this.maxAgeHours = options.maxAgeHours || 48;
    this.maxSizeMB = options.maxSizeMB || 2;
    this.maxEntriesPerSource = options.maxEntriesPerSource || 10000;
    
    // In-memory storage (wird bei Bedarf auf Disk persistiert)
    this.buffers = new Map();
    
    // Start cleanup interval
    this.cleanupInterval = setInterval(() => this.cleanup(), 60000); // Every minute
    
    logger.info(`RingBuffer initialized: maxAge=${this.maxAgeHours}h, maxSize=${this.maxSizeMB}MB`);
  }
  
  /**
   * Add data point to buffer
   */
  async add(entry) {
    try {
      const { source, data, timestamp = Date.now() } = entry;
      
      if (!source) {
        throw createError.validationFailed('source', 'Source is required');
      }
      
      // Get or create buffer for this source
      if (!this.buffers.has(source)) {
        this.buffers.set(source, []);
      }
      
      const buffer = this.buffers.get(source);
      
      // Add new entry
      buffer.push({
        timestamp,
        data: this.compressData(data),
      });
      
      // Enforce max entries limit
      while (buffer.length > this.maxEntriesPerSource) {
        buffer.shift();
      }
      
      // Enforce age limit
      const cutoffTime = Date.now() - (this.maxAgeHours * 60 * 60 * 1000);
      while (buffer.length > 0 && buffer[0].timestamp < cutoffTime) {
        buffer.shift();
      }
      
      logger.debug(`Added entry to ${source}. Buffer size: ${buffer.length}`);
      
      return true;
    } catch (err) {
      logger.error('Failed to add entry:', err);
      throw err;
    }
  }
  
  /**
   * Get historical data for a source
   */
  async getHistory(source, duration = '24h', metric = null) {
    try {
      const buffer = this.buffers.get(source);
      
      if (!buffer || buffer.length === 0) {
        return [];
      }
      
      // Parse duration
      const durationMs = this.parseDuration(duration);
      const cutoffTime = Date.now() - durationMs;
      
      // Filter by time
      let result = buffer.filter(entry => entry.timestamp >= cutoffTime);
      
      // Filter by metric if specified
      if (metric) {
        result = result.map(entry => ({
          timestamp: entry.timestamp,
          data: this.extractMetric(entry.data, metric),
        })).filter(entry => entry.data !== undefined);
      }
      
      logger.debug(`Retrieved ${result.length} entries for ${source} (${duration})`);
      
      return result;
    } catch (err) {
      logger.error('Failed to get history:', err);
      throw err;
    }
  }
  
  /**
   * Get latest value for a source
   */
  async getLatest(source, metric = null) {
    const buffer = this.buffers.get(source);
    
    if (!buffer || buffer.length === 0) {
      return null;
    }
    
    const latest = buffer[buffer.length - 1];
    
    if (metric) {
      return this.extractMetric(latest.data, metric);
    }
    
    return latest.data;
  }
  
  /**
   * Get all sources with data
   */
  getSources() {
    return Array.from(this.buffers.keys());
  }
  
  /**
   * Get buffer statistics
   */
  getStats() {
    const stats = {};
    let totalSize = 0;
    
    for (const [source, buffer] of this.buffers.entries()) {
      const sizeBytes = JSON.stringify(buffer).length;
      totalSize += sizeBytes;
      
      stats[source] = {
        entries: buffer.length,
        sizeBytes,
        oldestEntry: buffer.length > 0 ? buffer[0].timestamp : null,
        newestEntry: buffer.length > 0 ? buffer[buffer.length - 1].timestamp : null,
      };
    }
    
    return {
      sources: Object.keys(stats),
      totalSizeBytes: totalSize,
      totalSizeMB: (totalSize / (1024 * 1024)).toFixed(2),
      details: stats,
    };
  }
  
  /**
   * Cleanup old entries
   */
  cleanup() {
    const cutoffTime = Date.now() - (this.maxAgeHours * 60 * 60 * 1000);
    let removedCount = 0;
    
    for (const [source, buffer] of this.buffers.entries()) {
      const initialLength = buffer.length;
      
      // Remove old entries
      while (buffer.length > 0 && buffer[0].timestamp < cutoffTime) {
        buffer.shift();
        removedCount++;
      }
      
      // Enforce max entries
      while (buffer.length > this.maxEntriesPerSource) {
        buffer.shift();
        removedCount++;
      }
      
      if (buffer.length !== initialLength) {
        logger.debug(`Cleaned up ${initialLength - buffer.length} entries from ${source}`);
      }
    }
    
    if (removedCount > 0) {
      logger.debug(`Total cleanup: removed ${removedCount} entries`);
    }
  }
  
  /**
   * Clear all data
   */
  clear() {
    this.buffers.clear();
    logger.info('RingBuffer cleared');
  }
  
  /**
   * Export data for transfer to ESP32/CYD
   */
  exportForDevice(source, duration = '24h') {
    const data = this.buffers.get(source) || [];
    const durationMs = this.parseDuration(duration);
    const cutoffTime = Date.now() - durationMs;
    
    const filtered = data.filter(entry => entry.timestamp >= cutoffTime);
    
    // Compress for transmission
    return {
      source,
      duration,
      count: filtered.length,
      data: filtered.map(entry => [entry.timestamp, entry.data]),
    };
  }
  
  /**
   * Compress data for storage efficiency
   */
  compressData(data) {
    // Remove null/undefined values
    const compressed = {};
    
    for (const [key, value] of Object.entries(data)) {
      if (value !== null && value !== undefined) {
        if (typeof value === 'object') {
          compressed[key] = this.compressData(value);
        } else {
          compressed[key] = value;
        }
      }
    }
    
    return compressed;
  }
  
  /**
   * Extract specific metric from data
   */
  extractMetric(data, metricPath) {
    const parts = metricPath.split('.');
    let current = data;
    
    for (const part of parts) {
      if (current && typeof current === 'object' && part in current) {
        current = current[part];
      } else {
        return undefined;
      }
    }
    
    return current;
  }
  
  /**
   * Parse duration string to milliseconds
   */
  parseDuration(duration) {
    const match = duration.match(/^(\d+)(s|m|h|d)$/);
    
    if (!match) {
      throw createError.validationFailed('duration', 'Invalid duration format. Use e.g., 24h, 7d, 30m');
    }
    
    const value = parseInt(match[1]);
    const unit = match[2];
    
    switch (unit) {
      case 's': return value * 1000;
      case 'm': return value * 60 * 1000;
      case 'h': return value * 60 * 60 * 1000;
      case 'd': return value * 24 * 60 * 60 * 1000;
      default: throw createError.validationFailed('duration', 'Unknown unit');
    }
  }
  
  /**
   * Destroy and cleanup resources
   */
  destroy() {
    if (this.cleanupInterval) {
      clearInterval(this.cleanupInterval);
    }
    this.clear();
    logger.info('RingBuffer destroyed');
  }
}
