#pragma once
#include <QObject>
#include <QMap>
#include <QPointer>
#include <QVector>
class QScreen;
class QWindow;
class DisplayStandbyController : public QObject
{
public:
    explicit DisplayStandbyController(QObject *parent = nullptr);
    ~DisplayStandbyController() override;
    void setStandby(bool standby);
private:
    void turnOffHdmi();
    void restoreHdmi();
    bool m_standby = false;
    QMap<QScreen *, int> m_previousPower;
    QVector<QPointer<QWindow>> m_hiddenWindows;
};
