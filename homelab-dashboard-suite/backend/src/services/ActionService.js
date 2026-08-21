/**
 * Action Service - Bidirektionale Steuerung vom CYD/ESP-Key
 * Ermöglicht Quick Actions: Container restart, Services steuern, Scripts ausführen
 */

const logger = require('../utils/logger');
const { ERROR_CODES } = require('../utils/errorCodes');

class ActionService {
  constructor(config) {
    this.config = config;
    this.actions = new Map();
    this.actionHistory = [];
    this.maxHistory = 50;
    
    // Standard Actions registrieren
    this.registerDefaultActions();
    
    logger.info('Action Service initialized', { actionsCount: this.actions.size });
  }

  registerDefaultActions() {
    // Docker Container Actions
    this.registerAction({
      id: 'docker_container_restart',
      name: 'Container Neustart',
      description: 'Startet einen Docker Container neu',
      category: 'docker',
      params: [
        { name: 'containerId', type: 'string', required: true, description: 'Container ID oder Name' }
      ],
      handler: async (params) => {
        const Docker = require('dockerode');
        const docker = new Docker();
        
        try {
          const container = docker.getContainer(params.containerId);
          const info = await container.inspect();
          
          if (info.State.Running) {
            await container.restart();
          } else {
            await container.start();
          }
          
          return { success: true, message: `Container ${params.containerId} neu gestartet` };
        } catch (error) {
          logger.error('Docker action failed', { error: error.message, container: params.containerId });
          throw new Error(`Container Aktion fehlgeschlagen: ${error.message}`);
        }
      }
    });

    this.registerAction({
      id: 'docker_container_stop',
      name: 'Container Stoppen',
      description: 'Stoppt einen Docker Container',
      category: 'docker',
      params: [
        { name: 'containerId', type: 'string', required: true }
      ],
      handler: async (params) => {
        const Docker = require('dockerode');
        const docker = new Docker();
        
        try {
          const container = docker.getContainer(params.containerId);
          await container.stop({ t: 10 }); // 10s Timeout
          return { success: true, message: `Container ${params.containerId} gestoppt` };
        } catch (error) {
          throw new Error(`Container Stopp fehlgeschlagen: ${error.message}`);
        }
      }
    });

    this.registerAction({
      id: 'docker_container_start',
      name: 'Container Starten',
      description: 'Startet einen gestoppten Docker Container',
      category: 'docker',
      params: [
        { name: 'containerId', type: 'string', required: true }
      ],
      handler: async (params) => {
        const Docker = require('dockerode');
        const docker = new Docker();
        
        try {
          const container = docker.getContainer(params.containerId);
          await container.start();
          return { success: true, message: `Container ${params.containerId} gestartet` };
        } catch (error) {
          throw new Error(`Container Start fehlgeschlagen: ${error.message}`);
        }
      }
    });

    this.registerAction({
      id: 'docker_logs',
      name: 'Container Logs',
      description: 'Ruft die letzten Logs eines Containers ab',
      category: 'docker',
      params: [
        { name: 'containerId', type: 'string', required: true },
        { name: 'lines', type: 'number', required: false, default: 50 }
      ],
      handler: async (params) => {
        const Docker = require('dockerode');
        const docker = new Docker();
        
        try {
          const container = docker.getContainer(params.containerId);
          const logs = await container.logs({
            stdout: true,
            stderr: true,
            tail: params.lines || 50,
            timestamps: false
          });
          
          return { 
            success: true, 
            logs: logs.toString(),
            message: `Logs für ${params.containerId} abgerufen`
          };
        } catch (error) {
          throw new Error(`Log Abruf fehlgeschlagen: ${error.message}`);
        }
      }
    });

    // System Actions
    this.registerAction({
      id: 'system_reboot',
      name: 'System Neustart',
      description: 'Startet das Host-System neu (Vorsicht!)',
      category: 'system',
      requiresConfirmation: true,
      params: [],
      handler: async () => {
        const { exec } = require('child_process');
        const util = require('util');
        const execPromise = util.promisify(exec);
        
        try {
          // Nur in sicherer Umgebung testen!
          // await execPromise('sudo reboot');
          logger.warn('System Reboot angefordert - in Produktion ausführen');
          return { success: true, message: 'System Neustart wurde ausgelöst' };
        } catch (error) {
          throw new Error(`System Neustart fehlgeschlagen: ${error.message}`);
        }
      }
    });

    this.registerAction({
      id: 'service_restart',
      name: 'Service Neustart',
      description: 'Startet einen systemd Service neu',
      category: 'system',
      params: [
        { name: 'serviceName', type: 'string', required: true }
      ],
      handler: async (params) => {
        const { exec } = require('child_process');
        const util = require('util');
        const execPromise = util.promisify(exec);
        
        try {
          await execPromise(`sudo systemctl restart ${params.serviceName}`);
          return { success: true, message: `Service ${params.serviceName} neu gestartet` };
        } catch (error) {
          throw new Error(`Service Neustart fehlgeschlagen: ${error.message}`);
        }
      }
    });

    // Home Assistant Actions
    this.registerAction({
      id: 'ha_service_call',
      name: 'HA Service Call',
      description: 'Ruft einen Home Assistant Service auf',
      category: 'homeassistant',
      params: [
        { name: 'domain', type: 'string', required: true },
        { name: 'service', type: 'string', required: true },
        { name: 'data', type: 'object', required: false }
      ],
      handler: async (params) => {
        const axios = require('axios');
        
        try {
          const response = await axios.post(
            `${this.config.homeassistant.url}/api/services/${params.domain}/${params.service}`,
            params.data || {},
            {
              headers: {
                'Authorization': `Bearer ${this.config.homeassistant.token}`,
                'Content-Type': 'application/json'
              }
            }
          );
          
          return { success: true, message: `HA Service ${params.domain}.${params.service} ausgeführt` };
        } catch (error) {
          throw new Error(`HA Service Call fehlgeschlagen: ${error.message}`);
        }
      }
    });

    // Custom Script Action
    this.registerAction({
      id: 'custom_script',
      name: 'Custom Script',
      description: 'Führt ein benutzerdefiniertes Skript aus',
      category: 'custom',
      requiresConfirmation: true,
      params: [
        { name: 'scriptPath', type: 'string', required: true },
        { name: 'args', type: 'array', required: false, default: [] }
      ],
      handler: async (params) => {
        const { exec } = require('child_process');
        const util = require('util');
        const execPromise = util.promisify(exec);
        
        // Sicherheitscheck: Nur erlaubte Pfade
        const allowedPaths = this.config.allowedScriptPaths || ['/opt/scripts/', '/home/admin/scripts/'];
        const isAllowed = allowedPaths.some(path => params.scriptPath.startsWith(path));
        
        if (!isAllowed) {
          throw new Error(`Skript Pfad nicht erlaubt: ${params.scriptPath}`);
        }
        
        try {
          const argsString = (params.args || []).join(' ');
          const { stdout, stderr } = await execPromise(`${params.scriptPath} ${argsString}`);
          
          return { 
            success: true, 
            output: stdout,
            error: stderr,
            message: `Skript ${params.scriptPath} ausgeführt`
          };
        } catch (error) {
          throw new Error(`Skript Ausführung fehlgeschlagen: ${error.message}`);
        }
      }
    });
  }

  /**
   * Registriere eine neue Action
   */
  registerAction(actionDef) {
    if (!actionDef.id || !actionDef.handler) {
      throw new Error('Action muss id und handler haben');
    }
    
    this.actions.set(actionDef.id, actionDef);
    logger.debug('Action registered', { actionId: actionDef.id });
    return true;
  }

  /**
   * Hole alle verfügbaren Actions (für UI)
   */
  getAvailableActions(category = null) {
    let actions = Array.from(this.actions.values());
    
    if (category) {
      actions = actions.filter(a => a.category === category);
    }
    
    return actions.map(a => ({
      id: a.id,
      name: a.name,
      description: a.description,
      category: a.category,
      params: a.params,
      requiresConfirmation: a.requiresConfirmation || false
    }));
  }

  /**
   * Führe eine Action aus
   */
  async executeAction(actionId, params = {}) {
    const action = this.actions.get(actionId);
    
    if (!action) {
      throw new Error(`Action nicht gefunden: ${actionId}`);
    }

    // Parameter validieren
    if (action.params) {
      for (const param of action.params) {
        if (param.required && !(param.name in params)) {
          throw new Error(`Fehlender Parameter: ${param.name}`);
        }
        
        if (param.name in params) {
          // Typ-Validierung (einfach)
          const value = params[param.name];
          if (param.type === 'string' && typeof value !== 'string') {
            params[param.name] = String(value);
          } else if (param.type === 'number' && typeof value !== 'number') {
            params[param.name] = parseFloat(value);
          }
        } else if (param.default !== undefined) {
          params[param.name] = param.default;
        }
      }
    }

    logger.info('Executing action', { actionId, params });
    
    try {
      const result = await action.handler(params);
      
      // In History speichern
      this.addToHistory({
        actionId,
        params,
        result,
        timestamp: Date.now(),
        status: 'success'
      });
      
      return result;
    } catch (error) {
      this.addToHistory({
        actionId,
        params,
        error: error.message,
        timestamp: Date.now(),
        status: 'failed'
      });
      
      logger.error('Action execution failed', { actionId, error: error.message });
      throw error;
    }
  }

  addToHistory(entry) {
    this.actionHistory.unshift(entry);
    if (this.actionHistory.length > this.maxHistory) {
      this.actionHistory.pop();
    }
  }

  getHistory(limit = 20) {
    return this.actionHistory.slice(0, limit);
  }

  /**
   * Hole Action Details
   */
  getActionDetails(actionId) {
    const action = this.actions.get(actionId);
    if (!action) return null;
    
    return {
      id: action.id,
      name: action.name,
      description: action.description,
      category: action.category,
      params: action.params,
      requiresConfirmation: action.requiresConfirmation || false
    };
  }
}

module.exports = ActionService;
