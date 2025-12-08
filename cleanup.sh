#!/bin/bash

EXECUTE_DIR="$(pwd)"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cd "$SCRIPT_DIR" || exit 1

echo "Removing ESPHome caches..."
rm -rf esphome/.esphome
echo "Removing Python caches..."
rm -rf esphome/components/daikin_rotex_solaris/__pycache__
rm -rf esphome/components/daikin_rotex_solaris/translations/__pycache__

cd "$EXECUTE_DIR" || exit 1
