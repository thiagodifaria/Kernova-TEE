#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SERVICE_DIR="$ROOT_DIR/service-api/service-cpp"
BUILD_DIR="$SERVICE_DIR/build"

log() { printf "\033[0;34m[test]\033[0m %s\n" "$1"; }
ok() { printf "\033[0;32m[ok]\033[0m %s\n" "$1"; }

for path in \
    "$SERVICE_DIR/src/main.cpp" \
    "$SERVICE_DIR/src/virtualization/intel_vmx/core/vmx_init.asm" \
    "$SERVICE_DIR/include/registers.hpp" \
    "$SERVICE_DIR/tests/CMakeLists.txt"; do
    [ -s "$path" ] || { echo "Missing required file: $path" >&2; exit 1; }
done

log "Configuring service-cpp"
cmake -S "$SERVICE_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release

log "Building test targets"
cmake --build "$BUILD_DIR" --parallel

log "Running CTest"
ctest --test-dir "$BUILD_DIR" --output-on-failure

ok "Kernova test suite passed"
