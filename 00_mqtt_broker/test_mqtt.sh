#!/usr/bin/env bash
set -euo pipefail

echo "[INFO] Démarrage broker Mosquitto..."
mosquitto -v -c "$(dirname "$0")/mosquitto.conf" &
BROKER_PID=$!
sleep 1

echo "[INFO] Abonnement à sensors/#..."
mosquitto_sub -t "sensors/#" -v &
SUB_PID=$!
sleep 0.5

echo "[INFO] Publication test température..."
mosquitto_pub -t "sensors/temperature" -m '{"temp": 22.5, "ts": "test"}'
sleep 0.5

echo "[INFO] Publication test humidité..."
mosquitto_pub -t "sensors/humidity" -m '{"humidity": 65.2, "ts": "test"}'
sleep 0.5

echo "[OK] Test MQTT terminé"
kill $SUB_PID $BROKER_PID 2>/dev/null
