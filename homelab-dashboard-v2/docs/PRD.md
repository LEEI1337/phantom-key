# Homelab Dashboard Suite v2.0 - Product Requirement Document (PRD)

## 1. Executive Summary
Ein modulares, sicheres und skalierbares Dashboard-System zur Überwachung und Steuerung einer komplexen Homelab-Infrastruktur (Proxmox, Docker, OMV, pfSense, GPUs, Home Assistant). Das System besteht aus einem Backend-Aggregator, einem CYD-Touch-Display und einem ESP-Key-Macro-Pad.

## 2. Zielsetzung
- **Echtzeit-Überwachung**: Visuelle Darstellung aller kritischen Systemmetriken mit <1s Latenz.
- **Proaktive Alarmierung**: Intelligente Erkennung von Anomalien und Benachrichtigung via Display, MQTT & Telegram.
- **Interaktive Steuerung**: Ausführung von Admin-Aktionen (Container-Restart, Service-Management) direkt vom Dashboard.
- **Autonomie**: Funktion auch bei Netzausfall durch lokale Datenspeicherung (24-48h Ringbuffer).
- **Sicherheit**: Ende-zu-Ende Absicherung gegen unbefugten Zugriff (Auth, Rate Limiting, Input Validation).

## 3. Zielgruppe
- Homelab-Enthusiasten mit fortgeschrittenen Kenntnissen (Proxmox, Docker, Networking).
- Systemadministratoren, die eine physische Übersicht für ihr Rechenzentrum benötigen.
- Entwickler, die ein modulares Open-Source-Projekt erweitern möchten.

## 4. Funktionale Anforderungen

### 4.1 Backend Aggregator
| ID | Anforderung | Priorität | Status |
|----|-------------|-----------|--------|
| F-BE-01 | Sammlung von Metriken aus 6 Quellen (Proxmox, Docker, OMV, pfSense, GPU, HA) | Hoch | Implementiert |
| F-BE-02 | Bereitstellung via WebSocket (<100ms Latenz) | Hoch | Implementiert |
| F-BE-03 | Alert-Engine mit konfigurierbaren Schwellwerten & Cooldowns | Hoch | Implementiert |
| F-BE-04 | Action-API zur sicheren Ausführung von Befehlen (Docker, System, Scripts) | Hoch | Implementiert |
| F-BE-05 | Lokaler Ringbuffer-Speicher für 48h Historie | Mittel | Implementiert |
| F-BE-06 | Health-Score Berechnung (0-100%) für das Gesamtsystem | Mittel | Implementiert |
| F-BE-07 | Telegram Bot Integration für Mobile Alerts | Niedrig | Geplant |

### 4.2 CYD Firmware (Client)
| ID | Anforderung | Priorität | Status |
|----|-------------|-----------|--------|
| F-CYD-01 | Rendering von Widgets (Gauge, Chart, Status, Info) bei 30 FPS | Hoch | Implementiert |
| F-CYD-02 | Touch-Erkennung (Tap, Double, Long, Swipe) | Hoch | Implementiert |
| F-CYD-03 | Visualisierung von Alerts mit Severity-Farben | Hoch | Implementiert |
| F-CYD-04 | Interaktive Menüs für Quick-Actions (Long-Press) | Mittel | Implementiert |
| F-CYD-05 | Auto-Reconnect mit exponentiellem Backoff | Hoch | Implementiert |
| F-CYD-06 | Setup-Mode (AP) bei fehlender Konfiguration | Mittel | Implementiert |
| F-CYD-07 | OTA-Update Fähigkeit | Niedrig | Vorbereitet |

### 4.3 ESP-Key Enhancements
| ID | Anforderung | Priorität | Status |
|----|-------------|-----------|--------|
| F-ESP-01 | Erweiterte Potentiometer-Logik (Smoothing, Gestures) | Hoch | Implementiert |
| F-ESP-02 | MQTT Client für Home Assistant Integration | Hoch | Implementiert |
| F-ESP-03 | Sync mit CYD Alerts (LED Feedback) | Mittel | Implementiert |
| F-ESP-04 | Macro-Recording & Playback | Niedrig | Geplant |

## 5. Nicht-funktionale Anforderungen

### 5.1 Sicherheit (Security)
- **N-Sec-01**: Alle API-Endpunkte müssen authentifiziert sein (JWT oder API-Key).
- **N-Sec-02**: Rate Limiting (max 100 req/min pro IP) zur Verhinderung von DoS.
- **N-Sec-03**: Input-Validierung对所有Benutzereingaben (SQL-Injection, XSS Prävention).
- **N-Sec-04**: Secrets dürfen nicht im Code stehen (Umgebungsvariablen/.env).
- **N-Sec-05**: HTTPS Unterstützung für Web-Interfaces (via Reverse Proxy).

### 5.2 Performance
- **N-Perf-01**: Backend Startzeit < 5 Sekunden.
- **N-Perf-02**: WebSocket Latenz < 100ms im LAN.
- **N-Perf-03**: CYD UI Framerate > 30 FPS.
- **N-Perf-04**: Speichernutzung Backend < 512MB RAM.
- **N-Perf-05**: ESP32 Free Heap > 100KB nach Init.

### 5.3 Zuverlässigkeit (Reliability)
- **N-Rel-01**: System muss 24/7 betriebsbereit sein (Uptime > 99%).
- **N-Rel-02**: Automatische Wiederherstellung bei Absturz (Watchdog).
- **N-Rel-03**: Datenverlustschutz durch lokalen Buffer bei Netzwerkausfall.
- **N-Rel-04**: Graceful Degradation (Teilausfälle führen nicht zum Totalausfall).

### 5.4 Wartbarkeit (Maintainability)
- **N-Maint-01**: Modularer Aufbau (Microservices/Modules).
- **N-Maint-02**: Umfassende Dokumentation (README, Tech Spec, API Docs).
- **N-Maint-03**: Einheitliches Error-Handling mit Codes (0x0000-0xFFFF).
- **N-Maint-04**: Logging mit Rotation (max 10MB pro Datei).

## 6. Technische Einschränkungen
- **ESP32 Speicher**: 4MB Flash, ~400KB RAM -> Kein schweres OS, kein voller Graphana-Stack lokal.
- **Netzwerk**: Abhängig von LAN-Stabilität (WiFi kann flackern).
- **Stromversorgung**: USB-Strom für CYD/ESP-Key muss stabil sein (Brownout-Gefahr).

## 7. Meilensteine & Roadmap

| Phase | Meilenstein | Datum (Geplant) | Status |
|-------|-------------|-----------------|--------|
| M1 | Core Backend & Aggregators | Q1 2024 | ✅ Fertig |
| M2 | CYD Basis Firmware & Widgets | Q1 2024 | ✅ Fertig |
| M3 | Security Hardening & Alert Engine | Q2 2024 | ✅ Fertig |
| M4 | Interactive Actions & MQTT | Q2 2024 | ✅ Fertig |
| M5 | Telegram Bot & Voice Control | Q3 2024 | 🔄 In Arbeit |
| M6 | OTA Updates & Web Configurator | Q4 2024 | 📅 Geplant |

## 8. Erfolgskriterien
- Das System läuft stabil über 7 Tage ohne manuellen Neustart.
- Alerts werden innerhalb von 5 Sekunden nach Ereignis erkannt und angezeigt.
- Ein neuer User kann das System in < 30 Minuten installieren (Quickstart Guide).
- Code Coverage der Tests > 80% (Zukunft).

---
*Erstellt: 2024-05-20 | Version: 2.0 | Autor: AI Assistant*
