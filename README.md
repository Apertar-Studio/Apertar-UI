# Apertar-UI

Apertar-UI is the touchscreen interface for Raspberry Pi-based Apertar cameras, including supported Pi 5, Pi 4, Pi 3 and Zero 2 W setups. It brings live preview, camera settings, recording, still photography and playback together in one place. Available recording features depend on the board and camera hardware.

It works with [ApertarCore](https://github.com/Apertar-Studio/Apertar-Core), which handles the camera and recording pipeline. Apertar-UI handles what you see and touch.

The current display setup runs directly on the Raspberry Pi's graphics system using Qt Quick and EGLFS/DRM/KMS. It does not require a desktop, Wayland/labwc or X11.

## At a Glance

- Touch-friendly controls for resolution, crop, frame rate, ISO, shutter and white balance.
- Clip, Still and Timelapse modes.
- CinemaDNG, MLV, H.264 and ProRes controls, depending on the board.
- Focus peaking, zebras, false colour, framing guides and preview LUTs.
- A horizontal level with roll and pitch, using a supported motion sensor.
- External HDMI monitoring.
- Clip and still browsing, with in-camera playback.
- Presets, customisable favourites and remembered settings.
- Media tools, audio controls, Wi-Fi settings, display calibration and standby.

**ApertarCore is required for real camera operation.** Keep Core and UI updated together so their controls and capabilities match.

## Camera Controls

The main camera screen keeps common controls close at hand. Open Settings for the full set of options, or put frequently used settings in Favorites.

Depending on the active sensor and mode, you can adjust:

- Resolution and crop ratio.
- Frame rate and RAW bit depth.
- Manual ISO or Auto ISO.
- Manual shutter angle/speed or Auto Shutter.
- Manual Kelvin or Auto White Balance.
- Exposure compensation when automatic exposure is in use.
- Center-weighted, Average or Spot metering, where supported.
- Autofocus settings for supported lens hardware.

Available options come from the board, sensor, driver and active tuning. For example, white balance is unavailable on mono sensors or tuning files without the required calibration.

Camera settings are remembered across restarts, with separate remembered still and clip-format choices. Presets let you save and recall a setup without rebuilding the camera.

## Recording Formats

| Format | Recording Options |
| --- | --- |
| **CinemaDNG / cDNG** | Uncompressed, Lossless JPEG, Lossy 3:1 and Lossy 5:1, where supported. |
| **MLV** | RAW video in an MLV container. |
| **H.264** | Compressed MP4 video. |
| **ProRes** | 10-bit 4:2:2 MOV, with HQ, 422, LT and Proxy profiles. Not ProRes RAW. |

ProRes also has a separate colour-space/gamma selection: Rec709 / Gamma 2.4, ARRI Wide Gamut 4 / LogC4, Sony S-Gamut3.Cine / S-Log3, DaVinci Wide Gamut / Intermediate, and P3-D60 / Linear.

CinemaDNG has an independent **Preview Color Space / Gamma** setting. This changes monitoring only; it does not turn the recorded RAW data into log video.

The Rec709 option follows the sensor tuning's rendering. The other profiles have ISP and recording-format limitations, and may need manual input-space assignment in an editor. See the [Core colour-profile guide](https://github.com/Apertar-Studio/Apertar-Core/blob/main/docs/prores-color-profiles.md) for details.

### Board Differences

| Board Family | Clip Recording Options |
| --- | --- |
| **Pi 5 / CM5-class** | All four formats, including cDNG compression and ProRes. |
| **Pi 4 / Pi 400 / CM4-class** | Uncompressed cDNG, MLV and H.264. |
| **Pi 3 / Pi 3A+ / Zero 2 W and other supported USB 2.0-only boards** | H.264 only. |

On supported Pi 3/4-class boards, hardware H.264 output is limited to 1080p30 or smaller crop outputs. Pi 5 uses software H.264 instead.

**Still-photo resolution is independent of the selected video codec.** An H.264-only board can still offer full-resolution RAW photos when its sensor and driver support them.

Sensor-specific handling includes IMX477, IMX577, IMX585 and IMX294, plus supported monochrome RAW formats. Actual modes and performance depend on your hardware and installed camera stack; an available menu option is not a guarantee of sustained recording speed.

## Stills and Timelapse

Long-press the on-screen record/shutter button to choose **Clip**, **Still** or **Timelapse**.

Still mode includes:

- Single photos or bursts of 3, 5 or 10 shots.
- A self-timer with an on-screen countdown.
- DNG, JPEG, PNG and TIFF, including supported RAW + processed pairs.
- Three-shot exposure bracketing with an adjustable EV offset.
- Long exposures up to 30 seconds, subject to sensor and driver limits.

Tap the burst pill to cycle through Single, 3, 5 and 10 shots. Bracketing captures three different exposures for later editing; it does not automatically merge them into an HDR image.

With Long Exposure enabled, preview exposure is capped at 1/15 second so you can still frame the shot. During the actual exposure, a blackout and countdown show that the camera is taking the photo.

Timelapse has selectable and custom intervals, including fractional seconds. Tap the interval pill to cycle through choices, or long-press it to select an interval or enter a custom one. A shot counter shows how many images have been captured.

Long-exposure choices are limited to fit the timelapse interval. Readout and writing still take time, so a one-second exposure does not guarantee a one-second capture cadence.

## Monitoring Tools

| Tool | What It Helps With |
| --- | --- |
| **Focus Peaking** | Highlights edges to help judge focus, with selectable colour and sensitivity. |
| **Zebra** | Marks areas above the selected brightness threshold. |
| **False Color** | Uses colours to make exposure levels easier to judge. |
| **LUT Preview** | Applies a compatible 3D `.cube` LUT for monitoring. |
| **Horizontal Level** | Shows roll and pitch, with a selectable level-indicator colour. Requires a configured IMU. |
| **Guides and Center Marker** | Helps with composition and framing. |
| **Grayscale** | Removes colour from the preview for a monochrome monitoring view. |
| **Anamorphic Desqueeze** | Corrects monitoring proportions for the selected squeeze ratio. |
| **SMPTE Bars** | Displays a reference colour-bar pattern. |

Pinch to zoom and drag to pan the preview. Preview zoom helps with checking focus; it does not crop the recording.

Monitoring overlays, preview LUTs and display calibration are not baked into the recorded image. With log/gamut preview profiles, peaking, zebras and false colour use a common tuning/Rec709 reference rather than measuring the creative LUT output.

### Loading LUTs

Put compatible 3D `.cube` files in either:

```text
/home/pi/LUTs/
<recording-media>/LUTs/
```

Open **Settings > Monitoring > LUT Preview**, tap **Refresh LUTs**, and select a file. No UI restart is needed to discover new files.

Use a LUT made for the selected preview colour space and gamma. Applying a log-input LUT to a Rec709 image will not produce the intended result.

## External HDMI Monitoring

Connect an HDMI display and enable External Monitor in Monitoring settings. Monitoring options include a clean or assisted view, information overlays and display orientation.

The direct-display launcher detects the display topology rather than saving DRM card numbers, which can change between boots. On systems with separate panel and HDMI DRM devices, a monitor-only UI process handles the second display.

Display behaviour still depends on the panel, driver and Qt build. Do not assume every HDMI adapter or display layout has been tested.

## Playback and Media

The browser refreshes when opened, with **Newest First** as its default arrangement. Browse recorded clips or stills, then use playback, frame stepping and seeking to inspect them.

Supported playback includes:

- CinemaDNG sequences.
- DNG and processed still images.
- Apertar MLV RAW clips.
- ProRes MOV clips.
- H.264 MP4/MOV clips.

ProRes and H.264 playback require the optional FFmpeg decoder plugin. Playback is software-decoded, so large clips may skip display frames to keep the timeline moving, especially on older boards.

MLV playback supports Apertar's single-file uncompressed RAW layout, not every Canon MLV variant or split-file set. Movie playback currently supports matching PCM16 WAV sidecars rather than embedded MOV audio.

Media settings provide remaining-recording-time estimates, a write-speed test, mount/eject and format controls. Remaining minutes are estimates, not a guarantee: compression, scene noise, audio and actual write rates affect them.

**Wait for WRITING to finish before ejecting media or turning off power.** The shutter/record button is disabled while a previous capture is being written or finalized. A warning indicator reports detected dropped frames.

## Audio

Audio settings let you select the available input/output devices, recording channels, sample rate and sample format. Options depend on what the connected hardware actually supports.

The UI also provides audio meters, input/headphone levels and live monitoring where supported. Core records timestamped audio as a WAV sidecar; keep that file with its clip.

A 32-bit float option only appears when supported by the input path. It does not increase the microphone's physical dynamic range or recover clipping.

## Tuning Files and Hardware

Custom tuning files can be selected in Record settings from:

```text
/home/pi/Tuning-Files/
<recording-media>/Tuning-Files/
```

Use Refresh to discover new files. Selecting a different tuning restarts the camera pipeline without rebooting the Pi. Core remembers the last custom selection and otherwise chooses the compatible sensor default.

Default folders are `defaults-pisp` for Pi 5 and `defaults-vc4` for Pi 3/4. These files target different camera pipelines; renaming one does not make it compatible with the other.

Physical buttons, rotary controls, recording LEDs and supported accessories are configured in:

```text
/home/pi/apertar-hardware.conf
```

Mappings can trigger captures, change settings, toggle monitoring tools, enter standby or run configured scripts. Core handles these actions, while the UI reflects the resulting camera state.

## System Settings

System settings include display orientation, date/time, system information, power actions and display calibration. Wi-Fi settings provide network selection and hotspot controls on a correctly configured Apertar image.

### Display Calibration

Under **System > Display Calibration**, choose Built-in or HDMI and adjust red, green, blue, gamma, black level and white level.

Calibration affects the whole rendered UI, including preview. It does not change camera white balance or recordings, and separate profiles are saved for the built-in display and HDMI.

This is a manual display adjustment, not automatic colourimeter profiling. It requires the graphics capabilities used by the calibration renderer; unsupported builds report it as unavailable.

### Standby

Set an idle timeout under **Camera Standby**, or select **System > Power Actions > Standby**.

Standby stops camera capture, pauses monitoring work and turns off supported display outputs/backlights. It is software standby, not suspend-to-RAM or a complete power-off.

Wake with a configured physical camera button, a single Pi power-button press on supported hardware, or the WebUI's Wake Camera action. Touchscreen input does not wake the camera. The first physical wake press is consumed rather than also triggering a capture.

Wi-Fi, hotspot, SSH and WebUI remain available on a configured image. Optional Pi 5 USB power-off has guards for mounted storage and other critical USB uses. Backlight and deeper power saving depend on the installed panel drivers, helpers and permissions.

## WebUI

The browser interface is served by **ApertarCore**, not by the touchscreen application. A headless camera can therefore be controlled without a connected screen.

On the same network, open:

```text
http://<camera-ip>:8000
```

If mDNS and the hostname are configured, you can also use:

```text
http://apertar.local:8000
```

The WebUI offers live preview, capture controls, settings and media downloads. Display calibration is specific to the camera's native displays, not a calibration of your browser screen.

**Keep the WebUI on a trusted network. It is not an authenticated internet-facing service.** Building this repository alone does not configure Wi-Fi, hotspot, SSH, mDNS or automatic startup.

## Building From Source

Build natively on the Pi where possible. You need:

- CMake 3.21 or newer and a C++17 compiler.
- Qt 6: Core, DBus, Gui, Network, Qml, Quick and QuickControls2.
- OpenGL, EGL, OpenGL ES 2 and libtiff development files.
- Matching Qt private development headers for QML ahead-of-time compilation and supported display-power controls.
- FFmpeg/libav development files for optional ProRes/H.264 playback.

Typical Raspberry Pi OS / Debian packages:

```bash
sudo apt install build-essential cmake pkg-config \
  qt6-base-dev qt6-base-private-dev \
  qt6-declarative-dev qt6-declarative-private-dev \
  qml6-module-qtquick qml6-module-qtquick-window \
  qml6-module-qtquick-controls qml6-module-qtquick-templates \
  qml6-module-qtqml-models qml6-module-qtqml-workerscript \
  libgl-dev libegl-dev libgles-dev libtiff-dev \
  libavcodec-dev libavformat-dev libavutil-dev libswscale-dev
```

Qt headers and runtime libraries must match. Custom camera images may use their own graphics libraries; do not replace a working camera stack blindly.

From the UI repository root:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DAPERTAR_QML_AOT=ON
cmake --build build -j2
```

Use `-j1` if a low-memory board runs out of RAM during compilation. Cross-compilation needs matching host and target Qt tools; QML AOT defaults to off for cross-builds.

The main executable is:

```text
build/Apertar-UI
```

When the decoder dependencies are found, the build also produces `build/apertar-video-libav.so`. Keep it beside the UI executable when deploying manually. Without it, movie playback is unavailable; camera recording is handled separately by Core.

## Running the UI

Start a matching Core build first. The default local connection is:

```text
/tmp/apertar-core.sock
```

For the direct-display setup, run from the UI repository root:

```bash
bash ./run_pi_direct.sh ./build/Apertar-UI
```

The Qt EGLFS/KMS backend and suitable graphics drivers must be installed. An existing UI, desktop compositor or display server can compete for display ownership.

On an Apertar image with services already installed, stop the existing UI before a manual test:

```bash
sudo systemctl stop apertar-ui-direct.service
sudo systemctl start apertar-core.service
bash ./run_pi_direct.sh ./build/Apertar-UI
```

Press `Ctrl+C` to stop the manual UI, then return to normal startup with:

```bash
sudo systemctl start apertar-ui-direct.service
```

These commands assume the services already exist. Compiling the UI does not install or enable them.

### Updating an Existing Apertar Image

Some installed images use a prepared runtime under `/home/pi/.cache/apertar-runtime`. After rebuilding, refresh that runtime as the normal camera user when your installation uses it:

```bash
bash ./sync_pi_runtime.sh .
```

That directory can contain deployed application and graphics libraries, so do not treat it as a disposable cache. A fresh source checkout does not automatically include the installed image's service, permission or graphics-runtime setup.

## For Developers

The UI renders preview and provides controls; Core owns capture state and recording rules.

```text
Apertar-UI
    |
    | JSON commands/events + shared preview buffers
    v
ApertarCore
    |
    v
libcamera + camera driver
```

Preview metadata and DMA-BUF file descriptors travel over the local Unix socket. The UI imports compatible buffers into EGL/OpenGL instead of copying full camera images through JSON. Fallback paths are available for supported non-importable formats.

The optional movie decoder is loaded when needed, keeping FFmpeg out of the UI's startup dependencies. Board and sensor restrictions must remain enforced in Core as well as reflected in the interface.

## Credits

Apertar-UI is built with Qt and other open-source libraries. Inspiration comes from [Schoolpost's cinepi-gui](https://github.com/cinepi/cinepi-gui).

Thanks to the contributors and camera builders who test Apertar on different boards, sensors and displays.
