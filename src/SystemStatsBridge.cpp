#include "SystemStatsBridge.hpp"

#include <QFile>
#include <QTextStream>
#include <QStringList>
SystemStatsBridge::SystemStatsBridge(QObject *parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &SystemStatsBridge::updateStats);
    m_timer.start(1000);

    updateStats();
}

QString SystemStatsBridge::cpuText() const
{
    return QString("%1% • %2°C")
        .arg(QString::number(m_cpuPercent, 'f', 1))
        .arg(QString::number(m_socTempC, 'f', 0));
}

bool SystemStatsBridge::readCpuTimes(qulonglong &idle, qulonglong &total) const
{
    QFile file("/proc/stat");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    const QByteArray line = file.readLine();
    if (!line.startsWith("cpu "))
        return false;

    const QString text = QString::fromLatin1(line).simplified();
    const QStringList parts = text.split(' ');

    if (parts.size() < 5)
        return false;

    // /proc/stat fields:
    // cpu user nice system idle iowait irq softirq steal guest guest_nice
    qulonglong user = parts.value(1).toULongLong();
    qulonglong nice = parts.value(2).toULongLong();
    qulonglong system = parts.value(3).toULongLong();
    qulonglong idleOnly = parts.value(4).toULongLong();
    qulonglong iowait = parts.value(5).toULongLong();
    qulonglong irq = parts.value(6).toULongLong();
    qulonglong softirq = parts.value(7).toULongLong();
    qulonglong steal = parts.value(8).toULongLong();

    idle = idleOnly + iowait;
    total = user + nice + system + idleOnly + iowait + irq + softirq + steal;
    return true;
}

double SystemStatsBridge::readSocTemp() const
{
    // Raspberry Pi usually exposes this here
    QFile file("/sys/class/thermal/thermal_zone0/temp");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return 0.0;

    const QString text = QString::fromLatin1(file.readAll()).trimmed();
    bool ok = false;
    const double milliC = text.toDouble(&ok);
    if (!ok)
        return 0.0;

    return milliC / 1000.0;
}

void SystemStatsBridge::updateStats()
{
    qulonglong idle = 0;
    qulonglong total = 0;

    if (readCpuTimes(idle, total)) {
        if (m_havePrevCpuSample) {
            const qulonglong idleDelta = idle - m_prevIdle;
            const qulonglong totalDelta = total - m_prevTotal;

            if (totalDelta > 0) {
                const double usage = 100.0 * (1.0 - (double(idleDelta) / double(totalDelta)));
                if (!qFuzzyCompare(m_cpuPercent, usage)) {
                    m_cpuPercent = usage;
                    emit cpuPercentChanged();
                    emit cpuTextChanged();
                }
            }
        }

        m_prevIdle = idle;
        m_prevTotal = total;
        m_havePrevCpuSample = true;
    }

    const double newTemp = readSocTemp();
    if (!qFuzzyCompare(m_socTempC, newTemp)) {
        m_socTempC = newTemp;
        emit socTempCChanged();
        emit cpuTextChanged();
    }
}
