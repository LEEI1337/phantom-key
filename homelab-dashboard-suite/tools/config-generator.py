#!/usr/bin/env python3
"""
Config Generator für Homelab Dashboard Suite
Erstellt config.json für CYD und .env für Backend
"""

import json
import os
from pathlib import Path

def generate_cyd_config():
    """Generiert CYD config.json"""
    config = {
        "network": {
            "ssid": "",  # Leer für Setup Mode
            "password": "",
            "mqtt_broker": "192.168.1.100",
            "mqtt_port": 1883,
            "mqtt_user": "",
            "mqtt_password": "",
            "websocket_url": "ws://192.168.1.100",
            "websocket_port": 8765,
            "use_static_ip": False,
            "static_ip": "192.168.1.50",
            "gateway": "192.168.1.1",
            "subnet": "255.255.255.0",
            "dns": "192.168.1.1"
        },
        "display": {
            "brightness": 200,
            "timeout_seconds": 300,
            "auto_rotate": False,
            "default_screen": 0,
            "show_fps": False,
            "theme": "dark"
        },
        "data": {
            "refresh_interval_ms": 1000,
            "history_size": 2880,  # 48h @ 1min
            "log_to_sd": False,
            "aggregation_interval": 60
        },
        "services": {
            "proxmox_enabled": False,
            "proxmox_url": "https://192.168.1.10:8006",
            "proxmox_token": "",
            "docker_enabled": True,
            "docker_host": "192.168.1.100",
            "docker_port": 2375,
            "omv_enabled": False,
            "omv_url": "http://192.168.1.100",
            "pfsense_enabled": False,
            "pfsense_url": "https://192.168.1.1",
            "pfsense_user": "admin",
            "pfsense_password": "",
            "gpu_enabled": True,
            "gpu_index": 0,
            "homeassistant_enabled": True,
            "ha_url": "http://192.168.1.100:8123",
            "ha_token": ""
        },
        "poti": {
            "pin": -1,
            "mode": 1,  # 0=Volume, 1=Light, 2=Scroll, 3=Custom
            "smoothing_factor": 0.1,
            "steps": 100,
            "inverted": False,
            "custom_action": ""
        }
    }
    return config

def generate_backend_env():
    """Generiert backend/.env Datei"""
    env_content = """# Homelab Dashboard Backend Configuration

# Network Settings
HOST=0.0.0.0
PORT=8765
MQTT_BROKER=192.168.1.100
MQTT_PORT=1883
MQTT_USER=
MQTT_PASSWORD=

# Proxmox Configuration
PROXMOX_ENABLED=false
PROXMOX_URL=https://192.168.1.10:8006
PROXMOX_TOKEN=
PROXMOX_SECRET=

# Docker Configuration
DOCKER_ENABLED=true
DOCKER_HOST=unix:///var/run/docker.sock

# OMV Configuration
OMV_ENABLED=false
OMV_URL=http://192.168.1.100
OMV_USER=admin
OMV_PASSWORD=

# pfSense Configuration
PFSENSE_ENABLED=false
PFSENSE_URL=https://192.168.1.1
PFSENSE_USER=admin
PFSENSE_PASSWORD=

# GPU Configuration
GPU_ENABLED=true
GPU_INDEX=0
NVML_LIBRARY=/usr/lib/x86_64-linux-gnu/libnvidia-ml.so

# Home Assistant Configuration
HA_ENABLED=true
HA_URL=http://192.168.1.100:8123
HA_TOKEN=

# Redis Configuration
REDIS_HOST=redis
REDIS_PORT=6379
REDIS_DB=0

# Logging
LOG_LEVEL=INFO
LOG_FILE=/app/logs/aggregator.log

# Data Aggregation
AGGREGATION_INTERVAL=60
HISTORY_RETENTION_HOURS=48
"""
    return env_content

def main():
    base_dir = Path(__file__).parent.parent
    
    # CYD Config generieren
    cyd_config_path = base_dir / "cyd-firmware" / "data" / "config.json"
    cyd_config_path.parent.mkdir(parents=True, exist_ok=True)
    
    config = generate_cyd_config()
    with open(cyd_config_path, 'w') as f:
        json.dump(config, f, indent=2)
    print(f"✓ CYD Config erstellt: {cyd_config_path}")
    
    # Backend .env generieren
    backend_env_path = base_dir / "backend" / ".env"
    
    env_content = generate_backend_env()
    with open(backend_env_path, 'w') as f:
        f.write(env_content)
    print(f"✓ Backend .env erstellt: {backend_env_path}")
    
    # Beispiel config.json mit Kommentaren
    example_path = base_dir / "backend" / "config" / "example.env"
    if not example_path.exists():
        with open(example_path, 'w') as f:
            f.write(env_content)
        print(f"✓ Example .env erstellt: {example_path}")
    
    print("\n✅ Konfiguration erfolgreich generiert!")
    print("\nNächste Schritte:")
    print("1. Bearbeite die .env Datei mit deinen Daten")
    print("2. Starte das Backend: docker-compose up -d")
    print("3. Flash die CYD Firmware")
    print("4. Verbinde dich mit dem WiFi oder nutze den Setup Mode")

if __name__ == "__main__":
    main()
