import { BaseRunner } from './baseRunner';
import { logger } from '../utils/logger';
import Docker from 'dockerode';

export class DockerRunner extends BaseRunner {
  private docker: Docker;
  private metricsCount: number = 0;

  constructor() {
    super('docker-local', 'docker');
    this.docker = new Docker();
    this.pollInterval = 3000; // Poll every 3 seconds
  }

  async start(): Promise<void> {
    logger.info('Starting Docker runner...');
    
    try {
      // Test Docker connection
      await this.docker.ping();
      logger.info('Connected to Docker daemon');
      
      await this.startPolling(async () => {
        await this.collectData();
      });
    } catch (error: any) {
      logger.error('Failed to start Docker runner', error);
      throw error;
    }
  }

  async stop(): Promise<void> {
    this.stopPolling();
    logger.info('Docker runner stopped');
  }

  async collectData(): Promise<void> {
    const startTime = Date.now();
    
    try {
      // Get container list
      const containers = await this.docker.listContainers({ all: true });
      
      const metrics = {
        totalContainers: containers.length,
        runningContainers: containers.filter(c => c.State === 'running').length,
        pausedContainers: containers.filter(c => c.State === 'paused').length,
        stoppedContainers: containers.filter(c => c.State === 'exited').length,
        containers: containers.map(c => ({
          id: c.Id.substring(0, 12),
          name: c.Names[0].replace('/', ''),
          image: c.Image,
          state: c.State,
          status: c.Status,
          ports: c.Ports,
          createdAt: c.Created
        }))
      };

      // Store in Redis (live data)
      // In production, you'd inject the Redis client
      this.metricsCount = metrics.totalContainers;
      this.lastUpdate = Date.now();

      logger.debug(`Collected Docker metrics: ${metrics.totalContainers} containers`, {
        running: metrics.runningContainers,
        stopped: metrics.stoppedContainers
      });

    } catch (error: any) {
      logger.error('Error collecting Docker metrics', error);
      throw error;
    }
  }

  getStatus(): any {
    const baseStatus = super.getStatus();
    return {
      ...baseStatus,
      metricsCount: this.metricsCount,
      dockerConnection: this.docker ? 'connected' : 'disconnected'
    };
  }

  canExecuteAction(actionId: string): boolean {
    return ['container.restart', 'container.stop', 'container.start', 'container.logs'].includes(actionId);
  }

  async executeAction(actionId: string, params?: { containerId?: string }): Promise<any> {
    if (!params?.containerId) {
      throw new Error('Container ID required for Docker actions');
    }

    const container = this.docker.getContainer(params.containerId);

    switch (actionId) {
      case 'container.restart':
        logger.info(`Restarting container: ${params.containerId}`);
        await container.restart();
        return { success: true, action: 'restart', containerId: params.containerId };

      case 'container.stop':
        logger.info(`Stopping container: ${params.containerId}`);
        await container.stop();
        return { success: true, action: 'stop', containerId: params.containerId };

      case 'container.start':
        logger.info(`Starting container: ${params.containerId}`);
        await container.start();
        return { success: true, action: 'start', containerId: params.containerId };

      case 'container.logs':
        logger.info(`Fetching logs for container: ${params.containerId}`);
        const logs = await container.logs({ tail: 50, stdout: true, stderr: true });
        return { 
          success: true, 
          action: 'logs', 
          containerId: params.containerId,
          logs: logs.toString()
        };

      default:
        throw new Error(`Unknown action: ${actionId}`);
    }
  }
}
