import { WebSocketServer, WebSocket } from 'ws';
import { logger } from '../utils/logger';
import { RedisClient } from '../db/redisClient';
import { CollectorManager } from './collectorManager';

interface ClientConnection {
  ws: WebSocket;
  deviceId?: string;
  isAuthenticated: boolean;
  lastPing: number;
}

export class WebSocketHandler {
  private server: WebSocketServer;
  private clients: Map<WebSocket, ClientConnection> = new Map();
  private redisClient: RedisClient | null = null;
  private collectorManager: CollectorManager | null = null;
  private pingInterval: NodeJS.Timeout | null = null;

  constructor(server: WebSocketServer) {
    this.server = server;
    this.setupHandlers();
    this.startPingInterval();
  }

  private setupHandlers(): void {
    this.server.on('connection', (ws: WebSocket, req: any) => {
      const clientId = req.url?.split('?deviceId=')[1] || `client-${Date.now()}`;
      
      logger.info(`New WebSocket connection: ${clientId}`, { ip: req.socket.remoteAddress });

      const connection: ClientConnection = {
        ws,
        deviceId: clientId,
        isAuthenticated: false,
        lastPing: Date.now()
      };

      this.clients.set(ws, connection);

      ws.on('message', (data) => this.handleMessage(ws, data.toString()));
      ws.on('close', () => this.handleClose(ws));
      ws.on('error', (error) => this.handleError(ws, error));

      // Send welcome message
      this.send(ws, {
        type: 'welcome',
        message: 'Connected to Homelab Dashboard Suite',
        clientId
      });
    });
  }

  private handleMessage(ws: WebSocket, message: string): void {
    try {
      const data = JSON.parse(message);
      logger.debug('WS Message received', { type: data.type });

      switch (data.type) {
        case 'auth':
          this.handleAuth(ws, data.payload);
          break;
        case 'subscribe':
          this.handleSubscribe(ws, data.payload);
          break;
        case 'action':
          this.handleAction(ws, data.payload);
          break;
        case 'ping':
          this.handlePing(ws);
          break;
        default:
          logger.warn(`Unknown message type: ${data.type}`);
      }
    } catch (error) {
      logger.error('Error parsing WS message', error);
      this.send(ws, { type: 'error', message: 'Invalid message format' });
    }
  }

  private handleAuth(ws: WebSocket, payload: { token?: string }): void {
    // TODO: Implement JWT validation
    const connection = this.clients.get(ws);
    if (connection) {
      connection.isAuthenticated = true;
      this.send(ws, { type: 'auth_success', message: 'Authenticated' });
    }
  }

  private handleSubscribe(ws: WebSocket, payload: { channels?: string[] }): void {
    // Subscribe to specific metric channels
    logger.info(`Client ${ws} subscribed to channels`, payload.channels);
  }

  private async handleAction(ws: WebSocket, payload: { actionId: string; params?: any }): Promise<void> {
    logger.info(`Action requested: ${payload.actionId}`, payload.params);
    
    if (!this.collectorManager) {
      this.send(ws, { type: 'action_error', message: 'Collector manager not available' });
      return;
    }

    try {
      // Forward action to collector manager for execution
      const result = await this.collectorManager.executeAction(payload.actionId, payload.params);
      this.send(ws, { type: 'action_result', actionId: payload.actionId, success: true, result });
    } catch (error: any) {
      this.send(ws, { type: 'action_result', actionId: payload.actionId, success: false, error: error.message });
    }
  }

  private handlePing(ws: WebSocket): void {
    const connection = this.clients.get(ws);
    if (connection) {
      connection.lastPing = Date.now();
      this.send(ws, { type: 'pong', timestamp: Date.now() });
    }
  }

  private handleClose(ws: WebSocket): void {
    this.clients.delete(ws);
    logger.info('WebSocket connection closed');
  }

  private handleError(ws: WebSocket, error: Error): void {
    logger.error('WebSocket error', error);
    ws.close();
  }

  private startPingInterval(): void {
    this.pingInterval = setInterval(() => {
      this.clients.forEach((connection, ws) => {
        if (Date.now() - connection.lastPing > 60000) {
          logger.warn(`Closing stale connection: ${connection.deviceId}`);
          ws.close();
        } else {
          this.send(ws, { type: 'ping', timestamp: Date.now() });
        }
      });
    }, 30000);
  }

  public send(ws: WebSocket, data: any): void {
    if (ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify(data));
    }
  }

  public broadcast(data: any, excludeWs?: WebSocket): void {
    const message = JSON.stringify(data);
    this.clients.forEach((connection, ws) => {
      if (ws !== excludeWs && ws.readyState === WebSocket.OPEN) {
        ws.send(message);
      }
    });
  }

  public broadcastToChannel(channel: string, data: any): void {
    // Broadcast to all clients subscribed to a channel
    // Implementation depends on subscription tracking
    this.broadcast(data);
  }

  public setRedisClient(client: RedisClient): void {
    this.redisClient = client;
  }

  public setCollectorManager(manager: CollectorManager): void {
    this.collectorManager = manager;
  }

  public getClientCount(): number {
    return this.clients.size;
  }
}
