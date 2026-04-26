#include "Ina219Bridge.hpp"

#include <QFile>
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
}

Ina219Bridge::Ina219Bridge(int busNumber, int address, QObject *parent)
    : QObject(parent),
      m_busNumber(busNumber),
      m_address(address)
{
    connect(&m_timer, &QTimer::timeout, this, &Ina219Bridge::updateReadings);
    m_timer.start(1000);
    updateReadings();
}

Ina219Bridge::~Ina219Bridge()
{
    disconnectDevice();
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

bool Ina219Bridge::ensureConnected()
{
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
