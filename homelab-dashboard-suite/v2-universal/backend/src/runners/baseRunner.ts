import { logger } from '../utils/logger';
import axios from 'axios';

export interface RunnerStatus {
  healthy: boolean;
  lastUpdate: number;
  error?: string;
  metricsCount?: number;
}

export abstract class BaseRunner {
  protected id: string;
  protected type: string;
  protected isRunning: boolean = false;
  protected lastUpdate: number = 0;
  protected error?: string;
  protected pollInterval: number = 5000;
  protected pollTimer: NodeJS.Timeout | null = null;

  constructor(id: string, type: string) {
    this.id = id;
    this.type = type;
  }

  abstract start(): Promise<void>;
  abstract stop(): Promise<void>;
  abstract collectData(): Promise<any>;

  getStatus(): RunnerStatus {
    return {
      healthy: !this.error && this.isRunning,
      lastUpdate: this.lastUpdate,
      error: this.error,
      metricsCount: 0 // Override in subclasses
    };
  }

  canExecuteAction(actionId: string): boolean {
    return false; // Override in subclasses that support actions
  }

  async executeAction(actionId: string, params?: any): Promise<any> {
    throw new Error(`Action ${actionId} not supported by this runner`);
  }

  protected async startPolling(collectFn: () => Promise<void>): Promise<void> {
    this.isRunning = true;
    this.error = undefined;
    
    // Initial collection
    try {
      await collectFn();
    } catch (err: any) {
      this.error = err.message;
      logger.error(`Initial collection failed for ${this.id}`, err);
    }

    // Start polling loop
    this.pollTimer = setInterval(async () => {
      try {
        await collectFn();
        this.error = undefined;
      } catch (err: any) {
        this.error = err.message;
        logger.error(`Collection failed for ${this.id}`, err);
      }
    }, this.pollInterval);

    logger.info(`Started polling for ${this.id} every ${this.pollInterval}ms`);
  }

  protected stopPolling(): void {
    this.isRunning = false;
    if (this.pollTimer) {
      clearInterval(this.pollTimer);
      this.pollTimer = null;
    }
    logger.info(`Stopped polling for ${this.id}`);
  }

  protected async makeRequest<T>(url: string, options?: any): Promise<T> {
    const response = await axios.get<T>(url, {
      timeout: 5000,
      ...options
    });
    return response.data;
  }
}
