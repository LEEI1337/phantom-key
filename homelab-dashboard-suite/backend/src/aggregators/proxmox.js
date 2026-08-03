/**
 * Proxmox VE Data Aggregator
 * 
 * Sammelt VM-Status, Ressourcen-Nutzung und GPU-Informationen von Proxmox
 */

import axios from 'axios';
import https from 'https';
import { createSourceLogger } from '../utils/logger.js';
import { createError, ErrorCode } from '../utils/errors.js';
import { retryWithBackoff } from '../utils/errors.js';

const logger = createSourceLogger('Proxmox');

export class ProxmoxAggregator {
  constructor(config) {
    this.config = config;
    this.enabled = !!(config.proxmoxUrl && config.proxmoxToken);
    this.api = null;
    
    if (this.enabled) {
      this.initialize();
    }
  }
  
  initialize() {
    const { proxmoxUrl, proxmoxToken, proxmoxSecret, proxmoxNode, proxmoxInsecure } = this.config;
    
    // Create Axios instance with auth
    const agent = proxmoxInsecure ? new https.Agent({ rejectUnauthorized: false }) : undefined;
    
    this.api = axios.create({
      baseURL: proxmoxUrl,
      headers: {
        'Authorization': `PVEAPIToken=${proxmoxToken}=${proxmoxSecret}`,
      },
      httpsAgent: agent,
      timeout: 10000,
    });
    
    this.node = proxmoxNode || 'pve';
    
    logger.info('Proxmox aggregator initialized', { url: proxmoxUrl, node: this.node });
  }
  
  isEnabled() {
    return this.enabled;
  }
  
  async getStatus() {
    if (!this.enabled) {
      return { enabled: false };
    }
    
    try {
      const [clusterStatus, nodeResources, vms, lxcContainers] = await Promise.all([
        this.getClusterStatus(),
        this.getNodeResources(),
        this.getVMs(),
        this.getLXCContainers(),
      ]);
      
      return {
        enabled: true,
        connected: true,
        cluster: clusterStatus,
        node: nodeResources,
        vms,
        containers: lxcContainers,
        timestamp: Date.now(),
      };
    } catch (err) {
      logger.error('Failed to get Proxmox status:', err);
      throw createError.sourceUnavailable('proxmox', err.message);
    }
  }
  
  async getClusterStatus() {
    try {
      const response = await this.api.get('/api2/json/cluster/status');
      return {
        quorate: response.data.data.quorate ?? false,
        nodes: response.data.data.length,
      };
    } catch (err) {
      logger.warn('Failed to get cluster status:', err.message);
      return { error: err.message };
    }
  }
  
  async getNodeResources() {
    try {
      const response = await this.api.get(`/api2/json/nodes/${this.node}/status`);
      const data = response.data.data;
      
      return {
        cpu: {
          usage: Math.round((data.cpu ?? 0) * 100),
          cores: data.cpus ?? 0,
        },
        memory: {
          used: Math.round((data.mem ?? 0) / (1024 * 1024)),
          total: Math.round((data.maxmem ?? 0) / (1024 * 1024)),
          usage: Math.round(((data.mem ?? 0) / (data.maxmem ?? 1)) * 100),
        },
        storage: {
          used: Math.round((data.rootfs?.used ?? 0) / (1024 * 1024 * 1024)),
          total: Math.round((data.rootfs?.total ?? 0) / (1024 * 1024 * 1024)),
          usage: Math.round(((data.rootfs?.used ?? 0) / (data.rootfs?.total ?? 1)) * 100),
        },
        uptime: data.uptime ?? 0,
      };
    } catch (err) {
      logger.warn('Failed to get node resources:', err.message);
      return { error: err.message };
    }
  }
  
  async getVMs() {
    try {
      const response = await this.api.get(`/api2/json/nodes/${this.node}/qemu`);
      const vms = response.data.data || [];
      
      return {
        total: vms.length,
        running: vms.filter(vm => vm.status === 'running').length,
        stopped: vms.filter(vm => vm.status === 'stopped').length,
        list: vms.map(vm => ({
          id: vm.vmid,
          name: vm.name,
          status: vm.status,
          cpu: Math.round((vm.cpu ?? 0) * 100),
          memory: Math.round((vm.maxmem ?? 0) / (1024 * 1024)),
          disk: Math.round((vm.maxdisk ?? 0) / (1024 * 1024 * 1024)),
        })),
      };
    } catch (err) {
      logger.warn('Failed to get VMs:', err.message);
      return { error: err.message };
    }
  }
  
  async getLXCContainers() {
    try {
      const response = await this.api.get(`/api2/json/nodes/${this.node}/lxc`);
      const containers = response.data.data || [];
      
      return {
        total: containers.length,
        running: containers.filter(c => c.status === 'running').length,
        stopped: containers.filter(c => c.status === 'stopped').length,
        list: containers.map(c => ({
          id: c.vmid,
          name: c.name,
          status: c.status,
          cpu: Math.round((c.cpu ?? 0) * 100),
          memory: Math.round((c.maxmem ?? 0) / (1024 * 1024)),
          disk: Math.round((c.maxdisk ?? 0) / (1024 * 1024 * 1024)),
        })),
      };
    } catch (err) {
      logger.warn('Failed to get LXC containers:', err.message);
      return { error: err.message };
    }
  }
  
  async getGPUInfo() {
    try {
      // Get PCI devices to find GPUs
      const response = await this.api.get(`/api2/json/nodes/${this.node}/hardware/pci`);
      const devices = response.data.data || [];
      
      const gpus = devices.filter(device => 
        device.class?.includes('VGA') || device.class?.includes('3D')
      );
      
      return {
        count: gpus.length,
        devices: gpus.map(gpu => ({
          id: gpu.id,
          vendor: gpu.vendor,
          model: gpu.model,
          class: gpu.class,
          passthrough: gpu.devices?.length > 0,
        })),
      };
    } catch (err) {
      logger.debug('Failed to get GPU info:', err.message);
      return { error: err.message };
    }
  }
}
