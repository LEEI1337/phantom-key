# 🚀 Homelab Dashboard Suite v2.0 - Universal Platform

> **Foundation Phase Complete** - Ready for Docker deployment!

## ✅ What's Implemented

### Core Backend (Node.js/TypeScript)
- [x] Express server with security middleware (Helmet, CORS, Rate Limiting)
- [x] WebSocket server for real-time client communication
- [x] Redis client with auto-reconnect
- [x] TimescaleDB client with connection pooling
- [x] Collector Manager architecture
- [x] Base Runner class for extensibility
- [x] Docker Runner (collects container metrics + actions)
- [x] Comprehensive error handling with error codes
- [x] Structured logging with Winston
- [x] Health check endpoint

### Database Schema
- [x] Users table with RBAC
- [x] Data sources configuration
- [x] Metrics hypertable (TimescaleDB)
- [x] Layouts storage
- [x] Alerts and alert log
- [x] Audit logging

### WebUI Placeholder
- [x] React app structure
- [x] Nginx reverse proxy config
- [x] Docker build setup

### Infrastructure
- [x] Docker Compose with all services
- [x] MQTT Broker (Mosquitto)
- [x] Network isolation
- [x] Volume persistence
- [x] Health checks for all services

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────┐
│                 Docker Compose Stack                 │
├─────────────────────────────────────────────────────┤
│                                                      │
│  ┌──────────────┐    ┌──────────────┐               │
│  │   WebUI      │    │   Backend    │               │
│  │  (Nginx)     │───▶│  (Node.js)   │               │
│  │  Port 3001   │    │  Port 3000   │               │
│  └──────────────┘    └──────┬───────┘               │
│                              │                       │
│         ┌───────────────────┼───────────────────┐   │
│         │                   │                   │   │
│    ┌────▼────┐       ┌─────▼─────┐      ┌──────▼───┐│
│    │  Redis  │       │TimescaleDB│      │ Mosquitto││
│    │ (Cache) │       │  (Time-   │      │  (MQTT)  ││
│    │ :6379   │       │  series)  │      │ :1883    ││
│    └─────────┘       │ :5432     │      └──────────┘│
│                      └───────────┘                   │
│                                                      │
│  ┌──────────────────────────────────────────────┐   │
│  │          Runners (Extensible)                │   │
│  │  ┌────────────┐  ┌────────────┐              │   │
│  │  │   Docker   │  │  Proxmox   │ ...          │   │
│  │  │  Runner    │  │   Runner   │              │   │
│  │  └────────────┘  └────────────┘              │   │
│  └──────────────────────────────────────────────┘   │
│                                                      │
└─────────────────────────────────────────────────────┘
           │                        │
           ▼                        ▼
    ┌─────────────┐         ┌──────────────┐
    │   CYD/ESP   │         │ Sidecars     │
    │  Displays   │         │ (GPU, IPMI)  │
    └─────────────┘         └──────────────┘
```

## 🚀 Quick Start

### 1. Clone & Configure
```bash
cd homelab-dashboard-suite/v2-universal
cp backend/.env.example backend/.env
nano backend/.env  # Change passwords!
```

### 2. Start the stack
```bash
docker-compose up -d
```

### 3. Check status
```bash
docker-compose ps
docker-compose logs -f backend
```

### 4. Access services
- **WebUI**: http://localhost:3001
- **Backend API**: http://localhost:3000
- **Health Check**: http://localhost:3000/health
- **Database**: localhost:5432
- **Redis**: localhost:6379

## 📡 API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/health` | Health check |
| POST | `/api/auth/login` | Login |
| GET | `/api/auth/me` | Current user |
| GET | `/api/sources` | List data sources |
| POST | `/api/sources` | Create source |
| GET | `/api/layouts` | List layouts |
| GET | `/api/metrics` | Get metrics |

## 🔌 Adding New Runners

Create a new file in `backend/src/runners/`:

```typescript
import { BaseRunner } from './baseRunner';

export class MyServiceRunner extends BaseRunner {
  constructor() {
    super('my-service', 'myservice');
    this.pollInterval = 5000;
  }

  async start(): Promise<void> {
    await this.startPolling(async () => {
      await this.collectData();
    });
  }

  async stop(): Promise<void> {
    this.stopPolling();
  }

  async collectData(): Promise<void> {
    // Fetch data from your service
    // Store in Redis/Database
  }
}
```

Register in `CollectorManager`:
```typescript
await this.registerRunner(new MyServiceRunner());
```

## 🛠️ Development

### Backend
```bash
cd backend
npm install
npm run dev
```

### WebUI
```bash
cd webui
npm install
npm start
```

### Run Tests
```bash
npm test
```

## 📊 Supported Integrations (Roadmap)

| Category | Services | Status |
|----------|----------|--------|
| Virtualization | Proxmox, Docker, K8s, LXC | 🟡 Docker Done |
| Network | pfSense, WireGuard, UniFi, Pi-hole | ⏳ Planned |
| Storage | TrueNAS, OMV, Synology | ⏳ Planned |
| Monitoring | Prometheus, Grafana, UptimeKuma | ⏳ Planned |
| Smart Home | Home Assistant, OpenHAB | ⏳ Planned |
| Hardware | GPU, IPMI, UPS, OctoPrint | ⏳ Planned |

## 🔒 Security Notes

1. **Change default passwords** in `.env` before deploying!
2. Default admin user: `admin` / `admin123` (change immediately!)
3. Enable HTTPS in production
4. Use firewall rules to restrict access
5. Regularly update dependencies

## 📝 Next Phases

- [ ] Visual No-Code Editor (React DnD)
- [ ] JWT Authentication implementation
- [ ] Proxmox Runner
- [ ] Home Assistant Runner
- [ ] GPU Sidecar (NVML)
- [ ] IPMI Sidecar
- [ ] Alert Engine
- [ ] Multi-device support
- [ ] Firmware for ESP32/CYD

## 📄 License

MIT License - See LICENSE file

---

**Built with ❤️ for the Homelab Community**
