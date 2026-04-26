#pragma once

#include <QObject>
#include <QTimer>

class Ina219Bridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool sensorAvailable READ sensorAvailable NOTIFY sensorAvailableChanged)
    Q_PROPERTY(double busVoltageV READ busVoltageV NOTIFY busVoltageVChanged)
    Q_PROPERTY(double currentA READ currentA NOTIFY currentAChanged)
    Q_PROPERTY(double powerW READ powerW NOTIFY powerWChanged)
    Q_PROPERTY(int batteryPercent READ batteryPercent NOTIFY batteryPercentChanged)
    Q_PROPERTY(QString voltageText READ voltageText NOTIFY busVoltageVChanged)
    Q_PROPERTY(QString currentText READ currentText NOTIFY currentAChanged)
    Q_PROPERTY(QString powerText READ powerText NOTIFY powerWChanged)
    Q_PROPERTY(QString batteryPercentText READ batteryPercentText NOTIFY batteryPercentChanged)

public:
    explicit Ina219Bridge(int busNumber = 4, int address = 0x40, QObject *parent = nullptr);
    ~Ina219Bridge() override;

    bool sensorAvailable() const { return m_sensorAvailable; }
    double busVoltageV() const { return m_busVoltageV; }
    double currentA() const { return m_currentA; }
    double powerW() const { return m_powerW; }
    int batteryPercent() const { return m_batteryPercent; }

    QString voltageText() const;
    QString currentText() const;
    QString powerText() const;
    QString batteryPercentText() const;

signals:
    void sensorAvailableChanged();
    void busVoltageVChanged();
    void currentAChanged();
    void powerWChanged();
    void batteryPercentChanged();

private slots:
    void updateReadings();

private:
    bool ensureConnected();
    void disconnectDevice();
    bool writeRegister(quint8 reg, quint16 value);
    bool readRegister(quint8 reg, quint16 &value);
    static int estimateBatteryPercent(double voltageV, double currentA);

    const int m_busNumber = 4;
    const int m_address = 0x40;

    int m_fd = -1;
    QTimer m_timer;

    bool m_sensorAvailable = false;
    double m_busVoltageV = 0.0;
    double m_currentA = 0.0;
    double m_powerW = 0.0;
    int m_batteryPercent = 0;
};
