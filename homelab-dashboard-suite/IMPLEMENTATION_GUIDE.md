# Homelab Dashboard Suite - Implementierungs-Guide

## 📋 Übersicht für deine Agenten

Dieses Dokument erklärt Schritt-für-Schritt wie das System zu implementieren, konfigurieren und erweitern ist.

---

## 🏗️ Architektur-Übersicht

```
┌─────────────────────────────────────────────────────────────────────┐
│                        HOMELAB DASHBOARD SUITE                       │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐          │
│  │   Proxmox    │    │    Docker    │    │     OMV      │          │
│  │   (PVE API)  │    │  (Socket/API)│    │   (SMB/API)  │          │
│  └──────┬───────┘    └──────┬───────┘    └──────┬───────┘          │
│         │                   │                   │                   │
│  ┌──────┴───────┐    ┌──────┴───────┐    ┌──────┴───────┐          │
│  │   pfSense    │    │     GPU      │    │ HomeAssistant│          │
│  │ (XMLRPC/SSH) │    │  (NVML/ROCm) │    │   (REST API) │          │
│  └──────┬───────┘    └──────┬───────┘    └──────┬───────┘          │
│         │                   │                   │                   │
│         └───────────────────┼───────────────────┘                   │
│                             │                                       │
│                    ┌────────▼────────┐                              │
│                    │  Data Aggregator│                              │
│                    │   (Node.js)     │                              │
│                    └────────┬────────┘                              │
│                             │                                       │
│         ┌───────────────────┼───────────────────┐                   │
│         │                   │                   │                   │
│  ┌──────▼──────┐    ┌──────▼──────┐    ┌──────▼──────┐             │
│  │  WebSocket  │    │    MQTT     │    │    Redis    │             │
│  │   Server    │    │   Broker    │    │   Cache     │             │
│  └──────┬──────┘    └──────┬──────┘    └──────┬──────┘             │
│         │                   │                   │                   │
│         │           ┌───────▼───────┐           │                   │
│         │           │   Prometheus  │           │                   │
│         │           │   (Optional)  │           │                   │
│         │           └───────────────┘           │                   │
│         │                                       │                   │
│  ┌──────▼──────────────────────────────────────▼──────┐            │
│  │              CYD Dashboard Display                  │            │
│  │  • 2.4" Touchscreen                                 │            │
│  │  • Widget-basiertes UI                              │            │
│  │  • Lokaler Ring Buffer (24-48h)                     │            │
│  │  • WiFi Client                                      │            │
│  └─────────────────────────────────────────────────────┘            │
│                                                                     │
│  ┌──────────────────────────────────────────────────────┐           │
│  │           ESP-Key Enhancements                       │           │
│  │  • Erweiterte Potentiometer-Logik                    │           │
│  │  • MQTT Integration                                  │           │
│  │  • Button Combos                                     │           │
│  │  • BLE HID Keyboard                                  │           │
│  └──────────────────────────────────────────────────────┘           │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 🚀 Phase 1: Backend Setup

### 1.1 Voraussetzungen prüfen

```bash
# Docker & Docker Compose installiert?
docker --version
docker-compose --version

# Node.js (für lokales Testing)
node --version  # >= 18.x empfohlen
npm --version
```

### 1.2 Konfiguration erstellen

```bash
cd /workspace/homelab-dashboard-suite/backend

# Config Generator ausführen
python3 ../tools/config-generator.py
```

Der Generator erstellt:
- `.env` - Umgebungsvariablen mit deinen IPs/Tokens
- `config/settings.json` - Detaillierte Einstellungen

### 1.3 .env anpassen

Bearbeite `/workspace/homelab-dashboard-suite/backend/.env`:

```ini
# Server Konfiguration
PORT=3000
WS_PORT=8080
MQTT_PORT=1883

# Proxmox
PROXMOX_HOST=192.168.1.100
PROXMOX_USER=root@pam
PROXMOX_PASSWORD=dein_passwort
PROXMOX_NODE=pve

# Docker (lokaler Socket oder Remote API)
DOCKER_SOCKET=/var/run/docker.sock
# ODER für Remote:
# DOCKER_HOST=tcp://192.168.1.100:2375

# OMV
OMV_HOST=192.168.1.101
OMV_USER=admin
OMV_PASSWORD=dein_passwort

# pfSense
PFSENSE_HOST=192.168.1.1
PFSENSE_USER=admin
PFSENSE_PASSWORD=dein_passwort

# GPU (nvidia-smi oder rocm-smi)
GPU_TYPE=nvidia  # oder 'amd'
GPU_QUERY_INTERVAL=5000

# HomeAssistant
HA_HOST=http://192.168.1.102:8123
HA_TOKEN=dein_long_lived_token

# MQTT
MQTT_BROKER=192.168.1.103
MQTT_USER=dashboard
MQTT_PASSWORD=dein_mqtt_passwort

# Redis
REDIS_HOST=localhost
REDIS_PORT=6379

# Logging
LOG_LEVEL=info  # debug, info, warn, error
LOG_FILE=/var/log/dashboard/aggregator.log
```

### 1.4 Docker Compose starten

```bash
# Alle Services starten
docker-compose up -d

# Logs anzeigen
docker-compose logs -f aggregator

# Status prüfen
docker-compose ps
```

### 1.5 Backend testen

```bash
# WebSocket Verbindung testen
wscat -c ws://localhost:8080

# MQTT Test
mosquitto_sub -h localhost -t "homelab/#" -v
mosquitto_pub -h localhost -t "homelab/test" -m "Hello"
```

---

## 📱 Phase 2: CYD Firmware

### 2.1 PlatformIO installieren

```bash
# VS Code Extension installieren
# ODER CLI:
pip install platformio
pio upgrade
```

### 2.2 Board erkennen

```bash
cd /workspace/homelab-dashboard-suite/cyd-firmware

# Verfügbare Boards
pio boards | grep esp32

# CYD sollte als "esp32dev" oder ähnlich erscheinen
```

### 2.3 Konfiguration anpassen

Bearbeite `/workspace/homelab-dashboard-suite/cyd-firmware/data/config.json`:

```json
{
  "wifi": {
    "ssid": "Dein_Netzwerk",
    "password": "Dein_WLAN_Passwort"
  },
  "backend": {
    "websocket": {
      "host": "192.168.1.103",
      "port": 8080,
      "path": "/ws",
      "authToken": "dein_token"
    },
    "mqtt": {
      "enabled": true,
      "broker": "192.168.1.103",
      "port": 1883,
      "username": "cyd_dashboard",
      "password": "mqtt_pass"
    }
  },
  "display": {
    "brightness": 200,
    "orientation": 1,
    "sleepTimeout": 300
  },
  "widgets": [
    {
      "id": "proxmox_cpu",
      "type": "gauge",
      "screen": 0,
      "x": 10,
      "y": 10,
      "width": 140,
      "height": 100,
      "config": {
        "title": "Proxmox CPU",
        "min": 0,
        "max": 100,
        "unit": "%",
        "colorLow": "#2ecc71",
        "colorMid": "#f39c12",
        "colorHigh": "#e74c3c",
        "thresholdMid": 50,
        "thresholdHigh": 80
      }
    },
    {
      "id": "docker_containers",
      "type": "statusGrid",
      "screen": 0,
      "x": 160,
      "y": 10,
      "width": 150,
      "height": 100,
      "config": {
        "title": "Docker",
        "services": ["nginx", "plex", "homeassistant"]
      }
    }
  ]
}
```

### 2.4 Firmware kompilieren und flashen

```bash
# Build
pio run

# Upload via USB
pio run --target upload

# Serial Monitor
pio device monitor --baud 115200
```

### 2.5 Erste Inbetriebnahme

1. CYD mit USB verbinden
2. Serial Monitor öffnen
3. Boot-Log beobachten:
   ```
   [INFO] WiFi connecting...
   [INFO] Connected to Mein_Netzwerk
   [INFO] WebSocket connecting to 192.168.1.103:8080
   [INFO] WebSocket connected!
   [INFO] Loading widgets...
   [INFO] Dashboard ready!
   ```

---

## ⌨️ Phase 3: ESP-Key Enhancements

### 3.1 Bestehendes Projekt sichern

```bash
# Backup deines aktuellen ESP-Key Projekts
cp -r /pfad/zu/deinem/esp-key-projekt /workspace/backup-esp-key-$(date +%Y%m%d)
```

### 3.2 Neue Libraries integrieren

Kopiere die neuen Header-Dateien in dein Projekt:

```bash
cp /workspace/homelab-dashboard-suite/esp-key-enhancements/lib/*.h /pfad/zu/deinem/esp-key-projekt/lib/
```

### 3.3 PlatformIO Dependencies hinzufügen

In deiner `platformio.ini`:

```ini
[env:esp32-c3]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino

lib_deps =
    bblanchon/ArduinoJson@^7.0.0
    knolleary/PubSubClient@^2.8.0
    links2004/WebSockets@^2.3.0
```

### 3.4 Potentiometer 2.0 implementieren

In deinem `main.cpp`:

```cpp
#include "PotentiometerAdvanced.h"
#include "MQTTHandler.h"

// Globale Instanzen
PotentiometerAdvanced poti(POTI_PIN);
MQTTHandler mqtt;

void setup() {
    Serial.begin(115200);
    
    // Potentiometer initialisieren
    poti.begin(PotMode::INCREMENTAL);
    poti.onValue([](int value, PotMode mode) {
        Serial.printf("Poti Value: %d (Mode: %d)\n", value, (int)mode);
        
        // MQTT Publish für Lautstärke
        if (mode == PotMode::ABSOLUTE) {
            float volume = value / 4095.0f * 100.0f;
            mqtt.publish("homelab/audio/volume", String(volume));
        }
    });
    
    poti.onEvent([](PotEvent event, PotDirection dir) {
        Serial.printf("Poti Event: %d, Direction: %d\n", (int)event, (int)dir);
        
        // Gesten für Aktionen nutzen
        if (event == PotEvent::LONG_PRESS_CW) {
            // Nächstes Profil laden
            switchProfile();
        }
    });
    
    // MQTT initialisieren
    mqtt.begin(MQTT_BROKER, MQTT_PORT, "esp-key-01", MQTT_USER, MQTT_PASS);
    mqtt.onMessage([](const String& topic, const String& payload) {
        Serial.printf("MQTT: %s = %s\n", topic.c_str(), payload.c_str());
        
        // Befehle vom Backend empfangen
        if (topic == "homelab/commands") {
            executeCommand(payload);
        }
    });
}

void loop() {
    poti.loop(20);  // Alle 20ms aufrufen
    mqtt.loop();
    
    // Restliche Logik...
}
```

### 3.5 MQTT Topics Struktur

```
homelab/
├── status/
│   ├── proxmox/cpu
│   ├── proxmox/ram
│   ├── docker/containers
│   ├── network/upload
│   ├── network/download
│   └── gpu/temperature
├── commands/
│   ├── reboot
│   ├── shutdown
│   └── restart_service
├── audio/
│   └── volume
└── alerts/
    ├── critical
    └── warnings
```

---

## 🎨 Phase 4: UI Customization

### 4.1 Eigenes Widget erstellen

Neues Widget in `/workspace/homelab-dashboard-suite/cyd-firmware/include/MyCustomWidget.h`:

```cpp
#ifndef MY_CUSTOM_WIDGET_H
#define MY_CUSTOM_WIDGET_H

#include "Widget.h"

class MyCustomWidget : public Widget {
private:
    String customData;
    
public:
    MyCustomWidget(uint8_t id, int x, int y, int w, int h)
        : Widget(id, x, y, w, h, WidgetType::CUSTOM) {}
    
    void draw(LGFX& tft) override {
        tft.fillRect(x, y, width, height, Theme::bgSecondary);
        tft.drawString(customData, x + 5, y + height/2);
    }
    
    void update(float value) override {
        customData = String(value, 2);
        needsRedraw = true;
    }
};

#endif
```

### 4.2 Theme anpassen

In `Theme.h` Farben ändern:

```cpp
namespace Theme {
    // Hauptfarben
    constexpr uint16_t primary = 0x3498DB;      // Blau ändern
    constexpr uint16_t secondary = 0x2ECC71;    // Grün ändern
    constexpr uint16_t accent = 0xE74C3C;       // Rot ändern
    
    // Hintergrund
    constexpr uint16_t bgPrimary = 0x0A0A0A;    // Dunkler machen
    constexpr uint16_t bgSecondary = 0x1A1A1A;  // Etwas heller
}
```

---

## 🔧 Phase 5: Debugging & Error Handling

### 5.1 Error Codes verstehen

Siehe `ErrorCodes.h` für vollständige Liste:

```
0x0000 - OK
0x0100 - WIFI_ERROR_*
0x0200 - WEBSOCKET_ERROR_*
0x0300 - MQTT_ERROR_*
0x0400 - DISPLAY_ERROR_*
0x0500 - STORAGE_ERROR_*
0x0600 - CONFIG_ERROR_*
0x0700 - SENSOR_ERROR_*
```

### 5.2 Debug-Modus aktivieren

Im Serial Monitor:

```
# Debug Level setzen
DEBUG_SET=3

# WebSocket Traffic loggen
DEBUG_WS=1

# MQTT Messages loggen
DEBUG_MQTT=1
```

### 5.3 Remote Debugging

Backend hat integrierten Debug-Endpoint:

```bash
# Aktuelle Stats abrufen
curl http://localhost:3000/api/debug/stats

# Aktive Verbindungen
curl http://localhost:3000/api/debug/connections

# Logs streamen
curl -N http://localhost:3000/api/debug/logs
```

---

## 📊 Phase 6: Monitoring & Alerts

### 6.1 Prometheus Integration

Prometheus Config in `backend/config/prometheus.yml`:

```yaml
scrape_configs:
  - job_name: 'homelab-aggregator'
    static_configs:
      - targets: ['aggregator:3000']
    scrape_interval: 15s
    metrics_path: '/metrics'
```

### 6.2 Grafana Dashboard importieren

1. Grafana öffnen (http://dein-grafana:3000)
2. Data Source hinzufügen: Prometheus
3. Dashboard importieren mit ID: (wird noch erstellt)

### 6.3 Alert Rules

```yaml
# alerting_rules.yml
groups:
  - name: homelab-alerts
    rules:
      - alert: HighCPUUsage
        expr: proxmox_cpu_usage > 90
        for: 5m
        annotations:
          summary: "Proxmox CPU über 90%"
          
      - alert: ContainerDown
        expr: docker_container_status == 0
        annotations:
          summary: "Container {{ $labels.name }} ist down"
```

---

## 🔄 Wartung & Updates

### 7.1 OTA Updates für CYD

```bash
# Web-Interface für OTA
# Nach Flashen verfügbar unter: http://<cyd-ip>/update

# Oder via PlatformIO
pio run --target upload --upload-protocol=espota \
  --upload-port=<cyd-ip>
```

### 7.2 Backup erstellen

```bash
# Config Backups
tar -czf backup-$(date +%Y%m%d).tar.gz \
  backend/.env \
  cyd-firmware/data/config.json \
  esp-key-enhancements/lib/
```

### 7.3 Logs rotieren

In `docker-compose.yml`:

```yaml
services:
  aggregator:
    logging:
      driver: "local"
      options:
        max-size: "10m"
        max-file: "3"
```

---

## 🎯 Nächste Schritte

1. ✅ Backend aufsetzen und testen
2. ✅ CYD Firmware flashen
3. ✅ ESP-Key Enhancements integrieren
4. ✅ Widgets nach Bedarf anpassen
5. ✅ Alerts konfigurieren
6. ✅ Dokumentation erweitern

---

## 📞 Support & Troubleshooting

Häufige Probleme:

| Problem | Lösung |
|---------|--------|
| WiFi verbindet nicht | SSID/Password prüfen, AP Mode testen |
| WebSocket Timeout | Firewall Regeln, IP prüfen |
| Display bleibt schwarz | Helligkeit erhöhen, Wiring prüfen |
| MQTT keine Messages | Broker Logs prüfen, Topics validieren |

---

**Viel Erfolg beim Implementieren! 🚀**
