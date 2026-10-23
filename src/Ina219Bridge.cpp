#include "Ina219Bridge.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSettings>
#include <QTextStream>
#include <QByteArray>
#include <QDebug>
#include <QtGlobal>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace
{
constexpr quint8 kRegConfig = 0x00;
constexpr quint8 kRegShuntVoltage = 0x01;
constexpr quint8 kRegBusVoltage = 0x02;
constexpr quint8 kRegPower = 0x03;
constexpr quint8 kRegCurrent = 0x04;
constexpr quint8 kRegCalibration = 0x05;

constexpr quint16 kConfig32V3A = 0x399F;
constexpr quint16 kCalibration32V3A = 4096;

constexpr double kCurrentLsbAmps = 0.0001;
constexpr double kPowerLsbWatts = 0.002;
constexpr double kShuntMillivoltsPerBit = 0.01;
constexpr double kShuntOhms = 0.1;
constexpr double kVoltageCalibrationOffset = 0.08;
// Tuned for the SmallRig VB50's 4S mini V-mount behavior. We compensate
// some of the pack/cable/load sag so the displayed estimate better matches
// the battery's own onboard gauge under typical camera draw.
constexpr double kEstimatedPackSagOhms = 0.26;

constexpr int kDefaultIna219BusNumber = 4;
constexpr int kDefaultIna219Address = 0x40;

QString defaultHardwareConfigPath()
{
    return QStringLiteral("/home/pi/apertar-hardware.conf");
}

QStringList hardwareConfigCandidatePaths()
{
    QStringList candidates;

    const QString envPath = qEnvironmentVariable("APERTAR_HARDWARE_CONFIG");
    if (!envPath.trimmed().isEmpty())
        candidates.push_back(envPath.trimmed());

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
           << "# INA219 I2C bus example: /dev/i2c-4 -> bus=4\n"
           << "# Address can be decimal or hex.\n"
	           << "[ina219]\n"
	           << "enabled=true\n"
	           << "bus=" << kDefaultIna219BusNumber << "\n"
	           << "address=0x" << QString::number(kDefaultIna219Address, 16) << "\n\n"
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
           << "windowed=false\n"
           << "hdmi_external_only=false\n";

    return file.commit();
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

    QString text = value.toString().trimmed();
    if (text.isEmpty())
        return false;

    if (text.startsWith(QLatin1Char('"')) && text.endsWith(QLatin1Char('"')) && text.size() >= 2)
        text = text.mid(1, text.size() - 2);

    const int converted = text.toInt(&ok, 0);
    if (!ok)
        return false;

    *parsed = converted;
    return true;
}

bool parseConfigBool(const QVariant &value, bool fallback)
{
    if (!value.isValid())
        return fallback;

    QString text = value.toString().trimmed();
    if (text.startsWith(QLatin1Char('"')) && text.endsWith(QLatin1Char('"')) && text.size() >= 2)
        text = text.mid(1, text.size() - 2);

    const int commentStart = [&text]() {
        const int hash = text.indexOf(QLatin1Char('#'));
        const int semi = text.indexOf(QLatin1Char(';'));
        if (hash < 0)
            return semi;
        if (semi < 0)
            return hash;
        return std::min(hash, semi);
    }();
    if (commentStart >= 0)
        text.truncate(commentStart);

    text = text.trimmed().toLower();
    if (text == QStringLiteral("1") ||
        text == QStringLiteral("true") ||
        text == QStringLiteral("yes") ||
        text == QStringLiteral("on") ||
        text == QStringLiteral("enabled"))
        return true;

    if (text == QStringLiteral("0") ||
        text == QStringLiteral("false") ||
        text == QStringLiteral("no") ||
        text == QStringLiteral("off") ||
        text == QStringLiteral("disabled"))
        return false;

    return fallback;
}

int readConfigInteger(const QSettings &settings,
                      const QStringList &keys,
                      int fallbackValue,
                      int minimumValue,
                      int maximumValue,
                      const char *label,
                      const QString &configPath)
{
    for (const QString &key : keys) {
        const QVariant value = settings.value(key);
        int parsed = fallbackValue;
        if (!parseConfigInteger(value, &parsed))
            continue;

        if (parsed < minimumValue || parsed > maximumValue) {
            qWarning().noquote()
                << QStringLiteral("INA219 config in %1 has %2=%3 outside %4-%5, using %6.")
                       .arg(configPath,
                            QString::fromUtf8(label),
                            value.toString(),
                            QString::number(minimumValue),
                            QString::number(maximumValue),
                            QString::number(fallbackValue));
            return fallbackValue;
        }

        return parsed;
    }

    return fallbackValue;
}
}

Ina219Bridge::Ina219Bridge(int busNumber, int address, QObject *parent)
    : QObject(parent),
      m_busNumber(busNumber),
      m_address(address)
{
    connect(&m_timer, &QTimer::timeout, this, &Ina219Bridge::updateReadings);
    QTimer::singleShot(5000, this, &Ina219Bridge::startDeferredMonitoring);
}

Ina219Bridge::~Ina219Bridge()
{
    disconnectDevice();
}

void Ina219Bridge::startDeferredMonitoring()
{
    if (m_monitoringStarted)
        return;

    m_monitoringStarted = true;
    loadConfiguration(m_busNumber, m_address);
    if (m_enabled) {
        m_timer.start(1000);
        updateReadings();
    }
}

QString Ina219Bridge::voltageText() const
{
    return QString::number(m_busVoltageV, 'f', 1) + "V";
}

QString Ina219Bridge::currentText() const
{
    return QString::number(m_currentA, 'f', 1) + "A";
}

QString Ina219Bridge::powerText() const
{
    return QString::number(m_powerW, 'f', m_powerW >= 100.0 ? 0 : 1) + "W";
}

QString Ina219Bridge::batteryPercentText() const
{
    return QString::number(m_batteryPercent) + "%";
}

void Ina219Bridge::loadConfiguration(int defaultBusNumber, int defaultAddress)
{
    m_busNumber = defaultBusNumber;
    m_address = defaultAddress;
    m_configPath.clear();
    m_enabled = true;

    ensureDefaultHardwareConfigFile();

    const QStringList candidates = hardwareConfigCandidatePaths();
    for (const QString &path : candidates) {
        const QFileInfo info(path);
        if (!info.exists() || !info.isFile())
            continue;

        QSettings settings(path, QSettings::IniFormat);
        settings.beginGroup(QStringLiteral("ina219"));
        const bool enabled = parseConfigBool(settings.value(QStringLiteral("enabled")), true);

        const int busNumber = readConfigInteger(settings,
                                                {QStringLiteral("bus"),
                                                 QStringLiteral("channel"),
                                                 QStringLiteral("i2c_bus")},
                                                defaultBusNumber,
                                                0,
                                                255,
                                                "bus",
                                                path);
        const int address = readConfigInteger(settings,
                                              {QStringLiteral("address"),
                                               QStringLiteral("addr")},
                                              defaultAddress,
                                              0x03,
                                              0x77,
                                              "address",
                                              path);
        settings.endGroup();

        m_busNumber = busNumber;
        m_address = address;
        m_configPath = path;
        m_enabled = enabled;

        qInfo().noquote()
            << QStringLiteral("INA219 config: %1 (enabled=%2, bus=%3, address=0x%4)")
                   .arg(path,
                        m_enabled ? QStringLiteral("true") : QStringLiteral("false"),
                        QString::number(m_busNumber),
                        QString::number(m_address, 16));
        return;
    }

    m_configPath = defaultHardwareConfigPath();
    qInfo().noquote()
        << QStringLiteral("INA219 config: using defaults in %1 (bus=%2, address=0x%3)")
               .arg(m_configPath,
                    QString::number(m_busNumber),
                    QString::number(m_address, 16));
}

bool Ina219Bridge::ensureConnected()
{
    if (!m_enabled)
        return false;

    if (m_fd >= 0)
        return true;

    const QByteArray devicePath = QFile::encodeName(QString("/dev/i2c-%1").arg(m_busNumber));
    m_fd = ::open(devicePath.constData(), O_RDWR | O_CLOEXEC);
    if (m_fd < 0)
        return false;

    if (::ioctl(m_fd, I2C_SLAVE, m_address) < 0) {
        disconnectDevice();
        return false;
    }

    if (!writeRegister(kRegConfig, kConfig32V3A) || !writeRegister(kRegCalibration, kCalibration32V3A)) {
        disconnectDevice();
        return false;
    }

    return true;
}

void Ina219Bridge::disconnectDevice()
{
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
}

bool Ina219Bridge::writeRegister(quint8 reg, quint16 value)
{
    if (m_fd < 0)
        return false;

    const char buffer[3] = {
        static_cast<char>(reg),
        static_cast<char>((value >> 8) & 0xff),
        static_cast<char>(value & 0xff)
    };

    return ::write(m_fd, buffer, sizeof(buffer)) == sizeof(buffer);
}

bool Ina219Bridge::readRegister(quint8 reg, quint16 &value)
{
    if (m_fd < 0)
        return false;

    const char regByte = static_cast<char>(reg);
    if (::write(m_fd, &regByte, 1) != 1)
        return false;

    unsigned char data[2] = {0, 0};
    if (::read(m_fd, data, sizeof(data)) != sizeof(data))
        return false;

    value = (quint16(data[0]) << 8) | quint16(data[1]);
    return true;
}

int Ina219Bridge::estimateBatteryPercent(double voltageV, double currentA)
{
    struct Sample {
        double voltage;
        int percent;
    };

    const double compensatedVoltage = std::clamp(voltageV + (std::max(0.0, currentA) * kEstimatedPackSagOhms), 13.2, 16.8);

    static const std::array<Sample, 14> curve = {{
        {13.2, 0},
        {14.0, 5},
        {14.3, 10},
        {14.6, 20},
        {14.8, 32},
        {15.0, 42},
        {15.2, 52},
        {15.4, 60},
        {15.6, 68},
        {15.8, 76},
        {16.0, 84},
        {16.2, 91},
        {16.4, 95},
        {16.8, 100}
    }};

    if (compensatedVoltage <= curve.front().voltage)
        return curve.front().percent;
    if (compensatedVoltage >= curve.back().voltage)
        return curve.back().percent;

    for (std::size_t i = 1; i < curve.size(); ++i) {
        if (compensatedVoltage <= curve[i].voltage) {
            const double x0 = curve[i - 1].voltage;
            const double x1 = curve[i].voltage;
            const double t = (compensatedVoltage - x0) / (x1 - x0);
            const double interpolated = curve[i - 1].percent + t * (curve[i].percent - curve[i - 1].percent);
            return qBound(0, int(std::lround(interpolated)), 100);
        }
    }

    return 0;
}

void Ina219Bridge::updateReadings()
{
    if (!m_enabled) {
        disconnectDevice();
        if (m_sensorAvailable) {
            m_sensorAvailable = false;
            emit sensorAvailableChanged();
        }
        return;
    }

    if (!ensureConnected()) {
        if (m_sensorAvailable) {
            m_sensorAvailable = false;
            emit sensorAvailableChanged();
        }
        return;
    }

    // Re-apply calibration in case another tool reset the sensor.
    if (!writeRegister(kRegCalibration, kCalibration32V3A)) {
        disconnectDevice();
        if (m_sensorAvailable) {
            m_sensorAvailable = false;
            emit sensorAvailableChanged();
        }
        return;
    }

    quint16 busRaw = 0;
    quint16 shuntRaw = 0;
    quint16 currentRaw = 0;
    quint16 powerRaw = 0;

    if (!readRegister(kRegBusVoltage, busRaw) ||
        !readRegister(kRegShuntVoltage, shuntRaw) ||
        !readRegister(kRegCurrent, currentRaw) ||
        !readRegister(kRegPower, powerRaw)) {
        disconnectDevice();
        if (m_sensorAvailable) {
            m_sensorAvailable = false;
            emit sensorAvailableChanged();
        }
        return;
    }

    const double busVoltage = ((busRaw >> 3) * 4.0) / 1000.0;
    const auto signedShunt = static_cast<qint16>(shuntRaw);
    const auto signedCurrent = static_cast<qint16>(currentRaw);
    const double shuntMillivolts = signedShunt * kShuntMillivoltsPerBit;
    const double sourceVoltage = busVoltage + (shuntMillivolts / 1000.0);
    const double calibratedSourceVoltage = sourceVoltage + kVoltageCalibrationOffset;

    double currentAmps = signedCurrent * kCurrentLsbAmps;
    if (qFuzzyIsNull(currentAmps) && !qFuzzyIsNull(shuntMillivolts))
        currentAmps = (shuntMillivolts / 1000.0) / kShuntOhms;

    double powerWatts = powerRaw * kPowerLsbWatts;
    if (powerWatts <= 0.0 && calibratedSourceVoltage > 0.0 && currentAmps > 0.0)
        powerWatts = calibratedSourceVoltage * currentAmps;

    const int percent = estimateBatteryPercent(calibratedSourceVoltage, currentAmps);

    if (!m_sensorAvailable) {
        m_sensorAvailable = true;
        emit sensorAvailableChanged();
    }

    if (!qFuzzyCompare(1.0 + m_busVoltageV, 1.0 + calibratedSourceVoltage)) {
        m_busVoltageV = calibratedSourceVoltage;
        emit busVoltageVChanged();
    }

    if (!qFuzzyCompare(1.0 + m_currentA, 1.0 + currentAmps)) {
        m_currentA = currentAmps;
        emit currentAChanged();
    }

    if (!qFuzzyCompare(1.0 + m_powerW, 1.0 + powerWatts)) {
        m_powerW = powerWatts;
        emit powerWChanged();
    }

    if (m_batteryPercent != percent) {
        m_batteryPercent = percent;
        emit batteryPercentChanged();
    }
}
