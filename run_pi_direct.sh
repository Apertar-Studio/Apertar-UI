#!/bin/bash

set -euo pipefail

APERTAR_UI_BIN="${1:-$HOME/ApertarUI/build/Apertar-UI}"
if (( $# > 0 )); then
    shift
fi
APERTAR_UI_ARGS=("$@")
SCRIPT_PATH="$(readlink -f "$0")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

RUNTIME_MIRROR_ROOT="${APERTAR_RUNTIME_ROOT:-${APERTAR_NVME_RUNTIME_ROOT:-$HOME/.cache/apertar-runtime}}"
RUNTIME_MIRROR_ENABLED="${APERTAR_USE_RUNTIME_MIRROR:-${APERTAR_USE_NVME_RUNTIME:-auto}}"
RUNTIME_MIRROR_ACTIVE=0
if [[ "$RUNTIME_MIRROR_ENABLED" != "0" \
      && -x "$RUNTIME_MIRROR_ROOT/bin/Apertar-UI" \
      && -r "$RUNTIME_MIRROR_ROOT/ui-build.signature" ]]; then
    SOURCE_UI_METADATA="$(stat -c '%d %s:%Y' "$APERTAR_UI_BIN" 2>/dev/null || true)"
    SOURCE_UI_SIGNATURE="${SOURCE_UI_METADATA#* }"
    MIRRORED_UI_SIGNATURE="$(<"$RUNTIME_MIRROR_ROOT/ui-build.signature")"
    USE_MIRROR=1
    if [[ "$RUNTIME_MIRROR_ENABLED" == auto ]]; then
        MIRROR_DEVICE="$(stat -c '%d' "$RUNTIME_MIRROR_ROOT/bin/Apertar-UI" 2>/dev/null || true)"
        # Copies on the same filesystem duplicate OS library page-cache reads
        # during boot. A mirror on separate storage can still be useful.
        if [[ -z "$MIRROR_DEVICE" || "${SOURCE_UI_METADATA%% *}" == "$MIRROR_DEVICE" ]]; then
            USE_MIRROR=0
            printf 'Apertar direct display: shared system runtime (mirror on same filesystem).\n'
        fi
    fi
    if (( USE_MIRROR )) && [[ -n "$SOURCE_UI_SIGNATURE" && "$SOURCE_UI_SIGNATURE" == "$MIRRORED_UI_SIGNATURE" ]]; then
        APERTAR_UI_BIN="$RUNTIME_MIRROR_ROOT/bin/Apertar-UI"
        RUNTIME_MIRROR_ACTIVE=1
        export QT_PLUGIN_PATH="$RUNTIME_MIRROR_ROOT/qt6/plugins"
        export QML2_IMPORT_PATH="$RUNTIME_MIRROR_ROOT/qt6/qml"
        printf 'Apertar direct display: runtime mirror=%s\n' "$RUNTIME_MIRROR_ROOT"
    fi
fi

# Prefer Apertar's compact Mesa build when installed. It contains only the
# Pi's VC4/V3D drivers and avoids loading the much larger LLVM-enabled system
# Gallium library from the boot media. Removing the directory restores the
# system Mesa fallback automatically.
DEFAULT_MESA_RUNTIME_ROOT="$SCRIPT_DIR/mesa-minimal"
DEFAULT_APERTAR_RUNTIME_LIB_DIR="$SCRIPT_DIR/build/apertar-runtime"
if (( RUNTIME_MIRROR_ACTIVE )); then
    DEFAULT_MESA_RUNTIME_ROOT="$RUNTIME_MIRROR_ROOT/mesa-minimal"
    DEFAULT_APERTAR_RUNTIME_LIB_DIR="$RUNTIME_MIRROR_ROOT/apertar-runtime"
fi
MESA_RUNTIME_ROOT="${APERTAR_MESA_RUNTIME_ROOT:-$DEFAULT_MESA_RUNTIME_ROOT}"
MESA_LIB_DIR="$MESA_RUNTIME_ROOT/lib/aarch64-linux-gnu"
MESA_EGL_VENDOR="$MESA_RUNTIME_ROOT/share/glvnd/egl_vendor.d/50_mesa.json"
APERTAR_RUNTIME_LIB_DIR="${APERTAR_RUNTIME_LIB_DIR:-$DEFAULT_APERTAR_RUNTIME_LIB_DIR}"
if [[ -f "$MESA_LIB_DIR/libgbm.so.1" && -f "$MESA_EGL_VENDOR" ]]; then
    RUNTIME_LIBRARY_PATH="$MESA_LIB_DIR"
    if [[ -f "$APERTAR_RUNTIME_LIB_DIR/libproxy.so.1" ]]; then
        RUNTIME_LIBRARY_PATH="$RUNTIME_LIBRARY_PATH:$APERTAR_RUNTIME_LIB_DIR"
    fi
    if (( RUNTIME_MIRROR_ACTIVE )); then
        RUNTIME_LIBRARY_PATH="$RUNTIME_LIBRARY_PATH:$RUNTIME_MIRROR_ROOT/lib"
    fi
    export LD_LIBRARY_PATH="$RUNTIME_LIBRARY_PATH${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    export GBM_BACKENDS_PATH="$MESA_LIB_DIR/gbm"
    export LIBGL_DRIVERS_PATH="$MESA_LIB_DIR/dri"
    export __EGL_VENDOR_LIBRARY_FILENAMES="$MESA_EGL_VENDOR"
    printf 'Apertar direct display: Mesa runtime=%s\n' "$MESA_RUNTIME_ROOT"
    if [[ -f "$APERTAR_RUNTIME_LIB_DIR/libproxy.so.1" ]]; then
        printf 'Apertar direct display: proxy backend=direct\n'
    fi
fi

if [[ ! -x "$APERTAR_UI_BIN" ]]; then
    printf 'Apertar-UI binary not found or not executable: %s\n' "$APERTAR_UI_BIN" >&2
    exit 1
fi

DRM_SYSFS="${APERTAR_DRM_SYSFS:-/sys/class/drm}"
MAIN_UI_READY_FILE="${APERTAR_UI_READY_FILE:-${XDG_RUNTIME_DIR:-/tmp}/main-ui-ready}"
HEADLESS_READY_FILE="${APERTAR_HEADLESS_READY_FILE:-${MAIN_UI_READY_FILE%/*}/headless-ready}"
rm -f "$HEADLESS_READY_FILE" "$MAIN_UI_READY_FILE"

connector_card() {
    local connector="$1"
    printf '%s\n' "${connector%%-*}"
}

all_connectors() {
    local status connector
    for status in "$DRM_SYSFS"/card*-*/status; do
        [[ -f "$status" ]] || continue
        connector="${status%/status}"
        connector="${connector##*/}"
        case "$connector" in
            *-Writeback-*) continue ;;
        esac
        printf '%s\n' "$connector"
    done
}

connector_is_connected() {
    local status="$DRM_SYSFS/$1/status"
    [[ -f "$status" && "$(<"$status")" == "connected" ]]
}

is_internal_connector() {
    case "$1" in
        *-DSI-*|*-DPI-*|*-LVDS-*|*-eDP-*) return 0 ;;
        *) return 1 ;;
    esac
}

is_hdmi_connector() {
    case "$1" in
        *-HDMI-A-*) return 0 ;;
        *) return 1 ;;
    esac
}

mapfile -t CONNECTORS < <(all_connectors)
INTERNAL_CONNECTORS=()
HDMI_CONNECTORS=()
AVAILABLE_HDMI_CONNECTORS=()

for connector in "${CONNECTORS[@]}"; do
    if is_internal_connector "$connector"; then
        if connector_is_connected "$connector"; then
            INTERNAL_CONNECTORS+=("$connector")
        fi
    elif is_hdmi_connector "$connector"; then
        AVAILABLE_HDMI_CONNECTORS+=("$connector")
        if connector_is_connected "$connector"; then
            HDMI_CONNECTORS+=("$connector")
        fi
    fi
done

MAIN_CONNECTOR=""
MONITOR_CONNECTOR=""
if (( ${#INTERNAL_CONNECTORS[@]} > 0 )); then
    MAIN_CONNECTOR="${INTERNAL_CONNECTORS[0]}"
    if (( ${#HDMI_CONNECTORS[@]} > 0 )); then
        MONITOR_CONNECTOR="${HDMI_CONNECTORS[0]}"
    elif (( ${#AVAILABLE_HDMI_CONNECTORS[@]} > 0 )); then
        MONITOR_CONNECTOR="${AVAILABLE_HDMI_CONNECTORS[0]}"
    fi
elif (( ${#HDMI_CONNECTORS[@]} > 0 )); then
    MAIN_CONNECTOR="${HDMI_CONNECTORS[0]}"
    if (( ${#HDMI_CONNECTORS[@]} > 1 )); then
        MONITOR_CONNECTOR="${HDMI_CONNECTORS[1]}"
    else
        for connector in "${AVAILABLE_HDMI_CONNECTORS[@]}"; do
            if [[ "$connector" != "$MAIN_CONNECTOR" ]]; then
                MONITOR_CONNECTOR="$connector"
                break
            fi
        done
    fi
else
    for connector in "${CONNECTORS[@]}"; do
        if connector_is_connected "$connector"; then
            MAIN_CONNECTOR="$connector"
            break
        fi
    done
fi

if [[ -z "$MAIN_CONNECTOR" ]]; then
    printf 'Apertar direct display: no connected screen; headless Core/WebUI remain available.\n'
    mkdir -p "$(dirname "$HEADLESS_READY_FILE")"
    : >"$HEADLESS_READY_FILE"
    if [[ -n "${APERTAR_CORE_START_READY_FILE:-}" ]]; then
        mkdir -p "$(dirname "$APERTAR_CORE_START_READY_FILE")"
        : >"$APERTAR_CORE_START_READY_FILE"
    fi
    trap 'exit 0' INT TERM
    while true; do
        sleep "${APERTAR_MONITOR_HOTPLUG_POLL_SECONDS:-1}"
        while IFS= read -r connector; do
            if connector_is_connected "$connector"; then
                printf 'Apertar direct display: screen connected; starting UI.\n'
                exec "$SCRIPT_PATH" "$APERTAR_UI_BIN" "${APERTAR_UI_ARGS[@]}"
            fi
        done < <(all_connectors)
    done
fi

MAIN_CARD="${APERTAR_UI_DRM_DEVICE:-/dev/dri/$(connector_card "$MAIN_CONNECTOR")}"
MONITOR_CARD=""
if [[ -n "$MONITOR_CONNECTOR" ]]; then
    MONITOR_CARD="${APERTAR_MONITOR_DRM_DEVICE:-/dev/dri/$(connector_card "$MONITOR_CONNECTOR")}"
fi

RUNTIME_BASE="${XDG_RUNTIME_DIR:-/tmp}/apertar-display-${UID}"
mkdir -p "$RUNTIME_BASE"
chmod 700 "$RUNTIME_BASE"

write_kms_config() {
    local path="$1"
    local device="$2"
    printf '{\n  "device": "%s",\n  "hwcursor": false\n}\n' "$device" >"$path"
}

MAIN_KMS_CONFIG="$RUNTIME_BASE/main-kms.json"
write_kms_config "$MAIN_KMS_CONFIG" "$MAIN_CARD"

export QT_QPA_PLATFORM=eglfs
export QT_QPA_EGLFS_INTEGRATION=eglfs_kms
export QT_QPA_EGLFS_KMS_CONFIG="$MAIN_KMS_CONFIG"
export QT_QPA_EGLFS_HOTPLUG_ENABLED="${QT_QPA_EGLFS_HOTPLUG_ENABLED:-1}"
if [[ "${APERTAR_QT_USE_LIBINPUT:-0}" == "1" ]]; then
    unset QT_QPA_EGLFS_NO_LIBINPUT
    INPUT_BACKEND="libinput"
else
    export QT_QPA_EGLFS_NO_LIBINPUT=1
    INPUT_BACKEND="evdev"
fi
export QSG_RENDER_LOOP="${QSG_RENDER_LOOP:-basic}"
export APERTAR_MONITOR_RUNTIME_SOCKET="$RUNTIME_BASE/monitor-state.sock"
export APERTAR_UI_READY_FILE="$MAIN_UI_READY_FILE"
unset DISPLAY WAYLAND_DISPLAY

printf 'Apertar direct display: UI=%s (%s)' "$MAIN_CONNECTOR" "$MAIN_CARD"
if [[ -n "$MONITOR_CONNECTOR" ]]; then
    printf ' monitor=%s (%s)' "$MONITOR_CONNECTOR" "$MONITOR_CARD"
fi
printf '\n'
printf 'Apertar direct display: input backend=%s\n' "$INPUT_BACKEND"

MONITOR_PID=""
MAIN_WATCH_PID=""
cleanup() {
    if [[ -n "$MONITOR_PID" ]] && kill -0 "$MONITOR_PID" 2>/dev/null; then
        kill "$MONITOR_PID" 2>/dev/null || true
        wait "$MONITOR_PID" 2>/dev/null || true
    fi
    if [[ -n "$MAIN_WATCH_PID" ]] && kill -0 "$MAIN_WATCH_PID" 2>/dev/null; then
        kill "$MAIN_WATCH_PID" 2>/dev/null || true
        wait "$MAIN_WATCH_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT INT TERM

if [[ -n "$MONITOR_CARD" && "$MONITOR_CARD" != "$MAIN_CARD" ]]; then
    MONITOR_KMS_CONFIG="$RUNTIME_BASE/monitor-kms.json"
    write_kms_config "$MONITOR_KMS_CONFIG" "$MONITOR_CARD"

    monitor_card_connected() {
        local candidate
        for candidate in "${AVAILABLE_HDMI_CONNECTORS[@]}"; do
            if [[ "/dev/dri/$(connector_card "$candidate")" == "$MONITOR_CARD" ]] \
                && connector_is_connected "$candidate"; then
                return 0
            fi
        done
        return 1
    }

    monitor_helper_supervisor() {
        local child_pid=""
        local previous_state=""
        local state

        stop_child() {
            if [[ -n "$child_pid" ]] && kill -0 "$child_pid" 2>/dev/null; then
                kill "$child_pid" 2>/dev/null || true
                wait "$child_pid" 2>/dev/null || true
            fi
            child_pid=""
        }

        start_child() {
            APERTAR_MONITOR_ONLY=1 \
            QT_QPA_DISABLE_INPUT=1 \
            QT_QPA_EGLFS_DISABLE_INPUT=1 \
            QT_QPA_EGLFS_KMS_CONFIG="$MONITOR_KMS_CONFIG" \
                "$APERTAR_UI_BIN" "${APERTAR_UI_ARGS[@]}" &
            child_pid=$!
        }

        trap 'stop_child; exit 0' INT TERM

        while true; do
            if monitor_card_connected; then
                state="connected"
            else
                state="disconnected"
            fi

            if [[ "$state" == "disconnected" ]]; then
                if [[ "$state" != "$previous_state" ]]; then
                    stop_child
                    printf 'Apertar direct display: waiting for HDMI on %s.\n' "$MONITOR_CARD"
                fi
                previous_state="$state"
            elif [[ ! -f "$MAIN_UI_READY_FILE" ]]; then
                if [[ "$previous_state" != "connected-waiting-for-ui" ]]; then
                    printf 'Apertar direct display: HDMI connected on %s; waiting for the primary UI first frame.\n' "$MONITOR_CARD"
                fi
                previous_state="connected-waiting-for-ui"
            elif [[ "$previous_state" != "connected" ]]; then
                stop_child
                printf 'Apertar direct display: primary UI is ready; starting HDMI monitor helper on %s.\n' "$MONITOR_CARD"
                start_child
                previous_state="connected"
            elif [[ -z "$child_pid" ]] || ! kill -0 "$child_pid" 2>/dev/null; then
                wait "$child_pid" 2>/dev/null || true
                printf 'Apertar direct display: monitor helper stopped; restarting it.\n'
                start_child
            fi

            sleep "${APERTAR_MONITOR_HOTPLUG_POLL_SECONDS:-0.5}" &
            wait $! || true
        done
    }

    monitor_helper_supervisor &
    MONITOR_PID=$!
    printf 'Apertar direct display: split DRM topology, monitor supervisor pid=%s\n' "$MONITOR_PID"
else
    printf 'Apertar direct display: single Qt process owns all required outputs.\n'
fi

same_card_monitor_signature() {
    local candidate status signature=""
    for candidate in "${AVAILABLE_HDMI_CONNECTORS[@]}"; do
        [[ "$candidate" != "$MAIN_CONNECTOR" ]] || continue
        [[ "/dev/dri/$(connector_card "$candidate")" == "$MAIN_CARD" ]] || continue
        status="disconnected"
        if connector_is_connected "$candidate"; then
            status="connected"
        fi
        signature+="$candidate=$status;"
    done
    printf '%s\n' "$signature"
}

watch_same_card_hotplug() {
    local baseline="$1"
    local main_pid="$2"
    local restart_marker="$3"
    local current

    while kill -0 "$main_pid" 2>/dev/null; do
        sleep "${APERTAR_MONITOR_HOTPLUG_POLL_SECONDS:-0.5}"
        current="$(same_card_monitor_signature)"
        if [[ "$current" != "$baseline" ]]; then
            printf 'Apertar direct display: same-card HDMI state changed; restarting Qt to re-enumerate KMS outputs.\n'
            : >"$restart_marker"
            kill -TERM "$main_pid" 2>/dev/null || true
            return
        fi
    done
}

if [[ -n "$MONITOR_CONNECTOR" && "$MONITOR_CARD" == "$MAIN_CARD" ]]; then
    RESTART_MARKER="$RUNTIME_BASE/restart-for-hdmi-hotplug"
    rm -f "$RESTART_MARKER"
    BASELINE_SIGNATURE="$(same_card_monitor_signature)"

    "$APERTAR_UI_BIN" "${APERTAR_UI_ARGS[@]}" &
    MAIN_PROCESS_PID=$!
    watch_same_card_hotplug "$BASELINE_SIGNATURE" "$MAIN_PROCESS_PID" "$RESTART_MARKER" &
    MAIN_WATCH_PID=$!

    set +e
    wait "$MAIN_PROCESS_PID"
    MAIN_STATUS=$?
    set -e

    if kill -0 "$MAIN_WATCH_PID" 2>/dev/null; then
        kill "$MAIN_WATCH_PID" 2>/dev/null || true
        wait "$MAIN_WATCH_PID" 2>/dev/null || true
    fi
    MAIN_WATCH_PID=""

    if [[ -f "$RESTART_MARKER" ]]; then
        rm -f "$RESTART_MARKER"
        exec "$SCRIPT_PATH" "$APERTAR_UI_BIN" "${APERTAR_UI_ARGS[@]}"
    fi
    exit "$MAIN_STATUS"
fi

"$APERTAR_UI_BIN" "${APERTAR_UI_ARGS[@]}"
