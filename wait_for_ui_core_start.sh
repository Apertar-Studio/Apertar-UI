#!/bin/bash

set -u

READY_FILE="${1:-/run/apertar-ui/core-start-ready}"
TIMEOUT_SECONDS="${2:-8}"
if [[ ! "$TIMEOUT_SECONDS" =~ ^[0-9]+$ ]]; then
    printf 'Apertar Core startup gate: invalid timeout; starting Core immediately.\n' >&2
    exit 0
fi
TIMEOUT_SECONDS=$((10#$TIMEOUT_SECONDS))

monotonic_milliseconds() {
    local uptime unused seconds fraction
    read -r uptime unused < /proc/uptime
    seconds="${uptime%.*}"
    fraction="${uptime#*.}000"
    MONOTONIC_MS=$((10#$seconds * 1000 + 10#${fraction:0:3}))
}

has_connected_display() {
    local status
    for status in "${APERTAR_DRM_SYSFS:-/sys/class/drm}"/card*-*/status; do
        [[ "$status" != *-Writeback-* && -f "$status" ]] || continue
        [[ "$(<"$status")" == "connected" ]] && return 0
    done
    return 1
}

if ! has_connected_display; then
    printf 'Apertar Core startup gate: no screen; starting Core immediately.\n'
    exit 0
fi

if [[ -e "$READY_FILE" ]]; then
    printf 'Apertar Core startup gate: UI scene already ready.\n'
    exit 0
fi

monotonic_milliseconds
start_ms=$MONOTONIC_MS
deadline_ms=$((start_ms + TIMEOUT_SECONDS * 1000))
while :; do
    monotonic_milliseconds
    if [[ -e "$READY_FILE" ]]; then
        printf 'Apertar Core startup gate: UI scene ready after %d ms.\n' \
               "$((MONOTONIC_MS - start_ms))"
        exit 0
    fi
    ((MONOTONIC_MS < deadline_ms)) || break
    sleep 0.025
done

printf 'Apertar Core startup gate: timed out after %d seconds; starting Core anyway.\n' \
       "$TIMEOUT_SECONDS" >&2
exit 0
