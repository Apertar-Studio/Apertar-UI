#pragma once

#include <QObject>
#include <QString>

class QSocketNotifier;
class QTimer;

class PowerButtonBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(bool pressed READ pressed NOTIFY pressedChanged)
    Q_PROPERTY(QString devicePath READ devicePath NOTIFY devicePathChanged)

public:
    explicit PowerButtonBridge(QObject *parent = nullptr);
    ~PowerButtonBridge() override;

    bool available() const { return m_available; }
    bool pressed() const { return m_pressed; }
    QString devicePath() const { return m_devicePath; }

signals:
    void availableChanged();
    void pressedChanged();
    void devicePathChanged();
    void buttonPressed();
    void buttonReleased();

private:
    void scanForDevice();
    void readEvents();
    bool openDevice(const QString &path);
    QString detectPowerButtonEventPath() const;
    void closeDevice();
    void setAvailable(bool available);
    void setPressed(bool pressed);
    void setDevicePath(const QString &path);

    int m_deviceFd = -1;
    QSocketNotifier *m_notifier = nullptr;
    QTimer *m_scanTimer = nullptr;
    bool m_available = false;
    bool m_pressed = false;
    QString m_devicePath;
};
