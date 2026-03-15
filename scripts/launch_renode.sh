#!/usr/bin/env bash
set -euo pipefail

RENODE_DIR="$HOME/tools/renode/renode_1.16.1_portable"
PLATFORM_DIR="$(cd "$(dirname "$0")/../platform" && pwd)"

echo "[INFO] Lancement Renode avec plateforme ESP32..."
"${RENODE_DIR}/renode" --console "${PLATFORM_DIR}/esp32.resc"
