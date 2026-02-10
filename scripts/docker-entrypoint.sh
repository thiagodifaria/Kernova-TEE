#!/bin/bash
# =============================================================================
# Docker Entry Point Script
# =============================================================================

set -e

echo "========================================"
echo "  Kernova-TEE Build Environment"
echo "========================================"
echo ""

# Verificar se estamos em modo de desenvolvimento
if [ "$1" = "dev" ]; then
    echo "[*] Development mode activated"
    exec bash
elif [ "$1" = "build" ]; then
    echo "[*] Building project..."
    cd /workspace && ./scripts/build.sh
elif [ "$1" = "test" ]; then
    echo "[*] Running tests..."
    cd /workspace && ./scripts/test.sh
elif [ "$1" = "shell" ]; then
    exec bash
else
    exec "$@"
fi
