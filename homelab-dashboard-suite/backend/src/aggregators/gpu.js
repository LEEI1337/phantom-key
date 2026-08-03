/**
 * GPU Data Aggregator
 * 
 * Sammelt GPU-Temperatur, Auslastung und Leistungsinformationen
 */

import { createSourceLogger } from '../utils/logger.js';
import { createError } from '../utils/errors.js';
import { exec } from 'child_process';
import { promisify } from 'util';

const execAsync = promisify(exec);
const logger = createSourceLogger('GPU');

export class GPUAggregator {
  constructor(config) {
    this.config = config;
    this.enabled = config.gpuEnabled === true || config.gpuEnabled === 'true';
    this.nvmlAvailable = false;
    
    if (this.enabled) {
      this.initialize();
    }
  }
  
  async initialize() {
    try {
      // Test if nvidia-smi is available
      await execAsync('nvidia-smi --query-gpu=count --format=csv,noheader');
      this.nvmlAvailable = true;
      logger.info('GPU aggregator initialized with NVML');
    } catch (err) {
      logger.warn('NVML not available, GPU monitoring disabled:', err.message);
      this.enabled = false;
    }
  }
  
  isEnabled() {
    return this.enabled && this.nvmlAvailable;
  }
  
  async getStatus() {
    if (!this.enabled) {
      return { enabled: false };
    }
    
    try {
      const gpus = await this.getGPUInfo();
      
      return {
        enabled: true,
        connected: true,
        count: gpus.length,
        gpus,
        timestamp: Date.now(),
      };
    } catch (err) {
      logger.error('Failed to get GPU status:', err);
      throw createError.sourceUnavailable('gpu', err.message);
    }
  }
  
  async getGPUInfo() {
    try {
      // Query all GPU metrics in one call
      const query = [
        'index',
        'name',
        'fan.speed',
        'temperature.gpu',
        'utilization.gpu',
        'utilization.memory',
        'memory.total',
        'memory.used',
        'memory.free',
        'power.draw',
        'power.limit',
        'clocks.current.graphics',
        'clocks.current.memory',
        'pstate',
      ].join(',');
      
      const { stdout } = await execAsync(
        `nvidia-smi --query-gpu=${query} --format=csv,noheader,nounits`
      );
      
      const lines = stdout.trim().split('\n');
      
      return lines.map(line => {
        const values = line.split(', ');
        
        return {
          index: parseInt(values[0]) || 0,
          name: values[1] || 'Unknown GPU',
          fanSpeed: parseInt(values[2]) || 0,
          temperature: parseInt(values[3]) || 0,
          utilization: parseInt(values[4]) || 0,
          memoryUtilization: parseInt(values[5]) || 0,
          memoryTotal: parseInt(values[6]) || 0,
          memoryUsed: parseInt(values[7]) || 0,
          memoryFree: parseInt(values[8]) || 0,
          powerDraw: parseFloat(values[9]) || 0,
          powerLimit: parseFloat(values[10]) || 0,
          clockGraphics: parseInt(values[11]) || 0,
          clockMemory: parseInt(values[12]) || 0,
          pstate: values[13] || 'unknown',
        };
      });
    } catch (err) {
      logger.warn('Failed to get GPU info:', err.message);
      return [];
    }
  }
  
  async getGPUProcesses() {
    try {
      const { stdout } = await execAsync(
        'nvidia-smi --query-compute-apps=gpu_name,pid,process_name,memory_used --format=csv,noheader'
      );
      
      const lines = stdout.trim().split('\n').filter(l => l.trim());
      
      return lines.map(line => {
        const values = line.split(', ');
        
        return {
          gpu: values[0] || 'unknown',
          pid: parseInt(values[1]) || 0,
          processName: values[2] || 'unknown',
          memoryUsed: parseInt(values[3]) || 0,
        };
      });
    } catch (err) {
      logger.debug('Failed to get GPU processes:', err.message);
      return [];
    }
  }
}
