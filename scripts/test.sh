#!/bin/sh
# Runs all host-side tests: C session coordinator (Pebble API stubs) and the Python CLI wrapper.
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT

cc -std=c99 -Wall -Wextra -Werror -I"$root/tests/pebble-stubs" -I"$root/src/c" \
  "$root/src/c/session_coordinator.c" "$root/src/c/session_store.c" "$root/src/c/haptic_settings.c" \
  "$root/tests/session_coordinator_test.c" -o "$out/session_coordinator_test"
"$out/session_coordinator_test"

python3 -m unittest discover -s "$root/tests"
