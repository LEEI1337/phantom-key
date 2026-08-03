/**
 * Base Aggregator Class
 * 
 * Abstract base class for all data aggregators
 */

export class BaseAggregator {
  constructor(config) {
    if (this.constructor === BaseAggregator) {
      throw new Error('BaseAggregator is abstract and cannot be instantiated');
    }
    
    this.config = config;
    this.enabled = false;
    this.lastError = null;
    this.lastSuccess = null;
  }
  
  /**
   * Check if aggregator is enabled
   */
  isEnabled() {
    return this.enabled;
  }
  
  /**
   * Get current status from source
   * @returns {Promise<Object>} Status data
   */
  async getStatus() {
    throw new Error('getStatus() must be implemented by subclass');
  }
  
  /**
   * Initialize the aggregator
   */
  initialize() {
    throw new Error('initialize() must be implemented by subclass');
  }
  
  /**
   * Test connection to source
   */
  async testConnection() {
    try {
      await this.getStatus();
      return { success: true, timestamp: Date.now() };
    } catch (err) {
      return { 
        success: false, 
        error: err.message,
        code: err.code,
        timestamp: Date.now() 
      };
    }
  }
  
  /**
   * Get aggregated data with error handling
   */
  async getSafeStatus() {
    try {
      const status = await this.getStatus();
      this.lastSuccess = Date.now();
      this.lastError = null;
      return status;
    } catch (err) {
      this.lastError = {
        message: err.message,
        code: err.code,
        timestamp: Date.now(),
      };
      
      return {
        enabled: this.enabled,
        connected: false,
        error: err.message,
        code: err.code,
        lastSuccess: this.lastSuccess,
      };
    }
  }
}
