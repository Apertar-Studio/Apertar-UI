#pragma once

#include <QObject>
#include <QPointer>
#include <QTimer>

class QQmlApplicationEngine;
class QQuickWindow;
class QScreen;
class CameraPreviewItem;
class PreviewWindowRenderer;
class SettingsBridge;
class DisplayCalibrationBridge;

class ExternalMonitorManager : public QObject
{
    Q_OBJECT

public:
    explicit ExternalMonitorManager(QQmlApplicationEngine *engine, QObject *parent = nullptr);

    void setHdmiExternalOnly(bool enabled);
    void setMainWindow(QQuickWindow *window);
    void setSettingsBridge(SettingsBridge *settings);
    void setDisplayCalibrationBridge(DisplayCalibrationBridge *bridge);

private slots:
    void syncWindow();
    void syncShutdownPopupState();

private:
    bool mainUiAllowsExternalMonitor() const;
    QScreen *externalScreen() const;
    void ensureWindow(QScreen *screen);
    void destroyWindow();

    QPointer<QQmlApplicationEngine> m_engine;
    QPointer<QQuickWindow> m_mainWindow;
    QPointer<SettingsBridge> m_settings;
    QPointer<QQuickWindow> m_externalWindow;
    QPointer<CameraPreviewItem> m_externalPreviewItem;
    PreviewWindowRenderer *m_renderer = nullptr;
    QTimer m_shutdownPopupTimer;
    bool m_hdmiExternalOnly = false;
    QPointer<DisplayCalibrationBridge> m_displayCalibration;
};
