#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
KERNEL_DIR="$ROOT_DIR/service-kernel/linux"
VMRUN_MODE="${1:-safe}"

make -C "$KERNEL_DIR" all

if lsmod | grep -q '^kernova'; then
    sudo rmmod kernova
fi

if [[ "$VMRUN_MODE" == "vmrun" ]]; then
    sudo insmod "$KERNEL_DIR/kernova.ko" allow_vmrun=1
else
    sudo insmod "$KERNEL_DIR/kernova.ko"
fi

ls -l /dev/kernova
sudo dmesg | tail -n 40
