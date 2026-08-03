/**
 * Alert Engine Service
 * Überwacht Schwellenwerte und generiert Alerts für CYD, ESP-Key und Mobile
 */

const EventEmitter = require('events');
const logger = require('../utils/logger');
const { ERROR_CODES } = require('../utils/errorCodes');

class AlertEngine extends EventEmitter {
  constructor(config) {
    super();
    this.config = config;
    this.alertRules = this.loadAlertRules();
    this.activeAlerts = new Map();
    this.alertHistory = [];
    this.maxHistory = 100;
    
    logger.info('Alert Engine initialized', { rulesCount: this.alertRules.length });
  }

  loadAlertRules() {
    // Standard Rules - können via Config erweitert werden
    return [
      {
        id: 'cpu_high',
        metric: 'cpu_usage',
        condition: (value) => value > 80,
        severity: 'warning',
        message: 'CPU Auslastung kritisch: {value}%',
        cooldown: 300000, // 5 min
        sources: ['proxmox', 'system']
      },
      {
        id: 'gpu_temp_high',
        metric: 'gpu_temperature',
        condition: (value) => value > 85,
        severity: 'critical',
        message: 'GPU Temperatur zu hoch: {value}°C',
        cooldown: 60000, // 1 min
        sources: ['gpu']
      },
      {
        id: 'container_down',
        metric: 'container_status',
        condition: (value, meta) => value === 'exited' || value === 'dead',
        severity: 'critical',
        message: 'Container {name} ist ausgefallen',
        cooldown: 0, // Sofort bei jedem Auftreten
        sources: ['docker']
      },
      {
        id: 'disk_space_low',
        metric: 'disk_usage',
        condition: (value) => value > 90,
        severity: 'warning',
        message: 'Festplatte fast voll: {value}%',
        cooldown: 600000, // 10 min
        sources: ['omv', 'system']
      },
      {
        id: 'network_attack',
        metric: 'firewall_blocks',
        condition: (value, meta) => value > 100, // Mehr als 100 Blocks in der Minute
        severity: 'critical',
        message: 'Möglicher Netzwerkangriff erkannt: {value} Blocks/min',
        cooldown: 120000, // 2 min
        sources: ['pfsense']
      },
      {
        id: 'ram_high',
        metric: 'ram_usage',
        condition: (value) => value > 90,
        severity: 'warning',
        message: 'RAM Auslastung kritisch: {value}%',
        cooldown: 300000,
        sources: ['proxmox', 'system']
      },
      {
        id: 'service_unreachable',
        metric: 'service_ping',
        condition: (value) => value === false,
        severity: 'critical',
        message: 'Service {name} nicht erreichbar',
        cooldown: 60000,
        sources: ['homeassistant', 'docker']
      }
    ];
  }

  /**
   * Verarbeitet eingehende Metriken und prüft gegen Regeln
   */
  processMetric(source, metricName, value, metadata = {}) {
    const relevantRules = this.alertRules.filter(
      rule => rule.sources.includes(source) && rule.metric === metricName
    );

    for (const rule of relevantRules) {
      this.checkRule(rule, value, metadata, source);
    }
  }

  checkRule(rule, value, metadata, source) {
    const now = Date.now();
    const lastAlert = this.activeAlerts.get(rule.id);
    
    // Cooldown prüfen
    if (lastAlert && (now - lastAlert.timestamp) < rule.cooldown) {
      return;
    }

    // Bedingung prüfen
    if (rule.condition(value, metadata)) {
      const alert = {
        id: rule.id,
        severity: rule.severity,
        message: this.formatMessage(rule.message, value, metadata),
        timestamp: now,
        source,
        metric: rule.metric,
        value,
        metadata,
        acknowledged: false
      };

      // Neuen Alert speichern
      this.activeAlerts.set(rule.id, alert);
      this.addToHistory(alert);
      
      // Event emittieren
      this.emit('alert', alert);
      
      logger.warn(`ALERT [${rule.severity.toUpperCase()}]: ${alert.message}`, {
        alertId: rule.id,
        source,
        value
      });

      // An verbundene Clients senden (WebSocket)
      this.broadcastAlert(alert);
    } else {
      // Alert zurücksetzen wenn Bedingung nicht mehr erfüllt
      if (this.activeAlerts.has(rule.id)) {
        const clearedAlert = {
          id: rule.id,
          type: 'clear',
          timestamp: now,
          message: `Alert ${rule.id} behoben`
        };
        
        this.activeAlerts.delete(rule.id);
        this.emit('alert_cleared', clearedAlert);
        this.broadcastAlert(clearedAlert);
        
        logger.info(`Alert cleared: ${rule.id}`);
      }
    }
  }

  formatMessage(template, value, metadata) {
    return template.replace(/{(\w+)}/g, (match, key) => {
      if (key === 'value') return typeof value === 'number' ? value.toFixed(1) : value;
      return metadata[key] || match;
    });
  }

  addToHistory(alert) {
    this.alertHistory.unshift(alert);
    if (this.alertHistory.length > this.maxHistory) {
      this.alertHistory.pop();
    }
  }

  broadcastAlert(alert) {
    // Wird vom WebSocket Handler abonniert
    // Emit an den globalen Event Bus
    this.emit('broadcast', {
      type: 'alert',
      payload: alert
    });
  }

  /**
   * Acknowledge einen Alert (z.B. via Touch auf CYD)
   */
  acknowledgeAlert(alertId) {
    if (this.activeAlerts.has(alertId)) {
      const alert = this.activeAlerts.get(alertId);
      alert.acknowledged = true;
      alert.acknowledgedAt = Date.now();
      
      this.emit('alert_acknowledged', { alertId, timestamp: Date.now() });
      this.broadcastAlert({ type: 'acknowledged', alertId });
      
      logger.info(`Alert acknowledged: ${alertId}`);
      return true;
    }
    return false;
  }

  /**
   * Hole alle aktiven Alerts
   */
  getActiveAlerts() {
    return Array.from(this.activeAlerts.values());
  }

  /**
   * Hole Alert Historie
   */
  getHistory(limit = 20) {
    return this.alertHistory.slice(0, limit);
  }

  /**
   * Füge benutzerdefinierte Regel hinzu
   */
  addRule(rule) {
    if (!rule.id || !rule.metric || !rule.condition) {
      throw new Error('Ungültige Regel: id, metric und condition sind erforderlich');
    }
    
    const existingIndex = this.alertRules.findIndex(r => r.id === rule.id);
    if (existingIndex >= 0) {
      this.alertRules[existingIndex] = { ...this.alertRules[existingIndex], ...rule };
    } else {
      this.alertRules.push(rule);
    }
    
    logger.info('Alert rule added/updated', { ruleId: rule.id });
    return true;
  }

  /**
   * Entferne Regel
   */
  removeRule(ruleId) {
    const index = this.alertRules.findIndex(r => r.id === ruleId);
    if (index >= 0) {
      this.alertRules.splice(index, 1);
      logger.info('Alert rule removed', { ruleId });
      return true;
    }
    return false;
  }
}

module.exports = AlertEngine;
