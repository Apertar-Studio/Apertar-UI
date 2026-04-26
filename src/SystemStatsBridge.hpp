#pragma once

#include <QObject>
#include <QTimer>

class SystemStatsBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double cpuPercent READ cpuPercent NOTIFY cpuPercentChanged)
    Q_PROPERTY(double socTempC READ socTempC NOTIFY socTempCChanged)
    Q_PROPERTY(QString cpuText READ cpuText NOTIFY cpuTextChanged)

public:
    explicit SystemStatsBridge(QObject *parent = nullptr);

    double cpuPercent() const { return m_cpuPercent; }
    double socTempC() const { return m_socTempC; }
    QString cpuText() const;

signals:
    void cpuPercentChanged();
    void socTempCChanged();
    void cpuTextChanged();

private slots:
    void updateStats();

private:
    bool readCpuTimes(qulonglong &idle, qulonglong &total) const;
    double readSocTemp() const;

    QTimer m_timer;

    qulonglong m_prevIdle = 0;
    qulonglong m_prevTotal = 0;
    bool m_havePrevCpuSample = false;

    double m_cpuPercent = 0.0;
    double m_socTempC = 0.0;
};