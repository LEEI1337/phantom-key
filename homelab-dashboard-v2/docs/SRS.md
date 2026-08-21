# Software Requirements Specification (SRS) - Homelab Dashboard Suite v2.0

## 1. Introduction

### 1.1 Purpose
Dieses Dokument beschreibt die detaillierten funktionalen und nicht-funktionalen Anforderungen an die Homelab Dashboard Suite v2.0. Es dient als Vertragsgrundlage zwischen Stakeholdern und Entwicklungsteam sowie als Referenz für Tests und Validierung.

### 1.2 Scope
Das System ist eine verteilte Anwendung bestehend aus:
1.  **Backend Service**: Node.js Anwendung zur Datenaggregation, Alerting und Action-Execution.
2.  **CYD Client**: ESP32-basierte Firmware mit Touch-Display für Visualisierung und Interaktion.
3.  **ESP-Key Client**: Erweiterungsmodul für physische Buttons, Potentiometer und MQTT-Integration.
4.  **External Interfaces**: APIs zu Proxmox, Docker, OMV, pfSense, Home Assistant, Telegram.

### 1.3 Definitions & Acronyms
- **CYD**: Cheap Yellow Display (ESP32-2432S028R)
- **HA**: Home Assistant
- **IoT**: Internet of Things
- **MQTT**: Message Queuing Telemetry Transport
- **OTA**: Over-The-Air Update
- **PRD**: Product Requirement Document
- **SRS**: Software Requirements Specification
- **WS**: WebSocket

## 2. Overall Description

### 2.1 Product Perspective
Das System agiert als zentrale Überwachungs- und Steuerungseinheit ("Single Pane of Glass") für eine heterogene Homelab-Umgebung. Es ersetzt keine vollwertigen Monitoring-Lösungen wie Grafana/Prometheus für langfristige Analysen, sondern ergänzt diese durch Echtzeit-Visualisierung und physische Interaktion am "Point of Need".

### 2.2 User Classes and Characteristics
- **Admin (Primary)**: Vollzugriff auf alle Dashboards, Konfiguration und Actions. Technisch versiert.
- **Viewer (Secondary)**: Lesender Zugriff auf Dashboards (z.B. im NOC). Keine Actions möglich.
- **System (Automated)**: Backend-Services, die Daten bereitstellen oder Alerts auslösen.

### 2.3 Operating Environment
- **Backend**: Docker Container auf Linux (Proxmox LXC/VM), benötigt Netzwerkzugang zu allen Quellen.
- **CYD**: Standalone ESP32 Gerät, verbunden via WiFi (2.4GHz) und USB-C (Strom).
- **ESP-Key**: Verbunden via USB-C (Host PC) und WiFi/MQTT (Netzwerk).

### 2.4 Design and Implementation Constraints
- **Speicherbegrenzung ESP32**: Maximal 4MB Flash, ca. 320KB nutzbarer RAM für Applikation.
- **Netzwerklatenz**: WLAN kann instabil sein; Client muss robust reconnecten.
- **Security**: Keine sensiblen Daten (Passwörter) im Klartext auf dem ESP32 speichern.
- **Echtzeit**: UI muss flüssig laufen (>25 FPS), auch bei hoher Netzwerkauslastung.

## 3. System Features

### 3.1 Feature: Data Aggregation
**Beschreibung**: Das Backend sammelt periodisch Metriken von definierten Quellen.
**Anforderungen**:
- **SRS-FE-01**: Unterstützung der Protokolle HTTP/REST, HTTPS, Docker Socket API.
- **SRS-FE-02**: Abfrageintervalle konfigurierbar pro Quelle (Default: 5s).
- **SRS-FE-03**: Fehlerhafte Quellen dürfen das Gesamtsystem nicht blockieren (Timeout < 2s).
- **SRS-FE-04**: Daten müssen normalisiert werden (einheitliches JSON-Schema).

### 3.2 Feature: Real-Time Visualization (CYD)
**Beschreibung**: Darstellung der aggregierten Daten auf dem 320x240 Touch-Display.
**Anforderungen**:
- **SRS-FE-05**: Unterstützung von 4 Widget-Typen: Gauge, Line Chart, Status Indicator, Info Card.
- **SRS-FE-06**: Layout muss konfigurierbar sein (JSON-basiert).
- **SRS-FE-07**: Aktualisierungsrate der Widgets mind. 1 Hz bei aktiver Verbindung.
- **SRS-FE-08**: Bei Verbindungsverlust muss der letzte bekannte Zustand + "Offline"-Indikator angezeigt werden.

### 3.3 Feature: Smart Alerting
**Beschreibung**: Erkennung von kritischen Zuständen und Benachrichtigung.
**Anforderungen**:
- **SRS-FE-09**: Definition von Schwellwerten (Thresholds) pro Metrik (Warning, Critical).
- **SRS-FE-10**: Cooldown-Mechanismus (min. 5 min) zur Vermeidung von Alert-Spam.
- **SRS-FE-11**: Visuelles Feedback auf CYD (Farbwechsel, Blinken, Overlay).
- **SRS-FE-12**: Optionaler Push-Versand via Telegram Bot.
- **SRS-FE-13**: Sync mit ESP-Key LEDs (Rot = Critical, Gelb = Warning).

### 3.4 Feature: Interactive Actions
**Beschreibung**: Ausführung von Befehlen via Touch-Interface.
**Anforderungen**:
- **SRS-FE-14**: Long-Press auf Widget öffnet Kontext-Menü.
- **SRS-FE-15**: Unterstützte Aktionen: Docker Container (Start/Stop/Restart), HA Services, Custom Scripts.
- **SRS-FE-16**: Jede Action erfordert eine Bestätigung (Double-Tap oder Popup "Are you sure?").
- **SRS-FE-17**: Actions müssen authentifiziert und autorisiert sein (Role-Based Access Control).
- **SRS-FE-18**: Feedback über Erfolg/Misserfolg der Action innerhalb von 3 Sekunden.

### 3.5 Feature: Local History (Ring Buffer)
**Beschreibung**: Pufferung der letzten Datenpunkte lokal auf dem ESP32.
**Anforderungen**:
- **SRS-FE-19**: Speicherkapazität für mind. 24 Stunden Daten bei 1 Min Intervall.
- **SRS-FE-20**: Automatische Überschreibung alter Daten (FIFO-Prinzip).
- **SRS-FE-21**: Möglichkeit, historische Daten im Chart per Swipe zu navigieren.

### 3.6 Feature: Security & Authentication
**Beschreibung**: Schutz des Systems vor unbefugtem Zugriff.
**Anforderungen**:
- **SRS-FE-22**: Backend API erfordert JWT Token oder starken API-Key im Header.
- **SRS-FE-23**: Rate Limiting (max 60 Requests/min pro Client IP).
- **SRS-FE-24**: Alle Passwörter/Tokens müssen verschlüsselt gespeichert werden (im Backend Vault/Env).
- **SRS-FE-25**: Input Validation gegen Injection Attacks (XSS, Command Injection).

## 4. External Interface Requirements

### 4.1 User Interfaces (UI)
- **CYD Display**: 320x240 Pixel, 16-bit Farbe (RGB565). Touch-Eingabe (Resistiv/Kapazitiv je nach Modell).
- **Web Interface (Optional)**: Einfaches Dashboard zur Konfiguration (Port 3000).
- **Serial Console**: Debug-Ausgabe (Baud 115200) für Entwicklung und Troubleshooting.

### 4.2 Hardware Interfaces
- **ESP32 GPIO**:
  - Touch: GPIO 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 (je nach Pinout).
  - LED: GPIO 38 (CYD), GPIO 8 (ESP-Key).
  - Poti: ADC1 Channel 0 (GPIO 1).
  - Buzzer (Optional): GPIO 2.

### 4.3 Communication Interfaces
- **WebSocket**: Primärer Kanal für Echtzeitdaten (ws://backend:3000/ws).
- **MQTT**: Sekundärer Kanal für IoT-Integration (mqtt://broker:1883).
- **HTTP/REST**: Für Konfiguration und externe APIs.

## 5. Non-Functional Requirements

### 5.1 Performance
- **NFR-PER-01**: End-to-End Latenz (Sensor -> Backend -> CYD) < 200ms.
- **NFR-PER-02**: Boot-Zeit CYD < 5 Sekunden bis zum ersten Bild.
- **NFR-PER-03**: Backend CPU Usage < 10% im Idle, < 50% unter Last.

### 5.2 Reliability
- **NFR-REL-01**: Mean Time Between Failures (MTBF) > 720 Stunden (30 Tage).
- **NFR-REL-02**: Automatische Wiederherstellung nach Absturz (Watchdog Timer).
- **NFR-REL-03**: Datenkonsistenz auch bei plötzlichem Stromausfall (Flash Writes atomar).

### 5.3 Maintainability
- **NFR-MNT-01**: Code muss modular sein (Trennung von Logik und UI).
- **NFR-MNT-02**: Alle Konfigurationen externalisiert (.env, config.json).
- **NFR-MNT-03**: Logging-Level einstellbar (ERROR, WARN, INFO, DEBUG).

### 5.4 Security
- **NFR-SEC-01**: Keine Hardcoded Credentials im Repository.
- **NFR-SEC-02**: Regelmäßige Updates der Dependencies (npm audit, platformio lib update).
- **NFR-SEC-03**: Network Segmentation empfohlen (Backend im Management VLAN).

## 6. Appendices

### 6.1 Data Schema Example (WebSocket)
```json
{
  "type": "metric_update",
  "timestamp": 1716220000000,
  "data": {
    "source": "proxmox",
    "metric": "cpu_usage",
    "value": 45.2,
    "unit": "%",
    "status": "ok"
  }
}
```

### 6.2 Error Code Schema
Format: `0x[Category][Subcategory][Specific]`
- `0x0100`: Network Errors
- `0x0200`: Sensor/Data Errors
- `0x0300`: UI/Render Errors
- `0x0400`: Security/Auth Errors

---
*Erstellt: 2024-05-20 | Version: 2.0 | Status: Final*
