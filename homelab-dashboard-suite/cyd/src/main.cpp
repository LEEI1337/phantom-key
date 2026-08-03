/**
 * CYD (Cheap Yellow Display) - Homelab Dashboard
 * 
 * Hauptdatei für das CYD Dashboard mit Widget-System
 */

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

// Configuration
#include "config.h"

// UI Components
#include "ui/Theme.h"
#include "ui/Widget.h"
#include "ui/Screen.h"

// Communication
#include "comms/WebSocketHandler.h"
#include "comms/MQTTHandler.h"
#include "comms/WiFiManager.h"

// Storage
#include "storage/RingBuffer.h"

// Widgets
#include "widgets/CPUWidget.h"
#include "widgets/MemoryWidget.h"
#include "widgets/NetworkWidget.h"
#include "widgets/GPUWidget.h"

// Global instances
static LGFX lcd;
static WiFiManager wifiManager;
static WebSocketHandler wsHandler;
static MQTTHandler mqttHandler;
static RingBuffer ringBuffer(2 * 60 * 60 * 1000); // 2 hours in milliseconds

// Screen management
static uint8_t currentScreen = 0;
static const uint8_t MAX_SCREENS = 4;

// Last update time
static unsigned long lastUpdate = 0;
static const unsigned long UPDATE_INTERVAL = 5000; // 5 seconds

// Touch calibration
static const uint16_t TOUCH_MIN_X = 200;
static const uint16_t TOUCH_MAX_X = 3700;
static const uint16_t TOUCH_MIN_Y = 250;
static const uint16_t TOUCH_MAX_Y = 3750;

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== CYD Homelab Dashboard ===");
  
  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialization failed!");
  }
  
  // Load configuration
  Config::load();
  
  // Initialize LCD
  lcd.init();
  lcd.setRotation(1);
  lcd.setBrightness(200);
  lcd.fillScreen(TFT_BLACK);
  
  // Show boot screen
  lcd.setTextSize(2);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setCursor(20, 100);
  lcd.print("Homelab Dashboard");
  lcd.setTextSize(1);
  lcd.setCursor(20, 130);
  lcd.print("Connecting...");
  
  // Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(Config::wifiSSID);
  
  WiFi.begin(Config::wifiSSID, Config::wifiPassword);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    lcd.fillRect(0, 150, 240, 20, TFT_GREEN);
    lcd.setTextColor(TFT_BLACK);
    lcd.setCursor(20, 155);
    lcd.print("WiFi Connected");
  } else {
    Serial.println("\nWiFi connection failed!");
    lcd.fillRect(0, 150, 240, 20, TFT_RED);
    lcd.setTextColor(TFT_WHITE);
    lcd.setCursor(20, 155);
    lcd.print("WiFi Failed");
  }
  
  // Initialize WebSocket
  wsHandler.begin(Config::backendWSUrl, onWebSocketMessage);
  
  // Initialize MQTT
  mqttHandler.begin(Config::mqttBroker, "cyd-dashboard", onMQTTMessage);
  
  // Initialize ring buffer from SPIFFS
  ringBuffer.load("/history.dat");
  
  delay(2000);
  
  // Clear screen and show main dashboard
  lcd.fillScreen(TFT_BLACK);
  drawDashboard();
}

void loop() {
  // Handle WiFi reconnection
  wifiManager.loop();
  
  // Handle WebSocket
  wsHandler.loop();
  
  // Handle MQTT
  mqttHandler.loop();
  
  // Periodic data update
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = millis();
    updateData();
  }
  
  // Handle touch input
  handleTouch();
  
  delay(10);
}

void drawDashboard() {
  lcd.fillScreen(TFT_BLACK);
  
  // Draw header
  lcd.fillRect(0, 0, 240, 25, Theme::primaryColor);
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(1);
  lcd.setCursor(10, 8);
  lcd.print("Homelab Dashboard");
  
  // Draw screen indicator
  lcd.setCursor(200, 8);
  lcd.printf("%d/%d", currentScreen + 1, MAX_SCREENS);
  
  // Draw widgets based on current screen
  switch (currentScreen) {
    case 0:
      drawCPUWidget();
      drawMemoryWidget();
      break;
    case 1:
      drawNetworkWidget();
      drawGPUWidget();
      break;
    case 2:
      drawProxmoxWidget();
      drawDockerWidget();
      break;
    case 3:
      drawHAWidget();
      drawSystemWidget();
      break;
  }
  
  // Draw navigation hint
  lcd.setTextColor(Theme::secondaryColor);
  lcd.setCursor(10, 305);
  lcd.print("< Swipe >");
}

void drawCPUWidget() {
  // Widget frame
  lcd.drawRect(10, 35, 220, 100, Theme::borderColor);
  
  // Title
  lcd.setTextColor(Theme::accentColor);
  lcd.setCursor(20, 45);
  lcd.print("CPU Usage");
  
  // Value (will be updated)
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(2);
  lcd.setCursor(20, 80);
  lcd.print("-- %");
}

void drawMemoryWidget() {
  // Widget frame
  lcd.drawRect(10, 145, 220, 100, Theme::borderColor);
  
  // Title
  lcd.setTextColor(Theme::accentColor);
  lcd.setCursor(20, 155);
  lcd.print("Memory");
  
  // Value
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(2);
  lcd.setCursor(20, 190);
  lcd.print("-- / -- MB");
}

void drawNetworkWidget() {
  lcd.drawRect(10, 35, 220, 210, Theme::borderColor);
  lcd.setTextColor(Theme::accentColor);
  lcd.setCursor(20, 45);
  lcd.print("Network Traffic");
}

void drawGPUWidget() {
  lcd.drawRect(10, 35, 220, 100, Theme::borderColor);
  lcd.setTextColor(Theme::accentColor);
  lcd.setCursor(20, 45);
  lcd.print("GPU");
}

void drawProxmoxWidget() {
  lcd.drawRect(10, 35, 220, 100, Theme::borderColor);
  lcd.setTextColor(Theme::accentColor);
  lcd.setCursor(20, 45);
  lcd.print("Proxmox");
}

void drawDockerWidget() {
  lcd.drawRect(10, 145, 220, 100, Theme::borderColor);
  lcd.setTextColor(Theme::accentColor);
  lcd.setCursor(20, 155);
  lcd.print("Docker");
}

void drawHAWidget() {
  lcd.drawRect(10, 35, 220, 100, Theme::borderColor);
  lcd.setTextColor(Theme::accentColor);
  lcd.setCursor(20, 45);
  lcd.print("Home Assistant");
}

void drawSystemWidget() {
  lcd.drawRect(10, 145, 220, 100, Theme::borderColor);
  lcd.setTextColor(Theme::accentColor);
  lcd.setCursor(20, 155);
  lcd.print("System");
}

void updateData() {
  // Request data from backend via WebSocket
  StaticJsonDocument<64> doc;
  doc["command"] = "get_status";
  
  String output;
  serializeJson(doc, output);
  wsHandler.send(output);
}

void onWebSocketMessage(const String& payload) {
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, payload);
  
  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    return;
  }
  
  // Store in ring buffer
  ringBuffer.add(payload.c_str(), millis());
  
  // Update display based on data
  if (doc.containsKey("proxmox")) {
    updateProxmoxDisplay(doc["proxmox"]);
  }
  
  if (doc.containsKey("docker")) {
    updateDockerDisplay(doc["docker"]);
  }
  
  if (doc.containsKey("gpu")) {
    updateGPUDisplay(doc["gpu"]);
  }
}

void onMQTTMessage(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("MQTT [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);
  
  // Process MQTT message
  ringBuffer.add(message.c_str(), millis());
}

void handleTouch() {
  uint16_t x, y;
  if (lcd.getTouch(&x, &y)) {
    // Simple swipe detection
    static uint16_t lastX = 0;
    static unsigned long lastTouch = 0;
    
    if (millis() - lastTouch < 500) {
      // Too fast, ignore
      return;
    }
    
    if (lastX > 0) {
      int16_t diff = (int16_t)x - (int16_t)lastX;
      
      if (abs(diff) > 50) {
        if (diff < 0) {
          // Swipe left - next screen
          currentScreen = (currentScreen + 1) % MAX_SCREENS;
        } else {
          // Swipe right - previous screen
          currentScreen = (currentScreen + MAX_SCREENS - 1) % MAX_SCREENS;
        }
        
        drawDashboard();
      }
    }
    
    lastX = x;
    lastTouch = millis();
  }
}

void updateProxmoxDisplay(JsonVariant data) {
  // Update Proxmox widget with actual data
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(1);
  lcd.setCursor(20, 70);
  
  if (data.containsKey("node") && data["node"].containsKey("cpu")) {
    int cpuUsage = data["node"]["cpu"]["usage"];
    lcd.printf("CPU: %d%%", cpuUsage);
  }
}

void updateDockerDisplay(JsonVariant data) {
  // Update Docker widget
}

void updateGPUDisplay(JsonVariant data) {
  // Update GPU widget
}

