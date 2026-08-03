/**
 * Home Assistant Data Aggregator
 * 
 * Sammelt Status von Geräten, Sensoren und Automationen
 */

import axios from 'axios';
import { createSourceLogger } from '../utils/logger.js';
import { createError } from '../utils/errors.js';

const logger = createSourceLogger('HomeAssistant');

export class HomeAssistantAggregator {
  constructor(config) {
    this.config = config;
    this.enabled = !!(config.haUrl && config.haToken);
    this.api = null;
    
    if (this.enabled) {
      this.initialize();
    }
  }
  
  initialize() {
    const { haUrl, haToken } = this.config;
    
    this.api = axios.create({
      baseURL: haUrl,
      headers: {
        'Authorization': `Bearer ${haToken}`,
        'Content-Type': 'application/json',
      },
      timeout: 10000,
    });
    
    logger.info('Home Assistant aggregator initialized', { url: haUrl });
  }
  
  isEnabled() {
    return this.enabled;
  }
  
  async getStatus() {
    if (!this.enabled) {
      return { enabled: false };
    }
    
    try {
      const [states, config, stats] = await Promise.all([
        this.getStates(),
        this.getConfig(),
        this.getStats(),
      ]);
      
      return {
        enabled: true,
        connected: true,
        states,
        config,
        stats,
        timestamp: Date.now(),
      };
    } catch (err) {
      logger.error('Failed to get HA status:', err);
      throw createError.sourceUnavailable('homeassistant', err.message);
    }
  }
  
  async getStates() {
    try {
      const response = await this.api.get('/api/states');
      const states = response.data || [];
      
      // Group by domain
      const byDomain = {};
      states.forEach(state => {
        const domain = state.entity_id.split('.')[0];
        if (!byDomain[domain]) byDomain[domain] = [];
        byDomain[domain].push({
          entity_id: state.entity_id,
          state: state.state,
          attributes: state.attributes,
        });
      });
      
      // Summary counts
      const summary = {
        total: states.length,
        on: states.filter(s => s.state === 'on').length,
        off: states.filter(s => s.state === 'off').length,
        unavailable: states.filter(s => s.state === 'unavailable').length,
      };
      
      return {
        summary,
        byDomain: Object.keys(byDomain).map(domain => ({
          domain,
          count: byDomain[domain].length,
          entities: byDomain[domain].slice(0, 20), // Limit for performance
        })),
      };
    } catch (err) {
      logger.warn('Failed to get HA states:', err.message);
      return { error: err.message };
    }
  }
  
  async getConfig() {
    try {
      const response = await this.api.get('/api/config');
      const config = response.data || {};
      
      return {
        latitude: config.latitude,
        longitude: config.longitude,
        elevation: config.elevation,
        unitSystem: config.unit_system,
        locationName: config.location_name,
        timeZone: config.time_zone,
        version: config.version,
      };
    } catch (err) {
      logger.warn('Failed to get HA config:', err.message);
      return { error: err.message };
    }
  }
  
  async getStats() {
    try {
      const response = await this.api.get('/api/stats');
      const stats = response.data || {};
      
      return {
        domains: stats.domains || 0,
        entities: stats.entities || 0,
        areas: stats.areas || 0,
        floors: stats.floors || 0,
        labels: stats.labels || 0,
        devices: stats.devices || 0,
        scenes: stats.scenes || 0,
        automations: stats.automations || 0,
        scripts: stats.scripts || 0,
      };
    } catch (err) {
      logger.debug('Failed to get HA stats:', err.message);
      return { error: err.message };
    }
  }
}
