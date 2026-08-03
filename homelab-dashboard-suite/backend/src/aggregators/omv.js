/**
 * OMV (OpenMediaVault) Data Aggregator
 * 
 * Sammelt Festplatten-Status, SMART-Daten und Storage-Informationen
 */

import axios from 'axios';
import https from 'https';
import { createSourceLogger } from '../utils/logger.js';
import { createError } from '../utils/errors.js';

const logger = createSourceLogger('OMV');

export class OMVAggregator {
  constructor(config) {
    this.config = config;
    this.enabled = !!(config.omvUrl && config.omvUsername);
    this.api = null;
    this.sessionId = null;
    
    if (this.enabled) {
      this.initialize();
    }
  }
  
  initialize() {
    const { omvUrl, omvUsername, omvPassword } = this.config;
    
    // Create Axios instance
    const agent = new https.Agent({ rejectUnauthorized: false });
    
    this.api = axios.create({
      baseURL: omvUrl,
      headers: {
        'Content-Type': 'application/json',
        'Accept': 'application/json',
      },
      httpsAgent: agent,
      timeout: 10000,
      withCredentials: true,
    });
    
    // Store credentials for login
    this.credentials = {
      username: omvUsername,
      password: omvPassword,
    };
    
    logger.info('OMV aggregator initialized', { url: omvUrl });
  }
  
  isEnabled() {
    return this.enabled;
  }
  
  async getStatus() {
    if (!this.enabled) {
      return { enabled: false };
    }
    
    try {
      await this.ensureAuthenticated();
      
      const [systemInfo, disks, filesystems] = await Promise.all([
        this.getSystemInfo(),
        this.getDisks(),
        this.getFilesystems(),
      ]);
      
      return {
        enabled: true,
        connected: true,
        system: systemInfo,
        disks,
        filesystems,
        timestamp: Date.now(),
      };
    } catch (err) {
      logger.error('Failed to get OMV status:', err);
      throw createError.sourceUnavailable('omv', err.message);
    }
  }
  
  async ensureAuthenticated() {
    if (!this.sessionId) {
      await this.login();
    }
  }
  
  async login() {
    try {
      // OMV uses session-based auth
      const response = await this.api.post('/rpc.php', {
        service: 'Session',
        method: 'login',
        params: this.credentials,
      });
      
      this.sessionId = response.data.response?.sessionId;
      
      if (this.sessionId) {
        this.api.defaults.headers.common['X-OMV-SessionId'] = this.sessionId;
      }
      
      logger.debug('OMV login successful');
    } catch (err) {
      logger.warn('OMV login failed:', err.message);
      throw createError.authFailed('omv', err.message);
    }
  }
  
  async getSystemInfo() {
    try {
      // Get basic system information
      const response = await this.api.post('/rpc.php', {
        service: 'System',
        method: 'getInformation',
        params: {},
      });
      
      const data = response.data.response || {};
      
      return {
        hostname: data.hostname || 'unknown',
        platform: data.platform || 'unknown',
        cpuModel: data.cpuModel || 'unknown',
        memoryTotal: Math.round((data.memoryTotal || 0) / (1024 * 1024)), // MB
        memoryUsed: Math.round((data.memoryUsed || 0) / (1024 * 1024)), // MB
        uptime: data.uptime || 0,
      };
    } catch (err) {
      logger.warn('Failed to get OMV system info:', err.message);
      return { error: err.message };
    }
  }
  
  async getDisks() {
    try {
      const response = await this.api.post('/rpc.php', {
        service: 'Diskmgmt',
        method: 'enumerateDevices',
        params: { start: 0, limit: 25 },
      });
      
      const disks = response.data.response?.objects || [];
      
      return {
        total: disks.length,
        list: disks.map(disk => ({
          device: disk.device,
          model: disk.model,
          serialNumber: disk.serialNumber,
          size: Math.round((disk.size || 0) / (1024 * 1024 * 1024)), // GB
          temperature: disk.temperature || null,
          smartStatus: disk.smartStatus || 'unknown',
          mountPoint: disk.mountPoint,
        })),
      };
    } catch (err) {
      logger.warn('Failed to get OMV disks:', err.message);
      return { error: err.message };
    }
  }
  
  async getFilesystems() {
    try {
      const response = await this.api.post('/rpc.php', {
        service: 'Filesystems',
        method: 'enumerateMountedFilesystems',
        params: {},
      });
      
      const filesystems = response.data.response?.objects || [];
      
      return {
        total: filesystems.length,
        list: filesystems.map(fs => ({
          device: fs.device,
          dirName: fs.dirName,
          type: fs.type,
          sizeTotal: Math.round((fs.sizeTotal || 0) / (1024 * 1024 * 1024)), // GB
          sizeUsed: Math.round((fs.sizeUsed || 0) / (1024 * 1024 * 1024)), // GB
          usage: Math.round(((fs.sizeUsed || 0) / (fs.sizeTotal || 1)) * 100),
        })),
      };
    } catch (err) {
      logger.warn('Failed to get OMV filesystems:', err.message);
      return { error: err.message };
    }
  }
}
