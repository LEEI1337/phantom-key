/**
 * AlertManager für CYD - Verarbeitet und zeigt Alerts an
 * Zeigt Alert-Overlays, verwaltet Acknowledge, steuert LED/Vibration
 */

#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <functional>
#include "ErrorCodes.h"

// Alert Severity Levels
enum AlertSeverity {
  ALERT_INFO = 0,
  ALERT_WARNING = 1,
  ALERT_CRITICAL = 2,
  ALERT_EMERGENCY = 3
};

// Alert Struktur vom Backend
struct AlertData {
  String id;
  String message;
  AlertSeverity severity;
  uint32_t timestamp;
  String source;
  String metric;
  float value;
  bool acknowledged;
  bool isClear; // true wenn Alert behoben wurde
};

// Callback Typen
using AlertCallback = std::function<void(AlertData&)>;

class AlertManager {
private:
  TFT_eSPI* tft;
  
  // Aktive Alerts
  AlertData currentAlert;
  bool hasActiveAlert;
  uint32_t alertStartTime;
  uint32_t blinkInterval;
  bool blinkState;
  
  // Alert Queue (für mehrere Alerts)
  static const uint8_t MAX_QUEUED_ALERTS = 5;
  AlertData alertQueue[MAX_QUEUED_ALERTS];
  uint8_t queueHead;
  uint8_t queueTail;
  uint8_t queueCount;
  
  // Callbacks
  AlertCallback onAlertReceived;
  AlertCallback onAlertAcknowledged;
  
  // UI Positionen
  uint16_t overlayX, overlayY, overlayW, overlayH;
  bool overlayVisible;
  
  // Interne Methoden
  void drawAlertOverlay();
  void clearAlertOverlay();
  void triggerVibration(uint8_t pattern);
  void updateBlink();
  AlertSeverity parseSeverity(const String& sevStr);
  uint16_t getSeverityColor(AlertSeverity severity);
  
public:
  AlertManager(TFT_eSPI* display);
  
  // Initialisierung
  bool begin();
  void update(); // Muss im Loop aufgerufen werden
  
  // Alert Verarbeitung
  void processAlert(const String& jsonPayload);
  void acknowledgeCurrentAlert();
  void clearAlert();
  
  // Queue Management
  void addToQueue(AlertData& alert);
  AlertData getNextFromQueue();
  bool hasQueuedAlerts();
  
  // Callbacks
  void setOnAlertReceived(AlertCallback callback);
  void setOnAlertAcknowledged(AlertCallback callback);
  
  // State Abfragen
  bool hasActiveAlert() const { return hasActiveAlert; }
  AlertData getCurrentAlert() const { return currentAlert; }
  uint8_t getQueuedCount() const { return queueCount; }
  
  // UI Steuerung
  void showOverlay(bool show);
  void setOverlayPosition(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
};

// Implementation

AlertManager::AlertManager(TFT_eSPI* display) 
  : tft(display), hasActiveAlert(false), alertStartTime(0), 
    blinkInterval(500), blinkState(false), queueHead(0), queueTail(0), 
    queueCount(0), overlayVisible(false) {
  
  currentAlert = AlertData{ "", "", ALERT_INFO, 0, "", "", 0.0, false, false };
}

bool AlertManager::begin() {
  Serial.println("AlertManager initialized");
  
  // Overlay Standardposition (oberer Bildschirmbereich)
  overlayX = 0;
  overlayY = 0;
  overlayW = tft->width();
  overlayH = 60;
  
  return true;
}

void AlertManager::update() {
  if (!hasActiveAlert) return;
  
  // Blink-Effekt für kritische Alerts
  if (currentAlert.severity >= ALERT_CRITICAL) {
    updateBlink();
  }
  
  // Auto-Clear nach Timeout (optional)
  // if (millis() - alertStartTime > 30000) {
  //   clearAlert();
  // }
}

void AlertManager::processAlert(const String& jsonPayload) {
  // Einfacher JSON Parser (für Arduino ohne Library)
  // Format: {"id":"cpu_high","message":"CPU hoch","severity":"critical",...}
  
  AlertData alert;
  
  // JSON Felder extrahieren (sehr einfach gehalten)
  int start = jsonPayload.indexOf("\"id\":\"");
  if (start >= 0) {
    start += 6;
    int end = jsonPayload.indexOf("\"", start);
    alert.id = jsonPayload.substring(start, end);
  }
  
  start = jsonPayload.indexOf("\"message\":\"");
  if (start >= 0) {
    start += 11;
    int end = jsonPayload.indexOf("\"", start);
    alert.message = jsonPayload.substring(start, end);
  }
  
  start = jsonPayload.indexOf("\"severity\":\"");
  if (start >= 0) {
    start += 12;
    int end = jsonPayload.indexOf("\"", start);
    String sevStr = jsonPayload.substring(start, end);
    alert.severity = parseSeverity(sevStr);
  }
  
  start = jsonPayload.indexOf("\"type\":\"");
  if (start >= 0) {
    start += 8;
    int end = jsonPayload.indexOf("\"", start);
    String typeStr = jsonPayload.substring(start, end);
    alert.isClear = (typeStr == "clear");
  }
  
  // Alert ist vom Typ "clear" -> bestehenden Alert entfernen
  if (alert.isClear) {
    Serial.println("Alert cleared: " + alert.id);
    if (currentAlert.id == alert.id) {
      clearAlert();
    }
    return;
  }
  
  // Neuen Alert verarbeiten
  alert.timestamp = millis();
  alert.acknowledged = false;
  
  Serial.print("ALERT [");
  Serial.print(alert.severity);
  Serial.print("]: ");
  Serial.println(alert.message);
  
  // Wenn bereits ein Alert aktiv ist, in Queue stellen
  if (hasActiveAlert) {
    addToQueue(alert);
  } else {
    // Alert anzeigen
    currentAlert = alert;
    hasActiveAlert = true;
    alertStartTime = millis();
    
    // UI Overlay zeichnen
    drawAlertOverlay();
    
    // Vibration/Haptic Feedback
    triggerVibration(alert.severity);
    
    // Callback feuern
    if (onAlertReceived) {
      onAlertReceived(currentAlert);
    }
  }
}

void AlertManager::drawAlertOverlay() {
  if (!tft) return;
  
  overlayVisible = true;
  
  // Hintergrund je nach Severity
  uint16_t bgColor = getSeverityColor(currentAlert.severity);
  
  tft->fillRect(overlayX, overlayY, overlayW, overlayH, bgColor);
  
  // Border
  tft->drawRect(overlayX, overlayY, overlayW, overlayH, TFT_WHITE);
  
  // Icon je nach Severity
  String icon = "";
  switch (currentAlert.severity) {
    case ALERT_INFO: icon = "ℹ"; break;
    case ALERT_WARNING: icon = "⚠"; break;
    case ALERT_CRITICAL: icon = "🔴"; break;
    case ALERT_EMERGENCY: icon = "🚨"; break;
  }
  
  // Icon zeichnen (als Text, da keine Bitmap Icons)
  tft->setTextColor(TFT_WHITE);
  tft->setTextSize(2);
  tft->setCursor(overlayX + 10, overlayY + 15);
  tft->print(icon);
  
  // Message
  tft->setTextSize(1);
  tft->setCursor(overlayX + 40, overlayY + 10);
  
  // Text Wrapping (einfach)
  String msg = currentAlert.message;
  int maxChars = overlayW / 8 - 5; // Ca. Zeichen pro Zeile
  
  if (msg.length() > maxChars) {
    tft->println(msg.substring(0, maxChars));
    tft->setCursor(overlayX + 40, overlayY + 25);
    tft->println(msg.substring(maxChars));
  } else {
    tft->println(msg);
  }
  
  // Acknowledge Hint
  tft->setTextSize(1);
  tft->setCursor(overlayX + 10, overlayY + 45);
  tft->print("Touch to acknowledge");
}

void AlertManager::clearAlertOverlay() {
  if (!tft || !overlayVisible) return;
  
  // Overlay Bereich löschen (mit Hintergrundfarbe überschreiben)
  // Hier müsste der normale Hintergrund gezeichnet werden
  tft->fillRect(overlayX, overlayY, overlayW, overlayH, TFT_BLACK);
  
  overlayVisible = false;
}

void AlertManager::clearAlert() {
  hasActiveAlert = false;
  currentAlert = AlertData{ "", "", ALERT_INFO, 0, "", "", 0.0, false, false };
  
  clearAlertOverlay();
  
  // Nächsten Alert aus Queue holen
  if (hasQueuedAlerts()) {
    AlertData nextAlert = getNextFromQueue();
    processAlert(""); // Wird sofort überschrieben
    currentAlert = nextAlert;
    hasActiveAlert = true;
    alertStartTime = millis();
    drawAlertOverlay();
  }
  
  if (onAlertAcknowledged) {
    onAlertAcknowledged(currentAlert);
  }
}

void AlertManager::acknowledgeCurrentAlert() {
  if (!hasActiveAlert) return;
  
  currentAlert.acknowledged = true;
  Serial.println("Alert acknowledged: " + currentAlert.id);
  
  // Backend Notification würde hier gesendet werden (über WebSocket)
  // sendMessage("{\"type\":\"alert_ack\",\"id\":\"" + currentAlert.id + "\"}");
  
  clearAlert();
}

void AlertManager::addToQueue(AlertData& alert) {
  if (queueCount >= MAX_QUEUED_ALERTS) {
    // Queue voll, ältesten Alert verwerfen
    queueHead = (queueHead + 1) % MAX_QUEUED_ALERTS;
    queueCount--;
  }
  
  alertQueue[queueTail] = alert;
  queueTail = (queueTail + 1) % MAX_QUEUED_ALERTS;
  queueCount++;
  
  Serial.print("Alert queued (count: ");
  Serial.print(queueCount);
  Serial.println(")");
}

AlertData AlertManager::getNextFromQueue() {
  if (queueCount == 0) {
    return AlertData{ "", "", ALERT_INFO, 0, "", "", 0.0, false, false };
  }
  
  AlertData alert = alertQueue[queueHead];
  queueHead = (queueHead + 1) % MAX_QUEUED_ALERTS;
  queueCount--;
  
  return alert;
}

bool AlertManager::hasQueuedAlerts() {
  return queueCount > 0;
}

void AlertManager::updateBlink() {
  if (millis() - alertStartTime > blinkInterval) {
    blinkState = !blinkState;
    
    if (blinkState) {
      drawAlertOverlay();
    } else {
      // Kurz ausblenden
      tft->fillRect(overlayX, overlayY, overlayW, overlayH, TFT_BLACK);
    }
    
    alertStartTime = millis();
  }
}

AlertSeverity AlertManager::parseSeverity(const String& sevStr) {
  if (sevStr == "info") return ALERT_INFO;
  if (sevStr == "warning") return ALERT_WARNING;
  if (sevStr == "critical") return ALERT_CRITICAL;
  if (sevStr == "emergency") return ALERT_EMERGENCY;
  return ALERT_INFO;
}

uint16_t AlertManager::getSeverityColor(AlertSeverity severity) {
  switch (severity) {
    case ALERT_INFO: return TFT_BLUE;
    case ALERT_WARNING: return TFT_ORANGE;
    case ALERT_CRITICAL: return TFT_RED;
    case ALERT_EMERGENCY: return TFT_MAGENTA;
    default: return TFT_BLUE;
  }
}

void AlertManager::triggerVibration(uint8_t pattern) {
  // Vibration Motor Pin (falls vorhanden)
  // #define VIBRATION_PIN 4
  // 
  // switch (pattern) {
  //   case ALERT_INFO:
  //     digitalWrite(VIBRATION_PIN, HIGH);
  //     delay(100);
  //     digitalWrite(VIBRATION_PIN, LOW);
  //     break;
  //   case ALERT_WARNING:
  //     digitalWrite(VIBRATION_PIN, HIGH);
  //     delay(200);
  //     digitalWrite(VIBRATION_PIN, LOW);
  //     delay(100);
  //     digitalWrite(VIBRATION_PIN, HIGH);
  //     delay(200);
  //     digitalWrite(VIBRATION_PIN, LOW);
  //     break;
  //   case ALERT_CRITICAL:
  //     for (int i = 0; i < 5; i++) {
  //       digitalWrite(VIBRATION_PIN, HIGH);
  //       delay(100);
  //       digitalWrite(VIBRATION_PIN, LOW);
  //       delay(100);
  //     }
  //     break;
  // }
  
  Serial.print("Vibration pattern: ");
  Serial.println(pattern);
}

void AlertManager::setOnAlertReceived(AlertCallback callback) {
  onAlertReceived = callback;
}

void AlertManager::setOnAlertAcknowledged(AlertCallback callback) {
  onAlertAcknowledged = callback;
}

void AlertManager::showOverlay(bool show) {
  if (show && hasActiveAlert) {
    drawAlertOverlay();
  } else {
    clearAlertOverlay();
  }
  overlayVisible = show;
}

void AlertManager::setOverlayPosition(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  overlayX = x;
  overlayY = y;
  overlayW = w;
  overlayH = h;
  
  if (hasActiveAlert && overlayVisible) {
    drawAlertOverlay();
  }
}
