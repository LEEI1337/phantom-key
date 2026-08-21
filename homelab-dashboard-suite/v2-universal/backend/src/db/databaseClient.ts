import { Pool, QueryResult } from 'pg';
import { config } from './config';
import { logger } from '../utils/logger';

export class DatabaseClient {
  private pool: Pool | null = null;
  private reconnectAttempts = 0;
  private maxReconnectAttempts = 5;

  async connect(): Promise<void> {
    try {
      this.pool = new Pool({
        host: config.dbHost,
        port: config.dbPort,
        database: config.dbName,
        user: config.dbUser,
        password: config.dbPassword,
        max: 20,
        idleTimeoutMillis: 30000,
        connectionTimeoutMillis: 2000
      });

      // Test connection
      await this.pool.query('SELECT NOW()');
      
      this.pool.on('error', (err) => {
        logger.error('Unexpected database error', err);
        this.handleReconnect();
      });

      logger.info(`Connected to TimescaleDB at ${config.dbHost}:${config.dbPort}/${config.dbName}`);
      this.reconnectAttempts = 0;
    } catch (error) {
      logger.error('Failed to connect to TimescaleDB', error);
      throw error;
    }
  }

  private async handleReconnect(): Promise<void> {
    if (this.reconnectAttempts < this.maxReconnectAttempts) {
      this.reconnectAttempts++;
      const delay = Math.min(1000 * Math.pow(2, this.reconnectAttempts), 30000);
      
      logger.info(`Attempting DB reconnect ${this.reconnectAttempts}/${this.maxReconnectAttempts} in ${delay}ms`);
      
      setTimeout(async () => {
        try {
          await this.connect();
        } catch (error) {
          logger.error('Database reconnection failed', error);
        }
      }, delay);
    } else {
      logger.error('Max DB reconnect attempts reached');
    }
  }

  async query(text: string, params?: any[]): Promise<QueryResult> {
    if (!this.pool) throw new Error('Database not connected');
    
    const start = Date.now();
    try {
      const result = await this.pool.query(text, params);
      const duration = Date.now() - start;
      
      logger.debug('Executed query', { text, duration, rows: result.rowCount });
      return result;
    } catch (error) {
      logger.error('Database query error', { text, error });
      throw error;
    }
  }

  async getClient() {
    if (!this.pool) throw new Error('Database not connected');
    return await this.pool.connect();
  }

  async disconnect(): Promise<void> {
    if (this.pool) {
      await this.pool.end();
      this.pool = null;
      logger.info('Disconnected from TimescaleDB');
    }
  }

  getPool(): Pool | null {
    return this.pool;
  }
}
