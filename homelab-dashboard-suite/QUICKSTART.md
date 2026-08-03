# 🚀 Quickstart Guide - Homelab Dashboard Suite

## In 5 Minuten zum laufenden Dashboard

### Schritt 1: Backend konfigurieren (2 Min)

```bash
cd /workspace/homelab-dashboard-suite/backend

# Config Generator ausführen
python3 ../tools/config-generator.py

# .env Datei bearbeiten
nano .env
# → Deine IPs und Passwörter eintragen
```

### Schritt 2: Backend starten (1 Min)

```bash
docker-compose up -d

# Prüfen ob alles läuft
docker-compose ps
# Sollte zeigen: aggregator, redis, mqtt - alle "Up"
```

### Schritt 3: CYD Firmware flashen (2 Min)

```bash
cd ../cyd-firmware

# Config anpassen
nano data/config.json
# → WiFi Daten und Backend IP eintragen

# Flashen
pio run --target upload

# Serial Monitor zum Prüfen
pio device monitor --baud 115200
```

### ✅ Fertig!

Das Dashboard sollte jetzt auf dem CYD Display laufen!

---

## 🔍 Schnell-Checks

```bash
# Backend Logs
docker-compose logs -f aggregator

# WebSocket testen
curl ws://localhost:8080

# MQTT testen
mosquitto_sub -h localhost -t "homelab/#"
```

---

## 🆘 Probleme?

Siehe `IMPLEMENTATION_GUIDE.md` für detaillierte Hilfe.
