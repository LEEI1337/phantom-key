# Technical Specification (RFC) - Homelab Dashboard Suite v2.0

**RFC ID:** 001  
**Titel:** Architektur und Implementierungsdetails der Homelab Dashboard Suite v2.0  
**Status:** Active  
**Erstellt:** 2024-05-20  
**Autor:** System Architecture Team  

## 1. Zusammenfassung (Abstract)
Dieses Dokument beschreibt die technische Architektur, Datenflüsse, Sicherheitsmechanismen und Implementierungsentscheidungen für die Homelab Dashboard Suite v2.0. Es richtet sich an Entwickler, die das System erweitern, warten oder deployen wollen.

## 2. Motivation
Bestehende Monitoring-Lösungen (Grafana, Prometheus) sind zu schwerfällig für den Einsatz auf ressourcenbeschränkter Hardware (ESP32) und bieten keine physische Interaktion (Buttons, Poti). Ziel ist ein leichtgewichtiges, sicheres System mit <200ms Latenz, das auch bei Netzwerkausfall lokal funktioniert.

## 3. Systemarchitektur

### 3.1 High-Level Overview
```
┌───────────────────────────────────────────────────────────────┐
│                      Homelab Infrastructure                   │
│  [Proxmox] [Docker] [OMV] [pfSense] [GPUs] [HomeAssistant]    │
└───────────┬───────────┬──────────┬──────────┬────────┬────────┘
            │ REST      │ Socket   │ REST     │ SSH    │ REST
            ▼           ▼          ▼          ▼        ▼
┌───────────────────────────────────────────────────────────────┐
│                  Backend Aggregator (Node.js)                 │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐   │
│  │ Collectors  │  │ Alert Engine│  │   Action Service    │   │
│  │ (Polling)   │──▶│ (Rules)     │  │   (Executor)        │   │
│  └─────────────┘  └──────┬──────┘  └──────────┬──────────┘   │
│                          │                    │               │
│  ┌───────────────────────┴────────────────────┴───────────┐   │
│  │              WebSocket Server (Port 3000)              │   │
│  │              MQTT Bridge (Optional)                    │   │
│  │              Redis Cache (Temp Data)                   │   │
│  └────────────────────────────────────────────────────────┘   │
└───────────────────────────────────────────────────────────────┘
            │ WS (JSON)                │ MQTT
            ▼                          ▼
┌───────────────────────┐    ┌─────────────────────────────────┐
│   CYD Firmware        │    │   ESP-Key Firmware              │
│   (ESP32-2432S028R)   │    │   (ESP32-C3 SuperMini)          │
│  ┌─────────────────┐  │    │  ┌───────────────────────────┐  │
│  │ Render Engine   │  │    │  │ HID Keyboard (BLE/USB)    │  │
│  │ (LovyanGFX)     │  │    │  │ Macro Engine              │  │
│  ├─────────────────┤  │    │  ├───────────────────────────┤  │
│  │ Touch Handler   │  │    │  │ Potentiometer Logic       │  │
│  │ (Gestures)      │  │    │  │ (Smoothing, Modes)        │  │
│  ├─────────────────┤  │    │  ├───────────────────────────┤  │
│  │ Ring Buffer     │  │    │  │ MQTT Client               │  │
│  │ (Flash/RAM)     │  │    │  │ (HA Integration)          │  │
│  └─────────────────┘  │    │  └───────────────────────────┘  │
└───────────────────────┘    └─────────────────────────────────┘
```

### 3.2 Komponenten-Detailbeschreibung

#### 3.2.1 Backend Aggregator
- **Laufzeit**: Node.js v18+ in Docker Container.
- **Framework**: Express.js (HTTP), `ws` (WebSocket), `mqtt` (MQTT).
- **Datenfluss**:
  1.  **Collector Loop**: Paralleles Polling aller Quellen alle 5s (konfigurierbar).
  2.  **Normalization**: Umwandlung in einheitliches Schema `{ source, metric, value, unit, ts }`.
  3.  **Alert Check**: Vergleich mit Thresholds (`config/alerts.json`). Bei Match: Push zu `AlertEngine`.
  4.  **Broadcast**: Senden an alle verbundenen WebSocket-Clients.
  5.  **Persistence**: Speichern der letzten 2880 Punkte (48h) im lokalen RingBuffer (JSON/Flatfile) oder Redis.
- **Security**:
  - Middleware `authMiddleware`: Prüft `x-api-key` Header.
  - Middleware `rateLimiter`: Max 100 Requests/min pro IP.
  - Input Sanitization für alle API-Parameter.

#### 3.2.2 CYD Firmware
- **Framework**: PlatformIO mit Arduino Core for ESP32.
- **Grafik**: LovyanGFX Library (Hardware-beschleunigt).
- **Architektur**:
  - `main.cpp`: Initialisierung, Main-Loop.
  - `WiFiManager`: AutoConnect + AP Fallback.
  - `WebSocketClient`: Reconnect Logik mit exponentiellem Backoff (1s, 2s, 4s, ... max 30s).
  - `ScreenManager`: Double Buffering für flackerfreies Rendering.
  - `WidgetSystem`: Basisklasse `Widget`, abgeleitet `GaugeWidget`, `ChartWidget`, etc.
  - `TouchHandler`: Interrupt-basierte Erfassung, Debouncing, Gestenerkennung.
- **Speicher**:
  - `RingBuffer<T>`: Template-Klasse, speichert Daten im SPIFFS/LittleFS oder PSRAM.
  - Struktur: Circular Array mit Head/Tail Pointern.

#### 3.2.3 ESP-Key Enhancements
- **Rolle**: Hybrid aus HID-Keyboard und IoT-Controller.
- **Potentiometer**:
  - ADC Reading mit Oversampling (16x) für Rauschunterdrückung.
  - EMA-Filter (Exponential Moving Average) für glatte Werte.
  - Gesture-Erkennung: "Quick Turn" vs "Slow Adjust".
- **MQTT**:
  - Subscribtion auf `homelab/alerts/#` für LED-Sync.
  - Publishing von Button-Events an `homelab/macros`.

## 4. Datenmodelle & Protokolle

### 4.1 WebSocket Nachrichtenformat
**Metric Update:**
```json
{
  "type": "metric",
  "ts": 1716220000000,
  "payload": {
    "id": "proxmox_cpu",
    "label": "PVE CPU",
    "value": 42.5,
    "unit": "%",
    "min": 0,
    "max": 100,
    "status": "normal" // normal, warning, critical
  }
}
```

**Alert Event:**
```json
{
  "type": "alert",
  "ts": 1716220005000,
  "payload": {
    "id": "alert_001",
    "severity": "critical", // info, warning, critical
    "source": "docker",
    "message": "Container 'nginx' exited unexpectedly",
    "action_required": true
  }
}
```

**Action Command (Client -> Server):**
```json
{
  "type": "action_request",
  "token": "secure_api_key_xyz",
  "payload": {
    "target": "docker",
    "action": "restart",
    "params": { "container_id": "nginx" }
  }
}
```

### 4.2 Konfigurationsstruktur (config.json)
```json
{
  "wifi": { "ssid": "...", "password": "..." },
  "backend": { "host": "192.168.1.100", "port": 3000, "path": "/ws" },
  "layout": [
    { "type": "gauge", "id": "proxmox_cpu", "x": 0, "y": 0, "w": 160, "h": 120 },
    { "type": "chart", "id": "network_traffic", "x": 160, "y": 0, "w": 160, "h": 120 }
  ],
  "alerts": {
    "enabled": true,
    "cooldown_ms": 300000
  }
}
```

## 5. Sicherheitskonzept

### 5.1 Authentifizierung
- **Backend API**: Bearer Token oder API Key im Header (`Authorization: Bearer <token>`).
- **WebSocket**: Token wird beim Handshake im Subprotocol oder ersten Message gesendet.
- **ESP32**: Token wird verschlüsselt im NVS (Non-Volatile Storage) gespeichert.

### 5.2 Autorisierung
- **RBAC (Role Based Access Control)**:
  - `admin`: Vollzugriff (Read + Write/Actions).
  - `viewer`: Nur Read-Zugriff (Metrics).
- Middleware prüft Rolle vor Ausführung von Actions.

### 5.3 Netzwerksicherheit
- Empfehlung: Betrieb in einem isolierten VLAN (IoT/Management VLAN).
- Firewall-Regeln (pfSense): Nur Port 3000 (Backend) und 1883 (MQTT) von ESP-Clients erlauben.
- Kein direkter Internetzugang für ESP32 nötig (außer für NTP/Telegram).

## 6. Fehlerbehandlung & Logging

### 6.1 Error Codes
Schema: `0x[Scope][Module][Error]`
- `0x1001`: WiFi Connection Failed
- `0x1002`: WebSocket Timeout
- `0x2001`: Sensor Read Error
- `0x3001`: Render Buffer Overflow
- `0x4001`: Auth Failed

### 6.2 Logging Strategie
- **Backend**: Winston Logger mit File-Rotation (max 5 Files à 10MB). Level: `info` (Prod), `debug` (Dev).
- **ESP32**: Serial Output (115200 baud). Im Prod-Betrieb nur `ERROR` und `WARN`.

## 7. Deployment & CI/CD

### 7.1 Backend
- Docker Image Build via GitHub Actions.
- Deploy via `docker-compose up -d`.
- Healthcheck Endpoint: `/health` (prüft Verbindungen zu Quellen).

### 7.2 Firmware
- PlatformIO CI Pipeline.
- Automatisches Build bei Git Push.
- OTA-Update Server (einfacher HTTP Server auf Backend) für Fernupdates.

## 8. Offene Fragen & Zukünftige Arbeiten
- **OTA Security**: Signierung der Firmware Images verhindern Manipulation.
- **Mesh Networking**: Einsatz von ESP-NOW für direkte CYD <-> ESP-Key Kommunikation ohne Router.
- **AI Anomaly Detection**: Integration eines leichten ML-Modells (TensorFlow Lite Micro) zur Erkennung von ungewöhnlichen Mustern.

## 9. Referenzen
- [ESP32 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)
- [Node.js Security Best Practices](https://nodejs.org/en/security/)
- [LovyanGFX Documentation](https://github.com/lovyan03/LovyanGFX)

---
*Ende des Dokuments*
