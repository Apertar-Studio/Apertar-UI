#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${1:-$SCRIPT_DIR}"
DESTINATION="${2:-${APERTAR_RUNTIME_ROOT:-${APERTAR_NVME_RUNTIME_ROOT:-$HOME/.cache/apertar-runtime}}}"
SOURCE_UI="${APERTAR_UI_BIN:-$PROJECT_ROOT/build/Apertar-UI}"
MESA_SOURCE="${APERTAR_MESA_RUNTIME_ROOT:-$PROJECT_ROOT/mesa-minimal}"
APERTAR_LIB_SOURCE="${APERTAR_RUNTIME_LIB_DIR:-$PROJECT_ROOT/build/apertar-runtime}"

if [[ ! -x "$SOURCE_UI" ]]; then
    printf 'Apertar runtime sync: UI binary is unavailable: %s\n' "$SOURCE_UI" >&2
    exit 1
fi
if [[ "$DESTINATION" != /* || "$DESTINATION" == /media/RAW || "$DESTINATION" == /media/RAW/* ]]; then
    printf 'Apertar runtime sync: destination must be an absolute path outside /media/RAW: %s\n' "$DESTINATION" >&2
    exit 1
fi

mkdir -p "$DESTINATION/bin" "$DESTINATION/lib" "$DESTINATION/qt6"
cp -fp "$SOURCE_UI" "$DESTINATION/bin/Apertar-UI"

if [[ -d "$MESA_SOURCE" ]]; then
    rm -rf "$DESTINATION/mesa-minimal"
    mkdir -p "$DESTINATION/mesa-minimal"
    cp -rLf "$MESA_SOURCE/." "$DESTINATION/mesa-minimal/"
fi
if [[ -d "$APERTAR_LIB_SOURCE" ]]; then
    rm -rf "$DESTINATION/apertar-runtime"
    mkdir -p "$DESTINATION/apertar-runtime"
    cp -rLf "$APERTAR_LIB_SOURCE/." "$DESTINATION/apertar-runtime/"
fi

rm -rf "$DESTINATION/qt6/plugins" "$DESTINATION/qt6/qml"
cp -rLf /usr/lib/aarch64-linux-gnu/qt6/plugins "$DESTINATION/qt6/plugins"
cp -rLf /usr/lib/aarch64-linux-gnu/qt6/qml "$DESTINATION/qt6/qml"

LDD_LIBRARY_PATH="$MESA_SOURCE/lib/aarch64-linux-gnu:$APERTAR_LIB_SOURCE:$DESTINATION/lib"

copy_dependencies() {
    local target="$1"
    while IFS=$'\t' read -r soname source; do
        [[ -n "$soname" && -f "$source" ]] || continue
        [[ "$source" == "$DESTINATION/lib/$soname" ]] || \
            cp -Lf "$source" "$DESTINATION/lib/$soname"
    done < <(env LD_LIBRARY_PATH="$LDD_LIBRARY_PATH" ldd "$target" 2>/dev/null \
             | awk '$2 == "=>" && $3 ~ /^\// { print $1 "\t" $3 }')
}

copy_dependencies "$SOURCE_UI"

# Keep the playback decoder beside the cached UI, but do not preload it.
VIDEO_PLAYBACK_MODULE="$(dirname "$SOURCE_UI")/apertar-video-libav.so"
if [[ -f "$VIDEO_PLAYBACK_MODULE" ]]; then
    cp -fp "$VIDEO_PLAYBACK_MODULE" "$DESTINATION/bin/apertar-video-libav.so"
    copy_dependencies "$VIDEO_PLAYBACK_MODULE"
else
    rm -f "$DESTINATION/bin/apertar-video-libav.so"
fi

# QML and platform plugins are loaded dynamically, so they are absent from the
# main executable's ldd output. Mirror only the modules Apertar imports plus the
# EGLFS and input plugins, avoiding unrelated multimedia/WebEngine libraries.
while IFS= read -r -d '' module; do
    copy_dependencies "$module"
done < <(find \
    "$DESTINATION/qt6/plugins/platforms" \
    "$DESTINATION/qt6/plugins/egldeviceintegrations" \
    "$DESTINATION/qt6/plugins/generic" \
    "$DESTINATION/qt6/plugins/imageformats" \
    "$DESTINATION/qt6/qml/QtQml/Models" \
    "$DESTINATION/qt6/qml/QtQuick/Controls" \
    "$DESTINATION/qt6/qml/QtQuick/Window" \
    -type f -name '*.so' -print0 2>/dev/null)

while IFS= read -r -d '' module; do
    copy_dependencies "$module"
done < <(find "$DESTINATION/qt6/qml/QtQuick" \
               -maxdepth 1 -type f -name '*.so' -print0 2>/dev/null)

# Keep the project build useful for debugging while reducing cold reads from
# the deployed runtime copy.
if command -v strip >/dev/null 2>&1; then
    strip --strip-unneeded "$DESTINATION/bin/Apertar-UI"
fi

stat -c '%s:%Y' "$SOURCE_UI" >"$DESTINATION/ui-build.signature"
printf 'Apertar runtime sync: destination=%s size=%s signature=%s\n' \
       "$DESTINATION" \
       "$(du -sh "$DESTINATION" | cut -f1)" \
       "$(<"$DESTINATION/ui-build.signature")"
