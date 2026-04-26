# Apertar-UI

Apertar-UI is a touch-first camera interface for Raspberry Pi. It provides live preview, camera controls, playback, media tools, and still/record workflows on top of the ApertarCore backend.

The application is built with Qt Quick and is designed for embedded touchscreen use on Raspberry Pi 5 with Wayland/labwc.

## Highlights

- Touch-first camera control interface
- Live low-latency preview from ApertarCore
- Pinch-to-zoom, pan, and double-tap preview controls
- Real-time preview tools:
  - Zebra
  - Focus peaking
  - False color
  - Grayscale
  - Anamorphic de-squeeze
- Record mode and Still mode workflows
- Still timer with on-screen countdown
- Burst still capture support
- Clip browser for cDNG sequences
- Separate still browser for DNG photos
- In-app playback for recorded media
- Media eject and format controls
- Sensor-aware UI behavior for supported cameras
## Architecture

```text
Apertar-UI
  |
  | JSON control/events
  | + preview DMA-BUF FDs
  v
ApertarCore
  |
  v
libcamera / PiSP camera pipeline
```

### UI Responsibilities

- Display and render the live preview
- Provide touch-friendly camera controls
- Show monitoring tools and overlays
- Browse and play back recorded clips and stills
- Manage media operations such as eject, format, and capacity display
- Reflect camera state and settings from ApertarCore

### Preview Pipeline

- ApertarCore publishes preview frames over a Unix socket
- Apertar-UI receives preview metadata and attached DMA-BUF file descriptors
- The renderer imports those buffers directly into EGL/OpenGL for display
- Preview rendering remains low-latency and independent from clip playback

## Main Features

### Camera Control

- Resolution selection
- FPS selection
- ISO control
- White balance control
- Shutter angle / shutter speed workflow
- Manual and sensor-aware control presentation

### Preview Tools

- Zebra
- Focus peaking
- False color
- Grayscale
- Anamorphic de-squeeze
- Zoom and pan preview framing

### Recording and Stills

- cDNG recording workflow
- DNG still capture workflow
- Still timer presets
- Burst capture presets
- On-screen countdown in Still Mode

### Playback and Browsing

- Browse cDNG clip folders
- Browse individual DNG stills from `Photos`
- In-app playback
- Clip duration display
- Playback frame prefetching for smoother browsing on Raspberry Pi 5

### Media and System

- Media remaining-time estimate
- Card eject
- Card format
- Backend, sensor, and system info views
- Integrated system action controls

## Build Requirements

- CMake 3.21 or newer
- Qt 6:
  - Core
  - DBus
  - Gui
  - Network
  - Qml
  - Quick
  - QuickControls2
- OpenGL / EGL / GLESv2
- libtiff

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

The output binary is:

```text
build/Apertar-UI
```

## Run

Typical Wayland launch:

```bash
QT_QPA_PLATFORM=wayland ./build/Apertar-UI
```

Direct EGLFS launch example:

```bash
QT_QPA_PLATFORM=eglfs ./build/Apertar-UI
```

## Backend Integration

Apertar-UI expects ApertarCore to be available on:

```text
/tmp/apertar-core.sock
```

The UI uses ApertarCore for:

- camera settings and state
- recording and still commands
- preview frame delivery
- backend status synchronization

## Notes

- Apertar-UI is designed around Raspberry Pi touchscreen operation
- Sensor-specific capture rules belong in the backend, while the UI focuses on presentation and interaction
- The application is intended to run with ApertarCore as its capture backend
