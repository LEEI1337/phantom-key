/**
 * pfSense Data Aggregator
 * 
 * Sammelt Netzwerk-Traffic, Firewall-Status und System-Informationen
 */

import axios from 'axios';
import https from 'https';
import { createSourceLogger } from '../utils/logger.js';
import { createError } from '../utils/errors.js';

const logger = createSourceLogger('pfSense');

export class PfSenseAggregator {
  constructor(config) {
    this.config = config;
    this.enabled = !!(config.pfsenseUrl && config.pfsenseKey);
    this.api = null;
    
    if (this.enabled) {
      this.initialize();
    }
  }
  
  initialize() {
    const { pfsenseUrl, pfsenseKey, pfsenseSecret, pfsenseInsecure } = this.config;
    
    const agent = pfsenseInsecure ? new https.Agent({ rejectUnauthorized: false }) : undefined;
    
    this.api = axios.create({
      baseURL: `${pfsenseUrl}/api/v1`,
      auth: {
        username: pfsenseKey,
        password: pfsenseSecret,
      },
      httpsAgent: agent,
      timeout: 10000,
    });
    
    logger.info('pfSense aggregator initialized', { url: pfsenseUrl });
  }
  
  isEnabled() {
    return this.enabled;
  }
  
  async getStatus() {
    if (!this.enabled) {
      return { enabled: false };
    }
    
    try {
      const [systemInfo, interfaces, gateways, firewall] = await Promise.all([
        this.getSystemInfo(),
        this.getInterfaces(),
        this.getGateways(),
        this.getFirewallStatus(),
      ]);
      
      return {
        enabled: true,
        connected: true,
        system: systemInfo,
        interfaces,
        gateways,
        firewall,
        timestamp: Date.now(),
      };
    } catch (err) {
      logger.error('Failed to get pfSense status:', err);
      throw createError.sourceUnavailable('pfsense', err.message);
    }
  }
  
  async getSystemInfo() {
    try {
      const response = await this.api.get('/system/info');
      const data = response.data || {};
      
      return {
        hostname: data.hostname || 'unknown',
        version: data.version || 'unknown',
        uptime: data.uptime || 0,
        temp: data.temp || null,
        load: data.load_avg ? data.load_avg.map(l => Math.round(l * 100) / 100) : [0, 0, 0],
        cpuUsage: Math.round((data.cpu_usage || 0) * 100),
        memoryUsage: Math.round((data.memory_usage || 0) * 100),
        diskUsage: Math.round((data.disk_usage || 0) * 100),
      };
    } catch (err) {
      logger.warn('Failed to get pfSense system info:', err.message);
      return { error: err.message };
    }
  }
  
  async getInterfaces() {
    try {
      const response = await this.api.get('/interface');
      const interfaces = response.data || {};
      
      return {
        total: Object.keys(interfaces).length,
        list: Object.entries(interfaces).map(([name, iface]) => ({
          name,
          status: iface.status || 'unknown',
          ip4Address: iface.ipv4_address || 'N/A',
          ip4Subnet: iface.ipv4_subnet || 0,
          ip6Address: iface.ipv6_address || 'N/A',
          macAddress: iface.mac_address || 'N/A',
          inBytes: iface.in_bytes_total || 0,
          outBytes: iface.out_bytes_total || 0,
          inPackets: iface.in_packets_total || 0,
          outPackets: iface.out_packets_total || 0,
        })),
      };
    } catch (err) {
      logger.warn('Failed to get pfSense interfaces:', err.message);
      return { error: err.message };
    }
  }
  
  async getGateways() {
    try {
      const response = await this.api.get('/gateway/status');
      const gateways = response.data?.gateways || [];
      
      return {
        total: gateways.length,
        list: gateways.map(gw => ({
          name: gw.name || 'unknown',
          status: gw.status || 'unknown',
          address: gw.address || 'N/A',
          monitorAddress: gw.monitor_address || 'N/A',
          latency: gw.latency || null,
          loss: gw.loss || 0,
        })),
      };
    } catch (err) {
      logger.warn('Failed to get pfSense gateways:', err.message);
      return { error: err.message };
    }
  }
  
  async getFirewallStatus() {
    try {
      // Get basic firewall stats
      const response = await this.api.get('/firewall/status');
      const data = response.data || {};
      
      return {
        statesTotal: data.states_total || 0,
        statesCurrent: data.states_current || 0,
        statesPeak: data.states_peak || 0,
        packetsPassed: data.passed_packets || 0,
        bytesPassed: data.passed_bytes || 0,
        packetsBlocked: data.blocked_packets || 0,
        bytesBlocked: data.blocked_bytes || 0,
      };
    } catch (err) {
      logger.debug('Failed to get pfSense firewall status:', err.message);
      return { error: err.message };
    }
  }
}
