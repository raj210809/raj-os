#!/usr/bin/env bash
# Launch QEMU (paused) and GDB in one shot. Serial output appears in this terminal.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

make -q build/disk-debug.img build/kernel.elf debug/addresses.gdb 2>/dev/null || make build/disk-debug.img build/kernel.elf debug/addresses.gdb

QEMU="${QEMU:-qemu-system-x86_64}"
GDB="${GDB:-gdb}"

echo ""
echo "Starting QEMU (paused) — serial + GDB on :1234"
if [ "${DEBUG_HEADLESS:-0}" = "1" ]; then
  echo "Display: headless (DEBUG_HEADLESS=1)"
  DISPLAY_ARG="-display none"
else
  echo "Display: QEMU window (set DEBUG_HEADLESS=1 to hide it)"
  DISPLAY_ARG="-display default"
fi
echo "GDB will open in this terminal. Type 'c' to run."
echo "Note: the QEMU window stays black until you type 'c' (CPU starts frozen)."
echo "Press Ctrl+C in GDB to quit both."
echo ""

cleanup() {
  kill "$QEMU_PID" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

$QEMU \
  -machine pc-i440fx-9.2 \
  -vga std \
  -drive file=build/disk-debug.img,format=raw,if=ide,index=0,media=disk \
  -boot c \
  -serial stdio \
  $DISPLAY_ARG \
  -s -S &
QEMU_PID=$!

echo "QEMU PID: $QEMU_PID (running in background)"

sleep 0.3
$GDB -x debug/kernel.gdb
