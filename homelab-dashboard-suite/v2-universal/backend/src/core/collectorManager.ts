import { logger } from '../utils/logger';
import { DockerRunner } from '../runners/dockerRunner';
import { BaseRunner } from '../runners/baseRunner';

interface Runner {
  id: string;
  type: string;
  start(): Promise<void>;
  stop(): Promise<void>;
  getStatus(): any;
}

export class CollectorManager {
  private runners: Map<string, Runner> = new Map();
  private isRunning: boolean = false;
  private healthCheckInterval: NodeJS.Timeout | null = null;

  async start(): Promise<void> {
    logger.info('Starting Collector Manager...');
    this.isRunning = true;

    // Initialize and start default runners
    await this.registerRunner(new DockerRunner());
    
    // Start health check loop
    this.startHealthChecks();
    
    logger.info('Collector Manager started successfully');
  }

  async stop(): Promise<void> {
    logger.info('Stopping Collector Manager...');
    this.isRunning = false;

    // Stop all runners
    const stopPromises = Array.from(this.runners.values()).map(runner => 
      runner.stop().catch(err => logger.error(`Error stopping runner ${runner.id}`, err))
    );

    await Promise.all(stopPromises);
    this.runners.clear();

    if (this.healthCheckInterval) {
      clearInterval(this.healthCheckInterval);
    }

    logger.info('Collector Manager stopped');
  }

  async registerRunner(runner: Runner): Promise<void> {
    logger.info(`Registering runner: ${runner.id} (${runner.type})`);
    
    try {
      await runner.start();
      this.runners.set(runner.id, runner);
      logger.info(`Runner ${runner.id} registered and started`);
    } catch (error) {
      logger.error(`Failed to register runner ${runner.id}`, error);
      throw error;
    }
  }

  async unregisterRunner(runnerId: string): Promise<void> {
    const runner = this.runners.get(runnerId);
    if (runner) {
      await runner.stop();
      this.runners.delete(runnerId);
      logger.info(`Runner ${runnerId} unregistered`);
    }
  }

  getRunner(runnerId: string): Runner | undefined {
    return this.runners.get(runnerId);
  }

  getAllRunners(): Runner[] {
    return Array.from(this.runners.values());
  }

  async executeAction(actionId: string, params?: any): Promise<any> {
    logger.info(`Executing action: ${actionId}`, params);
    
    // Route action to appropriate runner
    // This is a simplified implementation - in production, you'd have a proper action routing system
    for (const runner of this.runners.values()) {
      if (runner instanceof BaseRunner && runner.canExecuteAction(actionId)) {
        return await runner.executeAction(actionId, params);
      }
    }

    throw new Error(`Action ${actionId} not found or not executable`);
  }

  private startHealthChecks(): void {
    this.healthCheckInterval = setInterval(() => {
      this.runners.forEach((runner, id) => {
        const status = runner.getStatus();
        if (!status.healthy) {
          logger.warn(`Runner ${id} is unhealthy`, status);
          // Could trigger auto-restart here
        }
      });
    }, 30000); // Check every 30 seconds
  }

  getStatus(): any {
    return {
      isRunning: this.isRunning,
      runnerCount: this.runners.size,
      runners: Array.from(this.runners.entries()).map(([id, runner]) => ({
        id,
        type: runner.type,
        status: runner.getStatus()
      }))
    };
  }
}
