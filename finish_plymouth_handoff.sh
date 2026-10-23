#!/bin/bash

set -u

READY_FILE="${1:-${XDG_RUNTIME_DIR:-/run/apertar-ui}/main-ui-ready}"
TIMEOUT_SECONDS="${APERTAR_PLYMOUTH_HANDOFF_TIMEOUT_SECONDS:-20}"
PLYMOUTH_BIN="${APERTAR_PLYMOUTH_BIN:-/usr/bin/plymouth}"
HEADLESS_READY_FILE="${APERTAR_HEADLESS_READY_FILE:-$(dirname "$READY_FILE")/headless-ready}"

if [[ ! -x "$PLYMOUTH_BIN" ]]; then
    exit 0
fi

SECONDS=0
while [[ ! -e "$READY_FILE" && ! -e "$HEADLESS_READY_FILE" && $SECONDS -lt $TIMEOUT_SECONDS ]]; do
    sleep 0.05
done

if [[ -e "$HEADLESS_READY_FILE" ]]; then
    printf 'Apertar display handoff: headless startup; no UI frame required.\n'
elif [[ -e "$READY_FILE" ]]; then
    printf 'Apertar display handoff: primary UI frame ready; stopping Plymouth.\n'
else
    printf 'Apertar display handoff: timed out waiting for primary UI frame; stopping Plymouth.\n' >&2
fi

# The Qt framebuffer is now active, so Plymouth can release its retained buffer.
"$PLYMOUTH_BIN" quit --retain-splash >/dev/null 2>&1 || true

exit 0
