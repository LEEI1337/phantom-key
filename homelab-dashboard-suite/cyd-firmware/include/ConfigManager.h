#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include "ErrorCodes.h"

// Konfigurationsstruktur für alle einstellbaren Parameter
// Keine fixen Werte im Code - alles über Config!

struct NetworkConfig {
    char ssid[32];
    char password[64];
    char mqttBroker[64];
    int mqttPort;
    char mqttUser[32];
    char mqttPassword[64];
    char websocketUrl[128];
    int websocketPort;
    bool useStaticIP;
    char staticIP[16];
    char gateway[16];
    char subnet[16];
    char dns[16];
};

struct DisplayConfig {
    int brightness;
    int timeoutSeconds;
    bool autoRotate;
    int defaultScreen;
    bool showFPS;
    String theme;
};

struct DataConfig {
    int refreshIntervalMs;
    int historySize;  // Anzahl der Punkte im Ring Buffer
    bool logToSD;
    int aggregationInterval;  // Sekunden für Daten-Aggregation
};

struct ServiceConfig {
    bool proxmoxEnabled;
    char proxmoxUrl[128];
    char proxmoxToken[64];
    
    bool dockerEnabled;
    char dockerHost[64];
    int dockerPort;
    
    bool omvEnabled;
    char omvUrl[128];
    
    bool pfsenseEnabled;
    char pfsenseUrl[128];
    char pfsenseUser[32];
    char pfsensePassword[64];
    
    bool gpuEnabled;
    int gpuIndex;
    
    bool homeAssistantEnabled;
    char haUrl[128];
    char haToken[64];
};

struct PotiConfig {
    int pin;
    int mode;  // 0=Volume, 1=Light, 2=Scroll, 3=Custom
    float smoothingFactor;  // 0.0-1.0 für Exponential Moving Average
    int steps;
    bool inverted;
    char customAction[64];
};

class ConfigManager {
private:
    NetworkConfig network;
    DisplayConfig display;
    DataConfig data;
    ServiceConfig services;
    PotiConfig poti;
    
    const char* configFile = "/config.json";
    
public:
    ConfigManager() {
        setDefaults();
    }
    
    void setDefaults() {
        // Network Defaults
        strcpy(network.ssid, "");
        strcpy(network.password, "");
        strcpy(network.mqttBroker, "192.168.1.100");
        network.mqttPort = 1883;
        strcpy(network.mqttUser, "");
        strcpy(network.mqttPassword, "");
        strcpy(network.websocketUrl, "ws://192.168.1.100");
        network.websocketPort = 8765;
        network.useStaticIP = false;
        
        // Display Defaults
        display.brightness = 200;
        display.timeoutSeconds = 300;
        display.autoRotate = false;
        display.defaultScreen = 0;
        display.showFPS = false;
        display.theme = "dark";
        
        // Data Defaults
        data.refreshIntervalMs = 1000;
        data.historySize = 2880;  // 48h bei 1 Min Intervall
        data.logToSD = false;
        data.aggregationInterval = 60;
        
        // Services Defaults
        services.proxmoxEnabled = false;
        services.dockerEnabled = true;
        services.omvEnabled = false;
        services.pfsenseEnabled = false;
        services.gpuEnabled = true;
        services.homeAssistantEnabled = true;
        
        // Poti Defaults
        poti.pin = -1;  // Wird zur Laufzeit gesetzt
        poti.mode = 1;  // Light dimming
        poti.smoothingFactor = 0.1;
        poti.steps = 100;
        poti.inverted = false;
    }
    
    ErrorCode load() {
        if (!SPIFFS.begin(true)) {
            ErrorHandler::set(Errors::STORAGE_READ_FAIL);
            return Errors::STORAGE_READ_FAIL;
        }
        
        if (!SPIFFS.exists(configFile)) {
            LOG_INFO("Config file not found, using defaults");
            save();  // Speichere Defaults
            return Errors::OK;
        }
        
        File file = SPIFFS.open(configFile, "r");
        if (!file) {
            ErrorHandler::set(Errors::STORAGE_READ_FAIL);
            return Errors::STORAGE_READ_FAIL;
        }
        
        StaticJsonDocument<2048> doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();
        
        if (error) {
            LOG_ERROR("JSON deserialization failed");
            ErrorHandler::set(Errors::CONFIG_INVALID);
            return Errors::CONFIG_INVALID;
        }
        
        // Parse Network
        if (doc.containsKey("network")) {
            JsonObject net = doc["network"];
            if (net.containsKey("ssid")) strncpy(network.ssid, net["ssid"], sizeof(network.ssid));
            if (net.containsKey("password")) strncpy(network.password, net["password"], sizeof(network.password));
            if (net.containsKey("mqtt_broker")) strncpy(network.mqttBroker, net["mqtt_broker"], sizeof(network.mqttBroker));
            if (net.containsKey("mqtt_port")) network.mqttPort = net["mqtt_port"];
            if (net.containsKey("websocket_url")) strncpy(network.websocketUrl, net["websocket_url"], sizeof(network.websocketUrl));
            if (net.containsKey("websocket_port")) network.websocketPort = net["websocket_port"];
        }
        
        // Parse Display
        if (doc.containsKey("display")) {
            JsonObject disp = doc["display"];
            if (disp.containsKey("brightness")) display.brightness = disp["brightness"];
            if (disp.containsKey("timeout")) display.timeoutSeconds = disp["timeout"];
            if (disp.containsKey("theme")) display.theme = disp["theme"].as<String>();
        }
        
        // Parse Data
        if (doc.containsKey("data")) {
            JsonObject d = doc["data"];
            if (d.containsKey("refresh_interval")) data.refreshIntervalMs = d["refresh_interval"];
            if (d.containsKey("history_size")) data.historySize = d["history_size"];
        }
        
        // Parse Services
        if (doc.containsKey("services")) {
            JsonObject svc = doc["services"];
            if (svc.containsKey("proxmox_enabled")) services.proxmoxEnabled = svc["proxmox_enabled"];
            if (svc.containsKey("docker_enabled")) services.dockerEnabled = svc["docker_enabled"];
            if (svc.containsKey("ha_enabled")) services.homeAssistantEnabled = svc["ha_enabled"];
            if (svc.containsKey("gpu_enabled")) services.gpuEnabled = svc["gpu_enabled"];
        }
        
        // Parse Poti
        if (doc.containsKey("poti")) {
            JsonObject p = doc["poti"];
            if (p.containsKey("mode")) poti.mode = p["mode"];
            if (p.containsKey("smoothing")) poti.smoothingFactor = p["smoothing"];
            if (p.containsKey("inverted")) poti.inverted = p["inverted"];
        }
        
        LOG_INFO("Config loaded successfully");
        return Errors::OK;
    }
    
    ErrorCode save() {
        if (!SPIFFS.begin(true)) {
            ErrorHandler::set(Errors::STORAGE_WRITE_FAIL);
            return Errors::STORAGE_WRITE_FAIL;
        }
        
        File file = SPIFFS.open(configFile, "w");
        if (!file) {
            ErrorHandler::set(Errors::STORAGE_WRITE_FAIL);
            return Errors::STORAGE_WRITE_FAIL;
        }
        
        StaticJsonDocument<2048> doc;
        
        // Network
        JsonObject net = doc.createNestedObject("network");
        net["ssid"] = network.ssid;
        net["password"] = network.password;
        net["mqtt_broker"] = network.mqttBroker;
        net["mqtt_port"] = network.mqttPort;
        net["websocket_url"] = network.websocketUrl;
        net["websocket_port"] = network.websocketPort;
        
        // Display
        JsonObject disp = doc.createNestedObject("display");
        disp["brightness"] = display.brightness;
        disp["timeout"] = display.timeoutSeconds;
        disp["theme"] = display.theme;
        
        // Data
        JsonObject d = doc.createNestedObject("data");
        d["refresh_interval"] = data.refreshIntervalMs;
        d["history_size"] = data.historySize;
        
        // Services
        JsonObject svc = doc.createNestedObject("services");
        svc["proxmox_enabled"] = services.proxmoxEnabled;
        svc["docker_enabled"] = services.dockerEnabled;
        svc["ha_enabled"] = services.homeAssistantEnabled;
        svc["gpu_enabled"] = services.gpuEnabled;
        
        // Poti
        JsonObject p = doc.createNestedObject("poti");
        p["mode"] = poti.mode;
        p["smoothing"] = poti.smoothingFactor;
        p["inverted"] = poti.inverted;
        
        if (serializeJson(doc, file) == 0) {
            file.close();
            ErrorHandler::set(Errors::CONFIG_SAVE_FAILED);
            return Errors::CONFIG_SAVE_FAILED;
        }
        
        file.close();
        LOG_INFO("Config saved successfully");
        return Errors::OK;
    }
    
    // Getters
    const NetworkConfig& getNetwork() const { return network; }
    const DisplayConfig& getDisplay() const { return display; }
    const DataConfig& getData() const { return data; }
    const ServiceConfig& getServices() const { return services; }
    const PotiConfig& getPoti() const { return poti; }
    
    // Setters mit Auto-Save
    void setBrightness(int val) { 
        display.brightness = constrain(val, 0, 255); 
        save();
    }
    
    void setPotiMode(int mode) {
        poti.mode = constrain(mode, 0, 3);
        save();
    }
    
    // Setup Mode aktivieren (AP Mode für Erstkonfiguration)
    bool isSetupMode() {
        return strlen(network.ssid) == 0;
    }
};

#endif // CONFIG_MANAGER_H
