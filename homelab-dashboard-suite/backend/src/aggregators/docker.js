/**
 * Docker Data Aggregator
 * 
 * Sammelt Container-Status, Ressourcen-Nutzung und Events von Docker
 */

import Docker from 'dockerode';
import { createSourceLogger } from '../utils/logger.js';
import { createError } from '../utils/errors.js';

const logger = createSourceLogger('Docker');

export class DockerAggregator {
  constructor(config) {
    this.config = config;
    this.enabled = !!config.dockerHost;
    this.docker = null;
    
    if (this.enabled) {
      this.initialize();
    }
  }
  
  initialize() {
    try {
      // Support both socket and TCP connections
      if (this.config.dockerHost?.startsWith('tcp://')) {
        this.docker = new Docker({ host: this.config.dockerHost.replace('tcp://', '') });
      } else {
        this.docker = new Docker({ socketPath: this.config.dockerHost || '/var/run/docker.sock' });
      }
      
      logger.info('Docker aggregator initialized');
    } catch (err) {
      logger.error('Failed to initialize Docker:', err.message);
      this.enabled = false;
    }
  }
  
  isEnabled() {
    return this.enabled;
  }
  
  async getStatus() {
    if (!this.enabled) {
      return { enabled: false };
    }
    
    try {
      const [info, containers, systemInfo] = await Promise.all([
        this.getContainerStats(),
        this.listContainers(),
        this.getSystemInfo(),
      ]);
      
      return {
        enabled: true,
        connected: true,
        containers,
        stats: info,
        system: systemInfo,
        timestamp: Date.now(),
      };
    } catch (err) {
      logger.error('Failed to get Docker status:', err);
      throw createError.sourceUnavailable('docker', err.message);
    }
  }
  
  async listContainers() {
    try {
      const allContainers = await this.docker.listContainers({ all: true });
      
      return {
        total: allContainers.length,
        running: allContainers.filter(c => c.State === 'running').length,
        paused: allContainers.filter(c => c.State === 'paused').length,
        stopped: allContainers.filter(c => c.State === 'exited' || c.State === 'created').length,
        list: allContainers.map(c => ({
          id: c.Id.substring(0, 12),
          name: c.Names[0].replace('/', ''),
          image: c.Image,
          state: c.State,
          status: c.Status,
          ports: c.Ports.map(p => `${p.PublicPort || 'N/A'}->${p.PrivatePort}`),
          created: c.Created,
        })),
      };
    } catch (err) {
      logger.warn('Failed to list containers:', err.message);
      return { error: err.message };
    }
  }
  
  async getContainerStats() {
    try {
      const containers = await this.docker.listContainers({ filters: { status: ['running'] } });
      
      const stats = [];
      
      for (const container of containers.slice(0, 10)) { // Limit to 10 for performance
        try {
          const stream = await this.docker.getContainer(container.Id).stats({ stream: false });
          const data = this.parseStats(stream);
          
          stats.push({
            id: container.Id.substring(0, 12),
            name: container.Names[0].replace('/', ''),
            cpu: data.cpu,
            memory: data.memory,
            networkRx: data.networkRx,
            networkTx: data.networkTx,
          });
        } catch (err) {
          logger.debug(`Failed to get stats for ${container.Names[0]}:`, err.message);
        }
      }
      
      return stats;
    } catch (err) {
      logger.warn('Failed to get container stats:', err.message);
      return { error: err.message };
    }
  }
  
  parseStats(statsData) {
    // Simplified parsing - in production you'd want more robust handling
    const read = statsData.read || {};
    const preRead = statsData.precpu_stats || {};
    const cpuStats = statsData.cpu_stats || {};
    
    // CPU calculation
    const cpuDelta = (cpuStats.cpu_usage?.total_usage || 0) - (preRead.cpu_usage?.total_usage || 0);
    const systemDelta = (cpuStats.system_cpu_usage || 0) - (preRead.system_cpu_usage || 0);
    const cpuUsage = systemDelta > 0 ? (cpuDelta / systemDelta) * cpuStats.online_cpus * 100 : 0;
    
    // Memory calculation
    const memory = statsData.memory_stats || {};
    const memoryUsage = memory.usage || 0;
    const memoryLimit = memory.limit || 1;
    const memoryPercent = (memoryUsage / memoryLimit) * 100;
    
    // Network calculation
    const networks = statsData.networks || {};
    let networkRx = 0;
    let networkTx = 0;
    
    Object.values(networks).forEach(net => {
      networkRx += net.rx_bytes || 0;
      networkTx += net.tx_bytes || 0;
    });
    
    return {
      cpu: Math.round(cpuUsage * 100) / 100,
      memory: Math.round(memoryUsage / (1024 * 1024)), // MB
      memoryPercent: Math.round(memoryPercent * 100) / 100,
      networkRx: Math.round(networkRx / (1024 * 1024)), // MB
      networkTx: Math.round(networkTx / (1024 * 1024)), // MB
    };
  }
  
  async getSystemInfo() {
    try {
      const info = await this.docker.info();
      
      return {
        containers: info.Containers,
        containersRunning: info.ContainersRunning,
        containersPaused: info.ContainersPaused,
        containersStopped: info.ContainersStopped,
        images: info.Images,
        driver: info.Driver,
        kernelVersion: info.KernelVersion,
        operatingSystem: info.OperatingSystem,
        architecture: info.Architecture,
        cpus: info.NCPU,
        memoryTotal: Math.round(info.MemTotal / (1024 * 1024 * 1024)), // GB
      };
    } catch (err) {
      logger.warn('Failed to get system info:', err.message);
      return { error: err.message };
    }
  }
}
