/*
 * CYD Homelab Dashboard Firmware
 * 
 * Features:
 * - Widget-basiertes UI System
 * - Ring Buffer für 24-48h Historie
 * - WebSocket/MQTT Client für Backend-Kommunikation
 * - Touch-Steuerung
 * - Potentiometer Support (optional)
 * - Error Handling mit Codes
 * - Config Manager (JSON-basiert)
 */

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

// Includes aus unserem Include-Ordner
#include "Theme.h"
#include "Widget.h"
#include "RingBuffer.h"
#include "ErrorCodes.h"
#include "ConfigManager.h"
#include "PotiHandler.h"

// Globale Instanzen
TFT_eSPI tft = TFT_eSPI();
ConfigManager config;
PotiHandler poti;

// WebSocket Client
WebSocketsClient webSocket;

// Ring Buffers für verschiedene Metriken (48h @ 1min = 2880 Punkte)
FloatBuffer cpuBuffer(2880);
FloatBuffer memoryBuffer(2880);
FloatBuffer networkBuffer(2880);
FloatBuffer gpuBuffer(2880);

// Widgets
StatusIndicator* proxmoxIndicator;
StatusIndicator* dockerIndicator;
StatusIndicator* haIndicator;
GaugeWidget* cpuGauge;
GaugeWidget* memoryGauge;
ChartWidget* networkChart;
InfoCardWidget* infoCard;

// System Status
bool wifiConnected = false;
uint32_t lastDataUpdate = 0;
uint32_t frameCount = 0;
uint32_t lastFPSUpdate = 0;
float fps = 0;

// Forward Declarations
void initWiFi();
void initWebSocket();
void initWidgets();
void setupMode();
void updateDashboard();
void handleTouch();
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);

void setup() {
    Serial.begin(115200);
    LOG_INFO("CYD Homelab Dashboard starting...");
    
    // SPIFFS initialisieren
    if (!SPIFFS.begin(true)) {
        ErrorHandler::set(Errors::STORAGE_READ_FAIL);
        delay(1000);
    }
    
    // Config laden
    ErrorCode err = config.load();
    if (err.toUint() != Errors::OK.toUint()) {
        LOG_ERROR("Config load failed, using defaults");
    }
    
    // Display initialisieren
    tft.init();
    tft.setRotation(1);  // Landscape
    tft.fillScreen(Theme::BACKGROUND);
    
    // Helligkeit setzen
    ledcSetup(7, 5000, 8);
    ledcAttachPin(TFT_BL, 7);
    ledcWrite(7, config.getDisplay().brightness);
    
    // WiFi Verbindung
    if (config.isSetupMode()) {
        LOG_INFO("No SSID configured, entering Setup Mode");
        setupMode();
        return;
    }
    
    initWiFi();
    initWidgets();
    
    if (wifiConnected) {
        initWebSocket();
    }
    
    LOG_INFO("Setup complete!");
}

void loop() {
    if (wifiConnected && WiFi.status() != WL_CONNECTED) {
        LOG_WARN("WiFi lost, reconnecting...");
        wifiConnected = false;
        initWiFi();
    }
    
    if (wifiConnected) {
        webSocket.loop();
    }
    
    handleTouch();
    
    if (millis() - lastDataUpdate > config.getData().refreshIntervalMs) {
        updateDashboard();
        lastDataUpdate = millis();
    }
    
    frameCount++;
    if (millis() - lastFPSUpdate >= 1000) {
        fps = frameCount;
        frameCount = 0;
        lastFPSUpdate = millis();
    }
    
    delay(10);
}

void initWiFi() {
    const NetworkConfig& net = config.getNetwork();
    
    LOG_INFO("Connecting to WiFi...");
    
    WiFi.begin(net.ssid, net.password);
    WiFi.setHostname("CYD-Dashboard");
    
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 30) {
        delay(500);
        Serial.print(".");
        timeout++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        LOG_INFO("WiFi connected!");
        LOG_INFO(WiFi.localIP().toString().c_str());
    } else {
        wifiConnected = false;
        ErrorHandler::set(Errors::NET_WIFI_CONNECT);
        LOG_ERROR("WiFi connection failed!");
    }
}

void initWebSocket() {
    const NetworkConfig& net = config.getNetwork();
    
    String url = String(net.websocketUrl) + ":" + String(net.websocketPort) + "/ws";
    
    webSocket.begin(url.c_str());
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);
    
    LOG_INFO("WebSocket configured");
}

void initWidgets() {
    proxmoxIndicator = new StatusIndicator(10, 10, "Proxmox", Theme::PROXMOX_COLOR);
    dockerIndicator = new StatusIndicator(100, 10, "Docker", Theme::DOCKER_COLOR);
    haIndicator = new StatusIndicator(190, 10, "HA", Theme::HA_COLOR);
    
    cpuGauge = new GaugeWidget(10, 80, "CPU", 0, 100, Theme::PRIMARY);
    memoryGauge = new GaugeWidget(140, 80, "RAM", 0, 100, Theme::SECONDARY);
    
    networkChart = new ChartWidget(10, 210, 280, 100, "Network", 60, 0, 1000, Theme::ACCENT);
    
    infoCard = new InfoCardWidget(270, 80, 50, 230, "System", "Online", "");
}

void updateDashboard() {
    tft.fillScreen(Theme::BACKGROUND);
    
    proxmoxIndicator->draw(tft);
    dockerIndicator->draw(tft);
    haIndicator->draw(tft);
    cpuGauge->draw(tft);
    memoryGauge->draw(tft);
    networkChart->draw(tft);
    infoCard->draw(tft);
    
    static float simCPU = 50;
    static float simMem = 60;
    
    simCPU += (random(-5, 5));
    simCPU = constrain(simCPU, 0, 100);
    simMem += (random(-3, 3));
    simMem = constrain(simMem, 0, 100);
    
    cpuGauge->setValue(simCPU);
    memoryGauge->setValue(simMem);
    
    networkChart->addValue(random(100, 500));
}

void handleTouch() {
    // CYD Touchscreen implementation
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            LOG_WARN("WebSocket disconnected");
            break;
        case WStype_CONNECTED:
            LOG_INFO("WebSocket connected");
            break;
        case WStype_TEXT:
            {
                StaticJsonDocument<1024> doc;
                DeserializationError error = deserializeJson(doc, payload, length);
                
                if (!error) {
                    if (doc.containsKey("cpu")) {
                        cpuGauge->setValue(doc["cpu"]);
                        cpuBuffer.push(doc["cpu"]);
                    }
                    if (doc.containsKey("memory")) {
                        memoryGauge->setValue(doc["memory"]);
                        memoryBuffer.push(doc["memory"]);
                    }
                    if (doc.containsKey("proxmox_status")) {
                        proxmoxIndicator->setStatus(doc["proxmox_status"]);
                    }
                    if (doc.containsKey("docker_status")) {
                        dockerIndicator->setStatus(doc["docker_status"]);
                    }
                    if (doc.containsKey("ha_status")) {
                        haIndicator->setStatus(doc["ha_status"]);
                    }
                } else {
                    ErrorHandler::set(Errors::COMM_INVALID_DATA);
                }
            }
            break;
        case WStype_ERROR:
            ErrorHandler::set(Errors::NET_WEBSOCKET_FAIL);
            break;
        default:
            break;
    }
}

void setupMode() {
    LOG_INFO("Starting Access Point: CYD-Setup");
    
    WiFi.softAP("CYD-Setup", "homelab123");
    
    tft.fillScreen(Theme::BACKGROUND);
    tft.setTextColor(Theme::TEXT_PRIMARY, Theme::BACKGROUND);
    tft.setTextSize(Fonts::MEDIUM);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("CYD Dashboard", 160, 100);
    
    tft.setTextSize(Fonts::SMALL);
    tft.setTextColor(Theme::TEXT_SECONDARY, Theme::BACKGROUND);
    tft.drawString("Connect to: CYD-Setup", 160, 140);
    tft.drawString("Password: homelab123", 160, 160);
    tft.drawString("IP: 192.168.4.1", 160, 180);
    
    while (true) {
        delay(1000);
        yield();
    }
}
