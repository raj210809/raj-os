#!/usr/bin/env bash
# Parse NASM listing and emit GDB set variables for boot label addresses.
set -euo pipefail

LST="${1:?usage: gen-syms.sh boot.lst}"
ORG=0x7C00

declare -A labels=(
  [start]=
  [enter_protected_mode]=
  [pm_entry]=
)

current=""
while IFS= read -r line; do
  if [[ "$line" =~ [[:space:]]([a-zA-Z_.][a-zA-Z0-9_.]*):[[:space:]]*$ ]]; then
    name="${BASH_REMATCH[1]}"
    if [[ -n "${labels[$name]+x}" ]]; then
      current="$name"
    else
      current=""
    fi
    continue
  fi
  if [[ -n "$current" && "$line" =~ ^[[:space:]]*[0-9]+[[:space:]]+([0-9A-Fa-f]+)[[:space:]] ]]; then
    off=$((16#${BASH_REMATCH[1]}))
    addr=$((ORG + off))
    printf 'set $%s = 0x%x\n' "$current" "$addr"
    labels[$current]=1
    current=""
  fi
done < "$LST"

for name in start enter_protected_mode pm_entry; do
  if [[ -z "${labels[$name]}" ]]; then
    echo "warning: label $name not found in listing" >&2
  fi
done

echo 'set $kernel_main = 0x10000'
