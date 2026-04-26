#pragma once

#include <QObject>
#include <QTimer>

class DeviceInfoBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString sensorName READ sensorName NOTIFY infoChanged)
    Q_PROPERTY(QString backendName READ backendName CONSTANT)
    Q_PROPERTY(QString piModel READ piModel NOTIFY infoChanged)
    Q_PROPERTY(QString serialNumber READ serialNumber NOTIFY infoChanged)
    Q_PROPERTY(QString osVersion READ osVersion NOTIFY infoChanged)
    Q_PROPERTY(QString kernelVersion READ kernelVersion NOTIFY infoChanged)
    Q_PROPERTY(QString uptimeText READ uptimeText NOTIFY infoChanged)
    Q_PROPERTY(QString ipAddress READ ipAddress NOTIFY infoChanged)
    Q_PROPERTY(QString hostname READ hostname NOTIFY infoChanged)
    Q_PROPERTY(QString ramTotalText READ ramTotalText NOTIFY infoChanged)
    Q_PROPERTY(QString ramAvailableText READ ramAvailableText NOTIFY infoChanged)
    Q_PROPERTY(QString systemStorageTotalText READ systemStorageTotalText NOTIFY infoChanged)
    Q_PROPERTY(QString systemStorageFreeText READ systemStorageFreeText NOTIFY infoChanged)
    Q_PROPERTY(QString systemStorageMountPoint READ systemStorageMountPoint NOTIFY infoChanged)

public:
    explicit DeviceInfoBridge(QObject *parent = nullptr);

    QString sensorName() const { return m_sensorName; }
    QString backendName() const { return QStringLiteral("Apertar-Core / libcamera"); }
    QString piModel() const { return m_piModel; }
    QString serialNumber() const { return m_serialNumber; }
    QString osVersion() const { return m_osVersion; }
    QString kernelVersion() const { return m_kernelVersion; }
    QString uptimeText() const { return m_uptimeText; }
    QString ipAddress() const { return m_ipAddress; }
    QString hostname() const { return m_hostname; }
    QString ramTotalText() const { return m_ramTotalText; }
    QString ramAvailableText() const { return m_ramAvailableText; }
    QString systemStorageTotalText() const { return m_systemStorageTotalText; }
    QString systemStorageFreeText() const { return m_systemStorageFreeText; }
    QString systemStorageMountPoint() const { return m_systemStorageMountPoint; }

signals:
    void infoChanged();

private slots:
    void refreshDynamicInfo();

private:
    void detectStaticInfo();
    QString readTextFile(const QString &path) const;
    QString detectSensorName() const;
    QString scanForSensorTokenInDirectory(const QString &rootPath, int maxFiles) const;
    QString scanBinaryFileForSensorToken(const QString &filePath, qint64 maxBytes) const;
    QString prettySensorName(const QString &token) const;
    QString formatBytes(qulonglong bytes) const;
    QString formatUptime(qulonglong uptimeSeconds) const;
    QString primaryIpv4Address() const;
    qulonglong readMemInfoValueKb(const QString &key) const;

    QTimer m_refreshTimer;

    QString m_sensorName = QStringLiteral("Unknown Sensor");
    QString m_piModel = QStringLiteral("Raspberry Pi 5");
    QString m_serialNumber = QStringLiteral("Unavailable");
    QString m_osVersion = QStringLiteral("Unavailable");
    QString m_kernelVersion = QStringLiteral("Unavailable");
    QString m_uptimeText = QStringLiteral("Unavailable");
    QString m_ipAddress = QStringLiteral("Unavailable");
    QString m_hostname = QStringLiteral("Unavailable");
    QString m_ramTotalText = QStringLiteral("Unavailable");
    QString m_ramAvailableText = QStringLiteral("Unavailable");
    QString m_systemStorageTotalText = QStringLiteral("Unavailable");
    QString m_systemStorageFreeText = QStringLiteral("Unavailable");
    QString m_systemStorageMountPoint = QStringLiteral("/");
};
