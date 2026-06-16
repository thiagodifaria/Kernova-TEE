#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SERVICE_DIR="$ROOT_DIR/service-api/service-cpp"
BUILD_DIR="$SERVICE_DIR/build"

log() { printf "\033[0;34m[build]\033[0m %s\n" "$1"; }
ok() { printf "\033[0;32m[ok]\033[0m %s\n" "$1"; }
fail() { printf "\033[0;31m[error]\033[0m %s\n" "$1" >&2; exit 1; }

check_dependencies() {
    local missing=()
    for command in g++ nasm cmake; do
        command -v "$command" >/dev/null 2>&1 || missing+=("$command")
    done

    if [ "${#missing[@]}" -gt 0 ]; then
        fail "Missing dependencies: ${missing[*]}"
    fi
}

clean() {
    log "Removing $BUILD_DIR"
    rm -rf "$BUILD_DIR"
}

build() {
    check_dependencies
    log "Configuring service-cpp"
    cmake -S "$SERVICE_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
    log "Building service-cpp"
    cmake --build "$BUILD_DIR" --parallel
    ok "Binary: $BUILD_DIR/Kernova"
}

test_project() {
    "$ROOT_DIR/scripts/test.sh"
}

package() {
    local version
    version="$(sed -n 's/.*VERSION \([0-9.]*\).*/\1/p' "$SERVICE_DIR/CMakeLists.txt" | head -1)"
    local dist_dir="$ROOT_DIR/dist"
    local package_name="Kernova-${version:-dev}-$(uname -m)"

    mkdir -p "$dist_dir"
    tar -czf "$dist_dir/$package_name.tar.gz" \
        --exclude='.git' \
        --exclude='build' \
        --exclude='dist' \
        -C "$ROOT_DIR" .
    ok "Package: $dist_dir/$package_name.tar.gz"
}

usage() {
    cat <<'EOF'
Usage: ./scripts/build.sh [--clean] [--build] [--test] [--package] [--all]

  --clean    Remove service-api/service-cpp/build
  --build    Configure and build the C++ service
  --test     Build and run the test suite
  --package  Create a source package in dist/
  --all      Clean, build, test and package
EOF
}

do_clean=false
do_build=false
do_test=false
do_package=false

while [ "$#" -gt 0 ]; do
    case "$1" in
        --clean|-c) do_clean=true ;;
        --build|-b) do_build=true ;;
        --test|-t) do_test=true ;;
        --package|-p) do_package=true ;;
        --all|-a)
            do_clean=true
            do_build=true
            do_test=true
            do_package=true
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            usage
            exit 1
            ;;
    esac
    shift
done

if ! $do_clean && ! $do_build && ! $do_test && ! $do_package; then
    do_build=true
fi

if $do_clean; then
    clean
fi
if $do_build; then
    build
fi
if $do_test; then
    test_project
fi
if $do_package; then
    package
fi
