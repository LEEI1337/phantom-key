/**
 * Homelab Aggregator - Main Entry Point
 * 
 * Sammelt Daten von verschiedenen Homelab-Quellen und stellt sie
 * via REST API, WebSocket und MQTT zur Verfügung.
 */

import express from 'express';
import { createServer } from 'http';
import { WebSocketServer } from 'ws';
import mqtt from 'mqtt';
import Redis from 'ioredis';
import Database from 'better-sqlite3';
import dotenv from 'dotenv';
import cors from 'cors';
import promClient from 'prom-client';
import winston from 'winston';

// Load environment variables
dotenv.config();

// Import modules
import { logger } from './utils/logger.js';
import { ProxmoxAggregator } from './aggregators/proxmox.js';
import { DockerAggregator } from './aggregators/docker.js';
import { OMVAggregator } from './aggregators/omv.js';
import { PfSenseAggregator } from './aggregators/pfsense.js';
import { GPUAggregator } from './aggregators/gpu.js';
import { HomeAssistantAggregator } from './aggregators/homeassistant.js';
import { RingBuffer } from './storage/ringbuffer.js';
import { ErrorCode, AppError } from './utils/errors.js';

// Configuration
const config = {
  apiPort: parseInt(process.env.API_PORT) || 3000,
  wsPort: parseInt(process.env.WS_PORT) || 3001,
  logLevel: process.env.LOG_LEVEL || 'info',
  debugMode: process.env.DEBUG_MODE === 'true',
  retentionHours: parseInt(process.env.RETENTION_HOURS) || 48,
};

// Initialize Express
const app = express();
const server = createServer(app);
const wsServer = new WebSocketServer({ port: config.wsPort });

// Middleware
app.use(cors());
app.use(express.json());

// Prometheus metrics
const collectDefaultMetrics = promClient.collectDefaultMetrics;
collectDefaultMetrics({ register: promClient.register });

// Metrics
const metrics = {
  dataPointsCollected: new promClient.Counter({
    name: 'homelab_datapoints_collected_total',
    help: 'Total number of data points collected',
    labelNames: ['source'],
  }),
  websocketConnections: new promClient.Gauge({
    name: 'homelab_websocket_connections',
    help: 'Number of active WebSocket connections',
  }),
  mqttMessagesPublished: new promClient.Counter({
    name: 'homelab_mqtt_messages_published_total',
    help: 'Total number of MQTT messages published',
  }),
  aggregationDuration: new promClient.Histogram({
    name: 'homelab_aggregation_duration_seconds',
    help: 'Time spent aggregating data',
    labelNames: ['source'],
    buckets: [0.1, 0.5, 1, 2, 5],
  }),
};

// Initialize Redis
let redis;
try {
  redis = new Redis(process.env.REDIS_URL || 'redis://localhost:6379');
  redis.on('error', (err) => logger.error('Redis error:', err));
  logger.info('Redis connected successfully');
} catch (err) {
  logger.warn('Redis not available, using in-memory storage');
}

// Initialize SQLite for time-series data
const db = new Database(process.env.DB_PATH || '/data/timeseries.db');
db.pragma('journal_mode = WAL');

// Create tables
db.exec(`
  CREATE TABLE IF NOT EXISTS metrics (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    source TEXT NOT NULL,
    metric_name TEXT NOT NULL,
    value REAL NOT NULL,
    timestamp INTEGER NOT NULL,
    metadata TEXT
  );
  
  CREATE INDEX IF NOT EXISTS idx_metrics_source_time 
  ON metrics(source, timestamp DESC);
  
  CREATE TABLE IF NOT EXISTS errors (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    code TEXT NOT NULL,
    message TEXT NOT NULL,
    source TEXT,
    timestamp INTEGER NOT NULL
  );
`);

// Initialize Ring Buffer for recent data
const ringBuffer = new RingBuffer({
  maxAgeHours: config.retentionHours,
  maxSizeMB: 2, // Limit to 2MB for ESP32 compatibility
});

// Initialize Aggregators
const aggregators = {
  proxmox: new ProxmoxAggregator(config),
  docker: new DockerAggregator(config),
  omv: new OMVAggregator(config),
  pfsense: new PfSenseAggregator(config),
  gpu: new GPUAggregator(config),
  homeassistant: new HomeAssistantAggregator(config),
};

// Initialize MQTT client
let mqttClient;
try {
  mqttClient = mqtt.connect(process.env.MQTT_BROKER || 'mqtt://localhost:1883', {
    username: process.env.MQTT_USER,
    password: process.env.MQTT_PASS,
    reconnectPeriod: 5000,
  });
  
  mqttClient.on('connect', () => {
    logger.info('MQTT connected');
    mqttClient.subscribe('homelab/#');
  });
  
  mqttClient.on('message', async (topic, message) => {
    try {
      const data = JSON.parse(message.toString());
      logger.debug(`MQTT message on ${topic}:`, data);
      
      // Publish to WebSocket clients
      broadcastToWebSockets({ topic, data });
      
      // Store in ring buffer
      await ringBuffer.add({ topic, data, timestamp: Date.now() });
    } catch (err) {
      logger.error('MQTT message processing error:', err);
    }
  });
} catch (err) {
  logger.warn('MQTT not available:', err.message);
}

// WebSocket clients tracking
const wsClients = new Set();

wsServer.on('connection', (ws) => {
  wsClients.add(ws);
  metrics.websocketConnections.inc();
  logger.info(`WebSocket client connected. Total: ${wsClients.size}`);
  
  ws.on('message', async (message) => {
    try {
      const msg = JSON.parse(message);
      logger.debug('WS message:', msg);
      
      // Handle commands from clients
      if (msg.command === 'subscribe') {
        ws.subscriptions = msg.topics || [];
      } else if (msg.command === 'get_history') {
        const history = await ringBuffer.getHistory(msg.source, msg.duration || '1h');
        ws.send(JSON.stringify({ type: 'history', source: msg.source, data: history }));
      }
    } catch (err) {
      logger.error('WebSocket message error:', err);
    }
  });
  
  ws.on('close', () => {
    wsClients.delete(ws);
    metrics.websocketConnections.dec();
    logger.info(`WebSocket client disconnected. Total: ${wsClients.size}`);
  });
  
  ws.on('error', (err) => {
    logger.error('WebSocket error:', err);
    wsClients.delete(ws);
    metrics.websocketConnections.dec();
  });
});

// Helper function to broadcast to all WebSocket clients
function broadcastToWebSockets(data) {
  const message = JSON.stringify(data);
  wsClients.forEach((client) => {
    if (client.readyState === 1) { // OPEN
      if (!client.subscriptions || client.subscriptions.some(s => data.topic?.includes(s))) {
        client.send(message);
      }
    }
  });
}

// REST API Routes

// Health check
app.get('/health', (req, res) => {
  res.json({
    status: 'ok',
    timestamp: Date.now(),
    uptime: process.uptime(),
    version: '1.0.0',
  });
});

// Metrics endpoint for Prometheus
app.get('/metrics', async (req, res) => {
  res.set('Content-Type', promClient.register.contentType);
  res.end(await promClient.register.metrics());
});

// Get current status from all sources
app.get('/api/status', async (req, res) => {
  try {
    const status = {};
    
    for (const [name, aggregator] of Object.entries(aggregators)) {
      try {
        status[name] = await aggregator.getStatus();
        metrics.dataPointsCollected.inc({ source: name }, Object.keys(status[name]).length);
      } catch (err) {
        logger.error(`Error getting status from ${name}:`, err);
        status[name] = { error: err.message, code: ErrorCode.SOURCE_UNAVAILABLE };
      }
    }
    
    res.json(status);
  } catch (err) {
    logger.error('Status endpoint error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

// Get specific source status
app.get('/api/status/:source', async (req, res) => {
  const { source } = req.params;
  
  if (!aggregators[source]) {
    return res.status(404).json({ error: 'Source not found', code: ErrorCode.SOURCE_NOT_FOUND });
  }
  
  try {
    const status = await aggregators[source].getStatus();
    metrics.dataPointsCollected.inc({ source }, Object.keys(status).length);
    res.json(status);
  } catch (err) {
    logger.error(`Status endpoint error for ${source}:`, err);
    res.status(500).json({ error: err.message, code: err.code || ErrorCode.UNKNOWN });
  }
});

// Get historical data
app.get('/api/history/:source', async (req, res) => {
  const { source } = req.params;
  const { duration = '24h', metric } = req.query;
  
  try {
    const history = await ringBuffer.getHistory(source, duration, metric);
    res.json({ source, duration, data: history });
  } catch (err) {
    logger.error('History endpoint error:', err);
    res.status(500).json({ error: err.message });
  }
});

// Get all available sources
app.get('/api/sources', (req, res) => {
  const sources = Object.keys(aggregators).map(name => ({
    name,
    enabled: aggregators[name].isEnabled(),
  }));
  res.json(sources);
});

// Error handling middleware
app.use((err, req, res, next) => {
  logger.error('Unhandled error:', err);
  
  const errorResponse = {
    error: err.message || 'Internal server error',
    code: err.code || ErrorCode.UNKNOWN,
    timestamp: Date.now(),
  };
  
  if (config.debugMode) {
    errorResponse.stack = err.stack;
  }
  
  res.status(err.status || 500).json(errorResponse);
});

// Data aggregation loop
async function runAggregationLoop() {
  const intervalMs = 5000; // 5 seconds
  
  while (true) {
    try {
      const startTime = Date.now();
      const aggregatedData = {};
      
      for (const [name, aggregator] of Object.entries(aggregators)) {
        if (!aggregator.isEnabled()) continue;
        
        const aggStart = Date.now();
        try {
          const data = await aggregator.getStatus();
          aggregatedData[name] = data;
          
          const aggDuration = Date.now() - aggStart;
          metrics.aggregationDuration.observe({ source: name }, aggDuration / 1000);
          
          // Store in ring buffer
          await ringBuffer.add({
            source: name,
            data,
            timestamp: Date.now(),
          });
          
          // Publish to MQTT
          if (mqttClient && mqttClient.connected) {
            mqttClient.publish(
              `homelab/${name}`,
              JSON.stringify(data),
              { retain: true }
            );
            metrics.mqttMessagesPublished.inc();
          }
          
          // Store in Redis for fast access
          if (redis) {
            await redis.setex(`homelab:${name}`, 60, JSON.stringify(data));
          }
          
          // Store in SQLite for persistence
          for (const [metricName, value] of Object.entries(flattenObject(data))) {
            if (typeof value === 'number') {
              db.prepare(`
                INSERT INTO metrics (source, metric_name, value, timestamp)
                VALUES (?, ?, ?, ?)
              `).run(name, metricName, value, Date.now());
            }
          }
          
        } catch (err) {
          logger.error(`Aggregation error for ${name}:`, err);
          db.prepare(`
            INSERT INTO errors (code, message, source, timestamp)
            VALUES (?, ?, ?, ?)
          `).run(err.code || ErrorCode.AGGREGATION_FAILED, err.message, name, Date.now());
        }
      }
      
      // Broadcast to WebSocket clients
      broadcastToWebSockets({
        type: 'update',
        timestamp: Date.now(),
        data: aggregatedData,
      });
      
      // Log duration
      const totalDuration = Date.now() - startTime;
      logger.debug(`Aggregation cycle completed in ${totalDuration}ms`);
      
      // Wait until next cycle
      const sleepTime = Math.max(0, intervalMs - totalDuration);
      await new Promise(resolve => setTimeout(resolve, sleepTime));
      
    } catch (err) {
      logger.error('Aggregation loop error:', err);
      await new Promise(resolve => setTimeout(resolve, 5000));
    }
  }
}

// Helper function to flatten nested objects
function flattenObject(obj, prefix = '') {
  return Object.keys(obj).reduce((acc, k) => {
    const pre = prefix.length ? prefix + '.' : '';
    if (typeof obj[k] === 'object' && obj[k] !== null && !Array.isArray(obj[k])) {
      Object.assign(acc, flattenObject(obj[k], pre + k));
    } else {
      acc[pre + k] = obj[k];
    }
    return acc;
  }, {});
}

// Graceful shutdown
process.on('SIGTERM', async () => {
  logger.info('SIGTERM received, shutting down gracefully');
  
  if (mqttClient) mqttClient.end();
  if (redis) await redis.quit();
  if (wsServer) wsServer.close();
  if (server) server.close();
  
  db.close();
  
  process.exit(0);
});

// Start server
async function start() {
  try {
    // Start HTTP server
    server.listen(config.apiPort, () => {
      logger.info(`HTTP API server running on port ${config.apiPort}`);
    });
    
    // Start aggregation loop
    runAggregationLoop();
    
    logger.info('Homelab Aggregator started successfully');
  } catch (err) {
    logger.error('Failed to start aggregator:', err);
    process.exit(1);
  }
}

start();
