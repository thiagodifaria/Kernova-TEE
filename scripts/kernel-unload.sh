#!/usr/bin/env bash
set -euo pipefail

if lsmod | grep -q '^kernova'; then
    sudo rmmod kernova
fi

sudo dmesg | tail -n 40
