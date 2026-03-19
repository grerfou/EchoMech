# EchoMesh

> *Capteurs qui parlent — ESP32-S3, FreeRTOS, MQTT, IoT de bout en bout.*

[![Demo](https://img.shields.io/badge/▶_Démo-Vimeo-1ab7ea?style=for-the-badge&logo=vimeo)](https://vimeo.com/1174806072)

EchoMesh est un système IoT complet développé sur **ESP32-S3**, démontrant une maîtrise de la chaîne complète embarquée : firmware temps réel, connectivité sécurisée, pipeline de données et dashboard de visualisation.

---

## Architecture
```
ESP32-S3 (FreeRTOS)
    │
    ├── Capteurs simulés (BMP280 + LDR)
    ├── Deep Sleep (autonomie maximale)
    │
    └── MQTT over TLS
            │
            └── HiveMQ Cloud
                    │
                    ├── Telegraf
                    │       └── InfluxDB
                    │               └── Grafana Dashboard
                    │
                    └── OTA Update (déclenché via MQTT)
```

---

## Stack technique

| Couche | Technologie |
|--------|-------------|
| Microcontrôleur | ESP32-S3 (QFN56) — Xtensa LX7 dual-core, 8MB PSRAM |
| Firmware | ESP-IDF v5.2.3 — C17 |
| RTOS | FreeRTOS — tâches, queues, EventGroups, watchdog |
| Connectivité | Wi-Fi WPA2 + MQTT over TLS 1.2 (port 8883) |
| Broker cloud | HiveMQ Cloud (Serverless) |
| Pipeline data | Telegraf → InfluxDB v2 → Grafana |
| Infrastructure | Docker Compose |
| CI/CD | GitHub Actions — build automatique esp32s3 |

---

## Fonctionnalités

### Firmware ESP32-S3
- **Simulation capteurs réaliste** — bruit gaussien (Box-Muller), dérive lente, cycle jour/nuit sinusoïdal
- **Architecture multi-tâches FreeRTOS** — séparation sensor / network / watchdog
- **Deep Sleep avec réveil RTC** — séquence complète réveil → mesure → publish → sleep
- **Wi-Fi robuste** — reconnexion automatique avec backoff exponentiel
- **Synchronisation NTP** — horodatage ISO8601 avec timezone Europe/Paris
- **Client MQTT TLS** — QoS 1, Last Will Testament, buffer offline (ring buffer FIFO)
- **Payload JSON** — sérialisation cJSON avec device ID basé sur adresse MAC
- **OTA via MQTT** — mise à jour firmware à distance sans accès physique, avec rollback automatique

### Infrastructure
- **Broker cloud sécurisé** — HiveMQ Cloud, TLS, authentification par credentials
- **Pipeline temps réel** — Telegraf consomme MQTT et écrit dans InfluxDB
- **Dashboard Grafana** — température, humidité, pression, luminosité en temps réel
- **CI/CD GitHub Actions** — build automatique à chaque push

---

## Démarrage rapide

### Prérequis
- ESP-IDF v5.2.3
- Docker + Docker Compose
- Compte HiveMQ Cloud (gratuit)

### 1. Cloner le repo
```bash
git clone https://github.com/grerfou/EchoMesh.git
cd EchoMesh
```

### 2. Configurer le firmware
```bash
get_idf
cd firmware
idf.py set-target esp32s3
idf.py menuconfig
```

Dans `EchoMesh Configuration` :
- WiFi SSID / Password
- HiveMQ Cloud URL, Username, Password
- Durée du deep sleep
```bash
idf.py flash monitor
```

### 3. Lancer le dashboard
```bash
cd docker
docker compose up -d
```

| Service | URL | Credentials |
|---------|-----|-------------|
| Grafana | http://localhost:3000 | admin / echomesh123 |
| InfluxDB | http://localhost:8086 | admin / echomesh123 |

### 4. Déclencher un OTA

Publier sur HiveMQ Cloud via le Web Client :

**Topic :** `device/echomesh-XXXXXX/ota/trigger`

**Payload :**
```json
{
  "url": "https://github.com/grerfou/EchoMesh/releases/download/v1.1.0/mqtt_ota.bin",
  "version": "1.1.0",
  "sha256": ""
}
```

---

## Structure du projet
```
EchoMesh/
├── firmware/                 # Firmware final — Deep Sleep + MQTT + OTA
│   ├── main/
│   │   ├── main.c            # app_main — séquence réveil → mesure → publish → sleep
│   │   ├── wifi_manager.c    # Wi-Fi STA + reconnexion backoff exponentiel
│   │   ├── ntp_manager.c     # Sync NTP + timestamp ISO8601
│   │   ├── mqtt_manager.c    # Client MQTT TLS + buffer offline + OTA subscribe
│   │   └── ota_manager.c     # OTA HTTPS + vérification version + rollback
│   ├── components/
│   │   └── sensor_sim/       # Simulateur BMP280 + LDR (Box-Muller, dérive)
│   └── partitions.csv        # Table OTA A/B (2 × 2MB sur flash 32MB)
├── docker/
│   ├── docker-compose.yml    # InfluxDB + Telegraf + Grafana
│   └── telegraf.conf         # Bridge MQTT → InfluxDB
└── 00_mqtt_broker/           # Mosquitto local pour tests
```

---

## CI/CD

Chaque push sur `main` déclenche un build automatique du firmware sur GitHub Actions :
```yaml
- ESP-IDF v5.2.3
- Target: esp32s3
- Vérification taille firmware
```

---

## Données publiées

Chaque mesure publiée sur `sensors/data` :
```json
{
  "temp": 20.45,
  "humidity": 49.8,
  "pressure": 1013.2,
  "lux": 754.0,
  "ts": "2026-03-18T11:28:30+0100",
  "device": "echomesh-3CEF64",
  "boot_count": 42
}
```

---

## Roadmap

- [x] FreeRTOS multi-tâches + watchdog
- [x] Simulation capteurs réaliste
- [x] Wi-Fi + NTP
- [x] Stack MQTT (QoS, LWT, buffer offline)
- [x] HiveMQ Cloud + TLS
- [x] Dashboard Grafana temps réel
- [x] Deep Sleep + réveil RTC
- [x] OTA déclenché via MQTT
- [ ] Vrais capteurs BMP280 + LDR (remplacement `SENSOR_USE_REAL`)
- [ ] TLS mutuel (certificat client)

---
