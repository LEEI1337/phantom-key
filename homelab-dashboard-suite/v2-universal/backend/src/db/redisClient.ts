import { createClient, RedisClientType } from 'redis';
import { config } from './config';
import { logger } from '../utils/logger';

export class RedisClient {
  private client: RedisClientType | null = null;
  private reconnectAttempts = 0;
  private maxReconnectAttempts = 5;

  async connect(): Promise<void> {
    try {
      this.client = createClient({
        url: `redis://${config.redisHost}:${config.redisPort}`
      });

      this.client.on('error', (err) => {
        logger.error('Redis Client Error', err);
        this.handleReconnect();
      });

      this.client.on('connect', () => {
        logger.info(`Connected to Redis at ${config.redisHost}:${config.redisPort}`);
        this.reconnectAttempts = 0;
      });

      await this.client.connect();
    } catch (error) {
      logger.error('Failed to connect to Redis', error);
      throw error;
    }
  }

  private async handleReconnect(): Promise<void> {
    if (this.reconnectAttempts < this.maxReconnectAttempts) {
      this.reconnectAttempts++;
      const delay = Math.min(1000 * Math.pow(2, this.reconnectAttempts), 30000);
      
      logger.info(`Attempting Redis reconnect ${this.reconnectAttempts}/${this.maxReconnectAttempts} in ${delay}ms`);
      
      setTimeout(async () => {
        try {
          await this.connect();
        } catch (error) {
          logger.error('Redis reconnection failed', error);
        }
      }, delay);
    } else {
      logger.error('Max Redis reconnect attempts reached');
    }
  }

  async set(key: string, value: any, expireSeconds?: number): Promise<void> {
    if (!this.client) throw new Error('Redis not connected');
    
    const stringValue = typeof value === 'string' ? value : JSON.stringify(value);
    
    if (expireSeconds) {
      await this.client.setEx(key, expireSeconds, stringValue);
    } else {
      await this.client.set(key, stringValue);
    }
  }

  async get(key: string): Promise<any> {
    if (!this.client) throw new Error('Redis not connected');
    
    const value = await this.client.get(key);
    if (!value) return null;
    
    try {
      return JSON.parse(value);
    } catch {
      return value;
    }
  }

  async publish(channel: string, message: string): Promise<void> {
    if (!this.client) throw new Error('Redis not connected');
    await this.client.publish(channel, message);
  }

  async disconnect(): Promise<void> {
    if (this.client) {
      await this.client.quit();
      this.client = null;
      logger.info('Disconnected from Redis');
    }
  }

  getClient(): RedisClientType | null {
    return this.client;
  }
}
