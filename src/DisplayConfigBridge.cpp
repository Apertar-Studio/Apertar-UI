#include "DisplayConfigBridge.hpp"

#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QSettings>
#include <QStringList>
#include <QTextStream>
#include <QVariant>
#include <QtGlobal>

namespace {

constexpr int kDefaultUiWidth = 720;
constexpr int kDefaultUiHeight = 720;
constexpr int kDefaultUiRotationDegrees = 0;
constexpr double kDefaultControlsOpacity = 1.0;
constexpr double kDefaultZoomToggleFactor = 4.0;

QString defaultHardwareConfigPath()
{
    return QStringLiteral("/home/pi/apertar-hardware.conf");
}

QStringList hardwareConfigCandidatePaths()
{
    QStringList candidates;

    const QString envPath = qEnvironmentVariable("APERTAR_HARDWARE_CONFIG").trimmed();
    if (!envPath.isEmpty())
        candidates.push_back(envPath);

    candidates.push_back(defaultHardwareConfigPath());
    candidates.push_back(QStringLiteral("/boot/firmware/apertar-hardware.conf"));
    candidates.push_back(QStringLiteral("/boot/apertar-hardware.conf"));

    candidates.removeDuplicates();
    return candidates;
}

bool ensureDefaultHardwareConfigFile()
{
    const QString configPath = defaultHardwareConfigPath();
    const QFileInfo info(configPath);
    if (info.exists())
        return true;

    QDir dir = info.dir();
    if (!dir.exists() && !dir.mkpath(QStringLiteral(".")))
        return false;

    QSaveFile file(configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;

    QTextStream stream(&file);
    stream << "# Apertar hardware configuration\n"
           << "# Primary location: /home/pi/apertar-hardware.conf\n"
           << "[network]\n"
           << "# Hotspot fallback wait in seconds (0-86400); 0 removes the delay.\n"
           << "# An established Wi-Fi connection is left alone. Restart apertar-network.service after changes.\n"
           << "hotspot_delay_seconds=60\n\n"
           << "[run_script]\n"
           << "# run_script (alias script) runs all configured scripts in order, up to five.\n"
           << "# run_script1..run_script5 (aliases script1..script5) select one slot.\n"
           << "# Example: P2=run_script1,run_script3. Absolute Bash paths; runs as pi.\n"
           << "# path is script 1 (path1 is an alias); unused slots stay empty.\n"
           << "# One script at a time; duplicate pending slots ignored. No automatic sudo.\n"
           << "path=\npath2=\npath3=\npath4=\npath5=\n\n"
	           << "[ina219]\n"
	           << "enabled=true\n"
	           << "bus=4\n"
	           << "address=0x40\n\n"
	           << "[pcf8574]\n"
	           << "# Optional I2C GPIO expander for physical buttons.\n"
	           << "# Pin actions: none, capture, record_toggle, photo, timelapse, eject,\n"
	           << "# fps, fps_up, fps_down, iso, iso_up, iso_down,\n"
	           << "# resolution, resolution_up, resolution_down,\n"
	           << "# shutter, shutter_up, shutter_down, wb, wb_up, wb_down,\n"
	           << "# still_mode, settings, clip_browser, quick_menu, focus_peaking, zebra,\n"
	           << "# false_color, guides, center_marker, grayscale, smpte, external_monitor,\n"
	           << "# date_time_overlay, audio_meter\n"
	           << "# Exact shortcuts: iso200, wb5000, fps24, resolution4056x2280,\n"
	           << "# EV actions: ev0, ev1, ev-1, ev_up, ev_down (1/3 EV, -3..+3).\n"
	           << "# shutter180, shutter_angle180, shutter_speed48, shutter1/48.\n"
	           << "# Use recindicatorled on one unused pin to drive a recording LED output.\n"
	           << "enabled=false\n"
	           << "bus=1\n"
	           << "address=0x20\n"
	           << "active_low=true\n"
	           << "poll_ms=40\n"
	           << "debounce_ms=60\n"
	           << "pin0=none\n"
	           << "pin1=none\n"
	           << "pin2=none\n"
	           << "pin3=none\n"
	           << "pin4=none\n"
	           << "pin5=none\n"
	           << "pin6=none\n"
	           << "pin7=none\n\n"
	           << "[trigger_output]\n"
	           << "# Optional capture trigger pulse for DIY hot shoe, flash sync, tally, or accessories.\n"
	           << "# target=gpio uses a Raspberry Pi GPIO line. target=pcf8574 uses a PCF8574 pin.\n"
	           << "# For hot-shoe style trigger circuits use an optocoupler/transistor; do not connect flash trigger voltage directly to a Pi GPIO.\n"
	           << "enabled=false\n"
	           << "target=gpio\n"
	           << "events=recording_start,still,timelapse_frame\n"
	           << "pulse_ms=80\n"
	           << "active_high=false\n"
	           << "gpiochip=auto\n"
	           << "line=17\n"
	           << "open_drain=true\n"
	           << "# For target=pcf8574, bus/address default to the [pcf8574] section unless overridden here.\n"
	           << "# bus=1\n"
	           << "# address=0x20\n"
	           << "pcf_pin=0\n\n"
	           << "[recording_indicator]\n"
	           << "# Optional recording tally LED. You can also set pinX=recindicatorled in [pcf8574]\n"
	           << "# or dX=recindicatorled in [gpio_buttons].\n"
	           << "# Default active_low wiring: 3.3V -> resistor -> LED -> output pin.\n"
	           << "enabled=false\n"
	           << "target=gpio\n"
	           << "gpiochip=auto\n"
	           << "line=17\n"
	           << "active_high=false\n"
	           << "open_drain=false\n"
	           << "# For target=pcf8574, bus/address default to [pcf8574] unless overridden here.\n"
	           << "# bus=1\n"
	           << "# address=0x20\n"
	           << "pcf_pin=7\n\n"
	           << "[neorotary4]\n"
	           << "# Optional Adafruit NeoRotary 4 / seesaw quad rotary encoder.\n"
	           << "# Default address is 0x49. Each encoder has clockwise, counter-clockwise, and press actions.\n"
	           << "# Actions use the same names as PCF8574 pins above. Set an action to none to disable it.\n"
	           << "# Encoder button examples: iso200, wb5000, fps24, resolution4056x2280.\n"
	           << "enabled=false\n"
	           << "bus=1\n"
	           << "address=0x49\n"
	           << "poll_ms=30\n"
	           << "debounce_ms=60\n"
	           << "invert_direction=false\n"
	           << "reset_on_start=false\n"
	           << "max_events_per_poll=8\n"
	           << "encoder0_cw=iso_up\n"
	           << "encoder0_ccw=iso_down\n"
	           << "encoder0_press=iso\n"
	           << "encoder1_cw=shutter_up\n"
	           << "encoder1_ccw=shutter_down\n"
	           << "encoder1_press=shutter\n"
	           << "encoder2_cw=wb_up\n"
	           << "encoder2_ccw=wb_down\n"
	           << "encoder2_press=wb\n"
	           << "encoder3_cw=fps_up\n"
	           << "encoder3_ccw=fps_down\n"
	           << "encoder3_press=resolution\n\n"
	           << "[gpio_buttons]\n"
	           << "# Optional direct Raspberry Pi GPIO / Grove digital button inputs.\n"
	           << "# Grove digital ports use the BCM number printed on the Grove Base HAT, for example D5 -> d5.\n"
	           << "# Use d17=recindicatorled to reserve GPIO17 as a recording LED output.\n"
	           << "enabled=false\n"
	           << "chip=auto\n"
	           << "active_low=true\n"
	           << "pull=up\n"
	           << "poll_ms=12\n"
	           << "debounce_ms=60\n"
	           << "d5=capture\n"
	           << "d6=eject\n\n"
	           << "[gpio_encoders]\n"
	           << "# Optional direct GPIO / Grove incremental rotary encoders.\n"
	           << "# For Grove Encoder, connect SIGA/SIGB to the two configured digital pins.\n"
	           << "enabled=false\n"
	           << "chip=auto\n"
	           << "active_low=true\n"
	           << "pull=up\n"
	           << "poll_ms=3\n"
	           << "debounce_ms=40\n"
	           << "encoder0_a=5\n"
	           << "encoder0_b=6\n"
	           << "encoder0_cw=iso_up\n"
	           << "encoder0_ccw=iso_down\n"
	           << "encoder0_press_gpio=13\n"
	           << "encoder0_press=iso\n"
	           << "encoder0_steps_per_event=4\n\n"
	           << "[grove_analog]\n"
	           << "# Optional Grove Base HAT analog controls: potentiometers, sliders, rotary angle sensors.\n"
	           << "# ADC address is usually 0x04 on STM32 HATs and 0x08 on newer MM32 HATs.\n"
	           << "enabled=false\n"
	           << "bus=1\n"
	           << "address=0x04\n"
	           << "poll_ms=80\n"
	           << "deadband=32\n"
	           << "emit_on_start=false\n"
	           << "a0=wb\n"
	           << "a0_min=2500\n"
	           << "a0_max=8000\n"
	           << "a0_step=50\n"
	           << "a1=iso\n"
	           << "a1_mode=preset\n\n"
		           << "[imu]\n"
		           << "# Current options: driver=mpu6050, bmi160, bmi270, icm42688p.\n"
		           << "# Free-running IMUs use sync_offset_ms to bake in a measured Gyroflow sync correction.\n"
		           << "enabled=true\n"
		           << "driver=mpu6050\n"
		           << "sync_mode=free_run\n"
		           << "sync_offset_ms=0\n\n"
	           << "[mpu6050]\n"
	           << "# Optional gyro/accelerometer sidecar recording for Gyroflow stabilization.\n"
	           << "# Enable \"Record Gyro Data\" in Record settings to write a .gcsv beside each clip.\n"
	           << "enabled=true\n"
	           << "bus=1\n"
	           << "address=0x68\n"
		           << "sample_rate_hz=500\n"
		           << "gyro_range_dps=1000\n"
		           << "accel_range_g=8\n"
		           << "dlpf_hz=98\n"
		           << "use_fifo=true\n"
		           << "fifo_batch_samples=24\n"
		           << "# orientation uses Gyroflow axis notation. For chip facing back, X left, Y down, use xYz.\n"
		           << "orientation=XYZ\n"
		           << "# UI-only neutral level reference. Use Set Level in Gyro Calibration to update these.\n"
		           << "level_reference_set=false\n"
		           << "level_reference_orientation=XYZ\n"
		           << "level_accel_x=0\n"
		           << "level_accel_y=0\n"
		           << "level_accel_z=0\n"
		           << "# Prefer stored/manual bias. Set auto_calibrate_seconds only if the camera is guaranteed still after pressing record.\n"
		           << "auto_calibrate_seconds=0\n"
		           << "gyro_bias_x=0\n"
		           << "gyro_bias_y=0\n"
		           << "gyro_bias_z=0\n"
			           << "# Optional Gyroflow rolling-shutter metadata. Direction: 0=top-bottom, 1=bottom-top.\n"
			           << "frame_readout_time_ms=0\n"
			           << "frame_readout_direction=0\n\n"
		           << "[bmi160]\n"
		           << "# Direct-polled Bosch BMI160 IMU. Use driver=bmi160 in [imu] to select it.\n"
		           << "enabled=false\n"
		           << "bus=1\n"
		           << "address=0x68\n"
		           << "sample_rate_hz=500\n"
		           << "gyro_range_dps=1000\n"
		           << "accel_range_g=8\n"
		           << "use_fifo=false\n"
		           << "orientation=XYZ\n"
		           << "level_reference_set=false\n"
		           << "level_reference_orientation=XYZ\n"
		           << "level_accel_x=0\n"
		           << "level_accel_y=0\n"
		           << "level_accel_z=0\n"
		           << "auto_calibrate_seconds=0\n"
		           << "gyro_bias_x=0\n"
		           << "gyro_bias_y=0\n"
		           << "gyro_bias_z=0\n"
		           << "frame_readout_time_ms=0\n"
		           << "frame_readout_direction=0\n\n"
		           << "[bmi270]\n"
		           << "# Direct-polled Bosch BMI270 IMU. Some boards need their Bosch config blob initialized by their driver.\n"
		           << "enabled=false\n"
		           << "bus=1\n"
		           << "address=0x68\n"
		           << "sample_rate_hz=500\n"
		           << "gyro_range_dps=1000\n"
		           << "accel_range_g=8\n"
		           << "use_fifo=false\n"
		           << "orientation=XYZ\n"
		           << "level_reference_set=false\n"
		           << "level_reference_orientation=XYZ\n"
		           << "level_accel_x=0\n"
		           << "level_accel_y=0\n"
		           << "level_accel_z=0\n"
		           << "auto_calibrate_seconds=0\n"
		           << "gyro_bias_x=0\n"
		           << "gyro_bias_y=0\n"
		           << "gyro_bias_z=0\n"
		           << "frame_readout_time_ms=0\n"
		           << "frame_readout_direction=0\n\n"
		           << "[icm42688p]\n"
		           << "# Direct-polled TDK InvenSense ICM42688-P IMU. Use driver=icm42688p in [imu] to select it.\n"
		           << "enabled=false\n"
		           << "bus=1\n"
		           << "address=0x68\n"
		           << "sample_rate_hz=500\n"
		           << "gyro_range_dps=1000\n"
		           << "accel_range_g=8\n"
		           << "use_fifo=false\n"
		           << "orientation=XYZ\n"
		           << "level_reference_set=false\n"
		           << "level_reference_orientation=XYZ\n"
		           << "level_accel_x=0\n"
		           << "level_accel_y=0\n"
		           << "level_accel_z=0\n"
		           << "auto_calibrate_seconds=0\n"
		           << "gyro_bias_x=0\n"
		           << "gyro_bias_y=0\n"
		           << "gyro_bias_z=0\n"
		           << "frame_readout_time_ms=0\n"
		           << "frame_readout_direction=0\n\n"
		           << "[cfexpress_hat]\n"
		           << "enabled=true\n"
           << "bus=10\n"
           << "address=0x34\n\n"
           << "[storage]\n"
           << "mount_path=/media/RAW\n"
           << "pcie_controller=1000110000.pcie\n\n"
           << "[display]\n"
           << "# Resolutions: 720x720(Default), 640x480, 800x480, 1280x720, 1024x600, 1920x1080\n"
           << "# Layouts:\n"
           << "# square (For 720x720)\n"
           << "# landscape_compact (For 640x480 and 800x480)\n"
           << "# landscape_medium (For 1024x600)\n"
           << "# landscape (For 1280x720)\n"
           << "# landscape_large (For 1920x1080)\n"
           << "# ui_rotation values: normal, left, right, upside_down\n"
           << "# Use left/right for native-portrait panels mounted in landscape, for example Raspberry Pi Touch Display 2.\n"
           << "ui_width=720\n"
           << "ui_height=720\n"
           << "ui_layout=square\n"
           << "ui_rotation=normal\n"
           << "controls_opacity=1.0\n"
           << "# Zoom button magnification: 1.0-4.0 (for example 1.5, 2, 3, 4).\n"
           << "# Restart the UI after changing this; pinch zoom keeps its 4x maximum.\n"
           << "zoom_toggle_factor=4.0\n"
           << "windowed=false\n"
           << "hdmi_external_only=false\n";

    return file.commit();
}

QString unquote(QString value)
{
    value = value.trimmed();
    if (value.size() >= 2 &&
        ((value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"'))) ||
         (value.startsWith(QLatin1Char('\'')) && value.endsWith(QLatin1Char('\''))))) {
        return value.mid(1, value.size() - 2).trimmed();
    }
    return value;
}

bool parseConfigInteger(const QVariant &value, int *parsed)
{
    if (!parsed || !value.isValid())
        return false;

    bool ok = false;
    const int direct = value.toInt(&ok);
    if (ok) {
        *parsed = direct;
        return true;
    }

    const int converted = unquote(value.toString()).toInt(&ok, 0);
    if (!ok)
        return false;

    *parsed = converted;
    return true;
}

int readConfigInteger(const QSettings &settings,
                      const QStringList &keys,
                      int fallback,
                      int minimum,
                      int maximum)
{
    for (const QString &key : keys) {
        if (!settings.contains(key))
            continue;

        int parsed = fallback;
        if (!parseConfigInteger(settings.value(key), &parsed))
            continue;

        return qBound(minimum, parsed, maximum);
    }

    return fallback;
}

bool parseConfigDouble(const QVariant &value, double *parsed)
{
    if (!parsed || !value.isValid())
        return false;

    bool ok = false;
    const double direct = value.toDouble(&ok);
    if (ok) {
        *parsed = direct;
        return true;
    }

    const double converted = unquote(value.toString()).toDouble(&ok);
    if (!ok)
        return false;

    *parsed = converted;
    return true;
}

double readConfigDouble(const QSettings &settings,
                        const QStringList &keys,
                        double fallback,
                        double minimum,
                        double maximum)
{
    for (const QString &key : keys) {
        if (!settings.contains(key))
            continue;

        double parsed = fallback;
        if (!parseConfigDouble(settings.value(key), &parsed) || !qIsFinite(parsed))
            continue;

        return qBound(minimum, parsed, maximum);
    }

    return fallback;
}

bool parseConfigBool(QString value, bool *ok)
{
    value = unquote(value).toLower();
    if (value == QStringLiteral("true") || value == QStringLiteral("yes") ||
        value == QStringLiteral("on") || value == QStringLiteral("1")) {
        if (ok)
            *ok = true;
        return true;
    }

    if (value == QStringLiteral("false") || value == QStringLiteral("no") ||
        value == QStringLiteral("off") || value == QStringLiteral("0")) {
        if (ok)
            *ok = true;
        return false;
    }

    if (ok)
        *ok = false;
    return false;
}

bool readConfigBool(const QSettings &settings,
                    const QStringList &keys,
                    bool fallback)
{
    for (const QString &key : keys) {
        if (!settings.contains(key))
            continue;

        bool parsed = false;
        const bool value = parseConfigBool(settings.value(key).toString(), &parsed);
        if (parsed)
            return value;
    }

    return fallback;
}

QString readConfigString(const QSettings &settings,
                         const QStringList &keys,
                         const QString &fallback)
{
    for (const QString &key : keys) {
        if (!settings.contains(key))
            continue;

        const QString value = unquote(settings.value(key).toString());
        if (!value.isEmpty())
            return value;
    }

    return fallback;
}

QString normalizedLayout(QString layout, int width, int height)
{
    layout = unquote(layout).trimmed().toLower();
    layout.replace(QLatin1Char('-'), QLatin1Char('_'));
    layout.replace(QLatin1Char(' '), QLatin1Char('_'));

    if (layout == QStringLiteral("square") ||
        layout == QStringLiteral("landscape_compact") ||
        layout == QStringLiteral("landscape_medium") ||
        layout == QStringLiteral("landscape") ||
        layout == QStringLiteral("landscape_large") ||
        layout == QStringLiteral("portrait_compact") ||
        layout == QStringLiteral("portrait") ||
        layout == QStringLiteral("portrait_large")) {
        return layout;
    }

    if (layout == QStringLiteral("landscape_small") || layout == QStringLiteral("small_landscape"))
        return QStringLiteral("landscape_compact");
    if (layout == QStringLiteral("medium_landscape"))
        return QStringLiteral("landscape_medium");
    if (layout == QStringLiteral("large_landscape"))
        return QStringLiteral("landscape_large");
    if (layout == QStringLiteral("compact") || layout == QStringLiteral("small"))
        return width > height ? QStringLiteral("landscape_compact") : QStringLiteral("portrait_compact");
    if (layout == QStringLiteral("small_portrait"))
        return QStringLiteral("portrait_compact");
    if (layout == QStringLiteral("large_portrait"))
        return QStringLiteral("portrait_large");

    if (width == height)
        return QStringLiteral("square");

    if (height > width) {
        if (width <= 540 || height <= 720)
            return QStringLiteral("portrait_compact");
        if (width >= 1000 || height >= 1800)
            return QStringLiteral("portrait_large");
        return QStringLiteral("portrait");
    }

    if (width > height) {
        if (height <= 540 || width <= 720)
            return QStringLiteral("landscape_compact");
        if (width >= 1800 || height >= 1000)
            return QStringLiteral("landscape_large");
        return QStringLiteral("landscape_medium");
    }

    return QStringLiteral("square");
}

int normalizedUiRotationDegrees(QString rotation)
{
    rotation = unquote(rotation).trimmed().toLower();
    rotation.replace(QLatin1Char('-'), QLatin1Char('_'));
    rotation.replace(QLatin1Char(' '), QLatin1Char('_'));

    if (rotation == QStringLiteral("left") ||
        rotation == QStringLiteral("left_side") ||
        rotation == QStringLiteral("counter_clockwise") ||
        rotation == QStringLiteral("counterclockwise") ||
        rotation == QStringLiteral("ccw") ||
        rotation == QStringLiteral("90") ||
        rotation == QStringLiteral("90deg") ||
        rotation == QStringLiteral("90_degrees")) {
        return 90;
    }

    if (rotation == QStringLiteral("right") ||
        rotation == QStringLiteral("right_side") ||
        rotation == QStringLiteral("clockwise") ||
        rotation == QStringLiteral("cw") ||
        rotation == QStringLiteral("-90") ||
        rotation == QStringLiteral("-90deg") ||
        rotation == QStringLiteral("-90_degrees") ||
        rotation == QStringLiteral("270") ||
        rotation == QStringLiteral("270deg") ||
        rotation == QStringLiteral("270_degrees")) {
        return -90;
    }

    if (rotation == QStringLiteral("upside_down") ||
        rotation == QStringLiteral("inverted") ||
        rotation == QStringLiteral("180") ||
        rotation == QStringLiteral("180deg") ||
        rotation == QStringLiteral("180_degrees")) {
        return 180;
    }

    return 0;
}

} // namespace

DisplayConfigBridge::DisplayConfigBridge(QObject *parent)
    : QObject(parent)
{
    loadConfiguration();
}

void DisplayConfigBridge::loadConfiguration()
{
    m_uiWidth = kDefaultUiWidth;
    m_uiHeight = kDefaultUiHeight;
    m_uiLayout = QStringLiteral("square");
    m_uiRotationDegrees = kDefaultUiRotationDegrees;
    m_controlsOpacity = kDefaultControlsOpacity;
    m_zoomToggleFactor = kDefaultZoomToggleFactor;
    m_windowed = false;
    m_hdmiExternalOnly = false;
    m_configPath.clear();

    ensureDefaultHardwareConfigFile();

    const QStringList candidates = hardwareConfigCandidatePaths();
    for (const QString &path : candidates) {
        const QFileInfo info(path);
        if (!info.exists() || !info.isFile())
            continue;

        QSettings settings(path, QSettings::IniFormat);
        settings.beginGroup(QStringLiteral("display"));

        m_uiWidth = readConfigInteger(settings,
                                      {QStringLiteral("ui_width"),
                                       QStringLiteral("uiWidth"),
                                       QStringLiteral("width")},
                                      kDefaultUiWidth,
                                      320,
                                      3840);
        m_uiHeight = readConfigInteger(settings,
                                       {QStringLiteral("ui_height"),
                                        QStringLiteral("uiHeight"),
                                        QStringLiteral("height")},
                                       kDefaultUiHeight,
                                       320,
                                       3840);
        const QString requestedLayout = readConfigString(settings,
                                                         {QStringLiteral("ui_layout"),
                                                          QStringLiteral("uiLayout"),
                                                          QStringLiteral("layout"),
                                                          QStringLiteral("profile")},
                                                         QStringLiteral("auto"));
        m_uiLayout = normalizedLayout(requestedLayout, m_uiWidth, m_uiHeight);
        const QString requestedRotation = readConfigString(settings,
                                                           {QStringLiteral("ui_rotation"),
                                                            QStringLiteral("uiRotation"),
                                                            QStringLiteral("rotation"),
                                                            QStringLiteral("orientation")},
                                                           QStringLiteral("normal"));
        m_uiRotationDegrees = normalizedUiRotationDegrees(requestedRotation);
        m_controlsOpacity = readConfigDouble(settings,
                                             {QStringLiteral("controls_opacity"),
                                              QStringLiteral("control_opacity"),
                                              QStringLiteral("controlsOpacity"),
                                              QStringLiteral("opacity")},
                                             kDefaultControlsOpacity,
                                             0.15,
                                             1.0);
        m_zoomToggleFactor = readConfigDouble(settings,
                                             {QStringLiteral("zoom_toggle_factor")},
                                             kDefaultZoomToggleFactor,
                                             1.0,
                                             4.0);
        m_windowed = readConfigBool(settings,
                                    {QStringLiteral("windowed"),
                                     QStringLiteral("ui_windowed"),
                                     QStringLiteral("uiWindowed"),
                                     QStringLiteral("debug_windowed"),
                                     QStringLiteral("debugWindowed")},
                                    false);
        m_hdmiExternalOnly = readConfigBool(settings,
                                           {QStringLiteral("hdmi_external_only"),
                                            QStringLiteral("hdmiExternalOnly")},
                                           false);
        settings.endGroup();
        m_configPath = path;
        break;
    }
}
