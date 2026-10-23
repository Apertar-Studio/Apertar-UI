#include <QGuiApplication>
#include <QFile>
#include <QElapsedTimer>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QRect>
#include <QScreen>
#include <QDebug>
#include <QSurfaceFormat>
#include <QSGRendererInterface>
#include <QTimer>
#include <QUrl>
#include <QByteArray>
#include <QVariant>

#include "PreviewWindowRenderer.hpp"
#include "CameraPreviewItem.hpp"
#include "ApertarPreviewSocketClient.hpp"
#include "SystemStatsBridge.hpp"
#include "DeviceInfoBridge.hpp"
#include "DisplayConfigBridge.hpp"
#include "DisplayCalibration.hpp"
#include "SystemActionBridge.hpp"
#include "PowerButtonBridge.hpp"
#include "SleepManager.hpp"
#include "DisplayStandbyController.hpp"
#include "MediaStatusBridge.hpp"
#include "Ina219Bridge.hpp"
#include "GyroConfigBridge.hpp"
#include "WifiBridge.hpp"
#include "AudioDeviceBridge.hpp"
#include "AudioMeterBridge.hpp"
#include "AudioMixerBridge.hpp"
#include "ExternalMonitorManager.hpp"
#include "ExternalMonitorRuntimeBridge.hpp"
#include "ApertarControlBridge.hpp"
#include "SettingsBridge.hpp"
#include "cliplistmodel.h"
#include "playbackcontroller.h"
#include "VideoPlayback.hpp"
#include "cdngimageprovider.h"

namespace {

bool screenIsHdmi(const QScreen *screen)
{
    return screen && screen->name().contains(QStringLiteral("HDMI"), Qt::CaseInsensitive);
}

bool environmentFlagEnabled(const char *name)
{
    const QByteArray value = qgetenv(name).trimmed().toLower();
    return !value.isEmpty()
        && value != QByteArrayLiteral("0")
        && value != QByteArrayLiteral("false")
        && value != QByteArrayLiteral("off")
        && value != QByteArrayLiteral("no");
}

bool screenLooksLikeInternalPanel(const QScreen *screen)
{
    if (!screen || screenIsHdmi(screen))
        return false;

    const QString name = screen->name().toLower();
    const QSize size = screen->geometry().size();
    return name.contains(QStringLiteral("dpi"))
           || name.contains(QStringLiteral("dsi"))
           || name.contains(QStringLiteral("lcd"))
           || name.contains(QStringLiteral("panel"))
           || (size.width() == 720 && size.height() == 720);
}

bool internalTouchPanelPresent()
{
    QFile devices(QStringLiteral("/proc/bus/input/devices"));
    if (!devices.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    const QString text = QString::fromLatin1(devices.readAll()).toLower();
    return text.contains(QStringLiteral("touch"))
           || text.contains(QStringLiteral("touchscreen"))
           || text.contains(QStringLiteral("goodix"))
           || text.contains(QStringLiteral("ep0110"))
           || text.contains(QStringLiteral("hyperpixel"))
           || text.contains(QStringLiteral("waveshare"));
}

QScreen *mainUiScreenForNormalMode()
{
    const QList<QScreen *> screens = QGuiApplication::screens();
    if (screens.isEmpty())
        return nullptr;
    if (screens.size() == 1)
        return screens.first();

    QScreen *firstHdmi = nullptr;
    QScreen *firstInternalPanel = nullptr;
    for (QScreen *screen : screens) {
        if (!screen)
            continue;
        if (!firstHdmi && screenIsHdmi(screen))
            firstHdmi = screen;
        if (!firstInternalPanel && screenLooksLikeInternalPanel(screen))
            firstInternalPanel = screen;
    }

    if (firstInternalPanel && (internalTouchPanelPresent() || !firstHdmi))
        return firstInternalPanel;

    if (firstHdmi)
        return firstHdmi;

    if (QScreen *primary = QGuiApplication::primaryScreen())
        return primary;

    return screens.first();
}

QScreen *externalMonitorScreen()
{
    const QList<QScreen *> screens = QGuiApplication::screens();
    for (QScreen *screen : screens) {
        if (screenIsHdmi(screen))
            return screen;
    }
    return nullptr;
}

int runMonitorOnly(QGuiApplication &app)
{
    DisplayConfigBridge displayConfig;
    ApertarPreviewSocketClient apertarPreviewBridge;
    ApertarControlBridge apertarControlBridge;
    DisplayStandbyController displayStandby;
    QObject::connect(&apertarControlBridge, &ApertarControlBridge::standbyChanged,
                     &displayStandby, [&]() { displayStandby.setStandby(apertarControlBridge.standby()); });
    SettingsBridge settings;
    settings.setControlBridge(&apertarControlBridge);

    ExternalMonitorRuntimeBridge monitorRuntimeBridge(
        ExternalMonitorRuntimeBridge::Mode::Subscriber);
    DisplayCalibrationBridge displayCalibration;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("apertarPreviewBridge", &apertarPreviewBridge);
    engine.rootContext()->setContextProperty("apertarControlBridge", &apertarControlBridge);
    engine.rootContext()->setContextProperty("settingsBridge", &settings);
    engine.rootContext()->setContextProperty("mediaBridge", &monitorRuntimeBridge);
    engine.rootContext()->setContextProperty("displayConfigBridge", &displayConfig);
    engine.rootContext()->setContextProperty("monitorRuntimeBridge", &monitorRuntimeBridge);
    engine.rootContext()->setContextProperty(
        "audioMeterBridge", QVariant::fromValue(static_cast<QObject *>(nullptr)));
    engine.load(QUrl(QStringLiteral("qrc:/qml/ExternalMonitorWindow.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    if (!window)
        return -1;

    QScreen *screen = externalMonitorScreen();
    if (!screen) {
        qWarning() << "Apertar monitor-only mode could not find a DRM screen.";
        return -1;
    }

    // EGLFS requires the target screen to be selected before the window is shown.
    window->setScreen(screen);
    window->setGeometry(screen->geometry());
    window->setPersistentGraphics(true);
    window->setPersistentSceneGraph(true);

    auto *previewItem = window->findChild<CameraPreviewItem *>(
        QStringLiteral("externalPreviewItem"));
    PreviewWindowRenderer renderer(&engine);
    renderer.setDirectWindowRendering(true);
    renderer.setPreviewItem(previewItem);
    new DisplayCalibrationRenderer(window, &displayCalibration, true);

    auto syncVisibility = [window, &settings, &displayConfig, &monitorRuntimeBridge, &apertarControlBridge]() {
        if (apertarControlBridge.standby()) {
            window->hide();
            return;
        }
        QScreen *screen = externalMonitorScreen();
        if (!screen) {
            if (window->isVisible())
                window->hide();
            return;
        }

        if (window->screen() != screen) {
            if (window->isVisible())
                window->hide();
            window->setScreen(screen);
        }
        window->setGeometry(screen->geometry());

        const bool enabled = displayConfig.hdmiExternalOnly()
            || settings.externalMonitorEnabled();
        if (enabled && monitorRuntimeBridge.cameraPageActive()) {
            if (!window->isVisible())
                window->showFullScreen();
        } else if (window->isVisible()) {
            window->hide();
        }
    };

    QObject::connect(&settings, &SettingsBridge::externalMonitorEnabledChanged,
                     window, syncVisibility);
    QObject::connect(&apertarControlBridge, &ApertarControlBridge::standbyChanged,
                     window, syncVisibility);
    QObject::connect(&monitorRuntimeBridge,
                     &ExternalMonitorRuntimeBridge::cameraPageActiveChanged,
                     window, syncVisibility);
    QObject::connect(&app, &QGuiApplication::screenAdded,
                     window, [window, syncVisibility](QScreen *) {
                         QTimer::singleShot(0, window, syncVisibility);
                     });
    QObject::connect(&app, &QGuiApplication::screenRemoved,
                     window, [window, syncVisibility](QScreen *) {
                         QTimer::singleShot(0, window, syncVisibility);
                     });

    apertarPreviewBridge.connectToCore();
    syncVisibility();
    qInfo().noquote()
        << QStringLiteral("Apertar compositor-free monitor process: screen=%1 geometry=%2x%3")
               .arg(screen->name())
               .arg(screen->geometry().width())
               .arg(screen->geometry().height());

    return app.exec();
}

} // namespace

int main(int argc, char *argv[])
{
    QElapsedTimer startupTimer;
    startupTimer.start();
    qInfo().noquote()
        << QStringLiteral("Apertar UI startup: entered main qml_aot=%1")
               .arg(APERTAR_QML_AOT_ENABLED ? QStringLiteral("true")
                                             : QStringLiteral("false"));

    QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    const QByteArray platform = qgetenv("QT_QPA_PLATFORM").toLower();
    if (qEnvironmentVariableIsEmpty("QSG_RENDER_LOOP") && platform == QByteArrayLiteral("eglfs"))
        qputenv("QSG_RENDER_LOOP", QByteArrayLiteral("basic"));

    QSurfaceFormat fmt;
    fmt.setRenderableType(QSurfaceFormat::OpenGLES);
    fmt.setMajorVersion(2);
    fmt.setMinorVersion(0);
    fmt.setProfile(QSurfaceFormat::NoProfile);
    fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    fmt.setSwapInterval(1);
    QSurfaceFormat::setDefaultFormat(fmt);

    QGuiApplication app(argc, argv);
    qInfo().noquote()
        << QStringLiteral("Apertar UI startup: QGuiApplication ready in %1 ms")
               .arg(startupTimer.elapsed());
    app.setApplicationName(QStringLiteral("Apertar-UI"));
    app.setApplicationDisplayName(QStringLiteral("Apertar-UI"));

    qmlRegisterType<CameraPreviewItem>("Apertar", 1, 0, "CameraPreviewItem");
    if (environmentFlagEnabled("APERTAR_MONITOR_ONLY"))
        return runMonitorOnly(app);

    DisplayConfigBridge displayConfig;
    const bool hdmiExternalOnly = displayConfig.hdmiExternalOnly();
    qInfo().noquote()
        << QStringLiteral("Apertar display config: path=%1 size=%2x%3 layout=%4 rotation=%5 opacity=%6 windowed=%7 hdmi_external_only=%8")
               .arg(displayConfig.configPath())
               .arg(displayConfig.uiWidth())
               .arg(displayConfig.uiHeight())
               .arg(displayConfig.uiLayout())
               .arg(displayConfig.uiRotationDegrees())
               .arg(QString::number(displayConfig.controlsOpacity(), 'f', 2))
               .arg(displayConfig.windowed() ? QStringLiteral("true") : QStringLiteral("false"))
               .arg(displayConfig.hdmiExternalOnly() ? QStringLiteral("true") : QStringLiteral("false"));

    ApertarPreviewSocketClient apertarPreviewBridge;
    SystemStatsBridge statsBridge;
    DeviceInfoBridge deviceInfoBridge;
    SystemActionBridge systemActionBridge;
    PowerButtonBridge powerButtonBridge;
    SleepManager sleepManager;
    MediaStatusBridge mediaBridge;
    Ina219Bridge powerBridge;
    GyroConfigBridge gyroConfigBridge;
    WifiBridge wifiBridge;
    AudioDeviceBridge audioDeviceBridge;
    ApertarControlBridge apertarControlBridge;
    SettingsBridge settings;
    AudioMeterBridge audioMeterBridge;
    DisplayCalibrationBridge displayCalibration;
    AudioMixerBridge audioMixerBridge;
    ExternalMonitorRuntimeBridge monitorRuntimeBridge(
        ExternalMonitorRuntimeBridge::Mode::Publisher);
    ClipListModel clipModel;
    PlaybackController playbackController;

    qInfo().noquote()
        << QStringLiteral("Apertar UI startup: application bridges ready in %1 ms")
               .arg(startupTimer.elapsed());

    settings.setControlBridge(&apertarControlBridge);
    monitorRuntimeBridge.setMediaBridge(&mediaBridge);
    settings.setDefaultCameraControlsOpacity(displayConfig.controlsOpacity());
    audioDeviceBridge.setSettingsBridge(&settings);
    audioMeterBridge.setSettingsBridge(&settings);
    audioMeterBridge.setControlBridge(&apertarControlBridge);
    QObject::connect(&audioDeviceBridge, &AudioDeviceBridge::capabilityProbeActiveChanged,
                     &audioMeterBridge, &AudioMeterBridge::setCapabilityProbeActive);
    QObject::connect(&apertarControlBridge,
                     &ApertarControlBridge::externalRecordingPreparationRequested,
                     &audioMeterBridge,
                     &AudioMeterBridge::releaseInputForRecording);
    audioMixerBridge.setSettingsBridge(&settings);
    playbackController.setSettingsBridge(&settings);
    QObject::connect(&sleepManager, &SleepManager::userActivity,
                     &apertarControlBridge, &ApertarControlBridge::notifyUserActivity);
    DisplayStandbyController displayStandby;
    QObject::connect(&apertarControlBridge, &ApertarControlBridge::standbyChanged,
                     &displayStandby, [&]() { displayStandby.setStandby(apertarControlBridge.standby()); });
    QObject::connect(&apertarControlBridge, &ApertarControlBridge::standbyChanged, &sleepManager, [&]() {
        const bool standby = apertarControlBridge.standby();
        sleepManager.setStandby(standby);
        gyroConfigBridge.setStandby(standby);
        statsBridge.setStandby(standby);
        mediaBridge.setStandby(standby);
        if (standby) {
            playbackController.stop();
            audioMeterBridge.suspendMonitoring();
            apertarPreviewBridge.disconnectFromCore();
        } else {
            apertarPreviewBridge.connectToCore();
        }
    });

    QQmlApplicationEngine engine;
    auto *cdngProvider = new CdngImageProvider(false, false, false);
    auto *cdngThumbProvider = new CdngImageProvider(true, true, false, 96, 8, 24ll * 1024ll * 1024ll);
    auto *cdngPlayProvider = new CdngImageProvider(false, true, true, 32, 8, 32ll * 1024ll * 1024ll);
    engine.addImageProvider(QLatin1String("cdng"), cdngProvider);
    engine.addImageProvider(QLatin1String("cdngthumb"), cdngThumbProvider);
    engine.addImageProvider(QLatin1String("cdngplay"), cdngPlayProvider);
    auto *videoProvider = new VideoImageProvider;
    auto *videoThumbProvider = new VideoImageProvider(true);
    engine.addImageProvider(QLatin1String("video"), videoProvider);
    engine.addImageProvider(QLatin1String("videothumb"), videoThumbProvider);
    videoProvider->setActive(false);
    QObject::connect(videoProvider, &VideoImageProvider::frameDecoded,
                     &playbackController, &PlaybackController::acceptVideoFrame, Qt::QueuedConnection);
    QObject::connect(videoProvider, &VideoImageProvider::decodeFailed,
                     &playbackController, &PlaybackController::videoDecodeFailed, Qt::QueuedConnection);
    QObject::connect(&apertarControlBridge, &ApertarControlBridge::standbyChanged, &engine, [&]() {
        if (apertarControlBridge.standby()) {
            cdngProvider->clearCache();
            cdngThumbProvider->clearCache();
            cdngPlayProvider->clearCache();
            videoProvider->setActive(false);
        }
    });
    engine.rootContext()->setContextProperty("apertarPreviewBridge", &apertarPreviewBridge);
    engine.rootContext()->setContextProperty("statsBridge", &statsBridge);
    engine.rootContext()->setContextProperty("deviceInfoBridge", &deviceInfoBridge);
    engine.rootContext()->setContextProperty("displayConfigBridge", &displayConfig);
    engine.rootContext()->setContextProperty("systemActionBridge", &systemActionBridge);
    engine.rootContext()->setContextProperty("displayCalibrationBridge", &displayCalibration);
    engine.rootContext()->setContextProperty("powerButtonBridge", &powerButtonBridge);
    engine.rootContext()->setContextProperty("sleepManager", &sleepManager);
    engine.rootContext()->setContextProperty("mediaBridge", &mediaBridge);
    engine.rootContext()->setContextProperty("powerBridge", &powerBridge);
    engine.rootContext()->setContextProperty("gyroConfigBridge", &gyroConfigBridge);
    engine.rootContext()->setContextProperty("wifiBridge", &wifiBridge);
    engine.rootContext()->setContextProperty("audioDeviceBridge", &audioDeviceBridge);
    engine.rootContext()->setContextProperty("audioMeterBridge", &audioMeterBridge);
    engine.rootContext()->setContextProperty("apertarControlBridge", &apertarControlBridge);
    engine.rootContext()->setContextProperty("settingsBridge", &settings);
    engine.rootContext()->setContextProperty("hdmiExternalOnly", hdmiExternalOnly);
    engine.rootContext()->setContextProperty("clipModel", &clipModel);
    engine.rootContext()->setContextProperty("playbackController", &playbackController);
    engine.rootContext()->setContextProperty("cdngThumbProvider", cdngThumbProvider);
    engine.rootContext()->setContextProperty("cdngPlayProvider", cdngPlayProvider);
    engine.rootContext()->setContextProperty("videoProvider", videoProvider);
    engine.rootContext()->setContextProperty("monitorRuntimeBridge", &monitorRuntimeBridge);
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    qInfo().noquote()
        << QStringLiteral("Apertar UI startup: Main.qml loaded in %1 ms")
               .arg(startupTimer.elapsed());

    if (engine.rootObjects().isEmpty())
        return -1;

    const QString coreStartReadyPath =
        QString::fromLocal8Bit(qgetenv("APERTAR_CORE_START_READY_FILE"));
    if (!coreStartReadyPath.isEmpty()) {
        QFile readyFile(coreStartReadyPath);
        if (readyFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            readyFile.write("ready\n");
            readyFile.close();
            qInfo().noquote()
                << QStringLiteral("Apertar UI startup: Core start released in %1 ms")
                       .arg(startupTimer.elapsed());
        } else {
            qWarning().noquote()
                << QStringLiteral("Apertar UI startup: could not create Core start marker %1")
                       .arg(coreStartReadyPath);
        }
    }

    QObject *root = engine.rootObjects().first();
    if (auto *window = qobject_cast<QQuickWindow *>(root)) {
        new DisplayCalibrationRenderer(window, &displayCalibration, false);
        monitorRuntimeBridge.setMainWindow(window);
        window->setPersistentGraphics(true);
        window->setPersistentSceneGraph(true);
        if (hdmiExternalOnly) {
            window->hide();
        } else if (QScreen *mainUiScreen = mainUiScreenForNormalMode()) {
            if (window->screen() != mainUiScreen) {
                window->hide();
                window->setScreen(mainUiScreen);
            }
            if (displayConfig.windowed()) {
                const QRect screenGeometry = mainUiScreen->geometry();
                qInfo().noquote()
                    << QStringLiteral("Apertar layout test viewport: screen=%1 screen_geometry=%2,%3 %4x%5 viewport=%6x%7")
                           .arg(mainUiScreen->name(),
                                QString::number(screenGeometry.x()),
                                QString::number(screenGeometry.y()),
                                QString::number(screenGeometry.width()),
                                QString::number(screenGeometry.height()),
                                QString::number(displayConfig.uiWidth()),
                                QString::number(displayConfig.uiHeight()));
                window->setGeometry(screenGeometry);
                window->showFullScreen();
                QTimer::singleShot(250, window, [window]() {
                    if (!window)
                        return;
                    window->showFullScreen();
                    window->raise();
                    window->requestActivate();
                });
            } else {
                window->setGeometry(mainUiScreen->geometry());
                window->showFullScreen();
            }
        }
        if (window->contentItem())
            window->contentItem()->setAcceptTouchEvents(true);

        const QString readyPath = QString::fromLocal8Bit(qgetenv("APERTAR_UI_READY_FILE"));
        if (!readyPath.isEmpty()) {
            auto signalReady = [readyPath,
                                &startupTimer,
                                &deviceInfoBridge,
                                &powerBridge,
                                signaled = false]() mutable {
                if (signaled)
                    return;
                signaled = true;

                QFile readyFile(readyPath);
                if (readyFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                    readyFile.write("ready\n");
                    readyFile.close();
                } else {
                    qWarning().noquote()
                        << QStringLiteral("Apertar UI startup: could not create ready marker %1")
                               .arg(readyPath);
                }

                qInfo().noquote()
                    << QStringLiteral("Apertar UI startup: first frame presented in %1 ms")
                           .arg(startupTimer.elapsed());

                QTimer::singleShot(250, &deviceInfoBridge, [&deviceInfoBridge]() {
                    deviceInfoBridge.startDeferredRefresh();
                });
                QTimer::singleShot(250, &powerBridge, [&powerBridge]() {
                    powerBridge.startDeferredMonitoring();
                });
            };

            if (hdmiExternalOnly)
                QTimer::singleShot(0, window, signalReady);
            else
                QObject::connect(window, &QQuickWindow::frameSwapped, window, signalReady);
        }
    }
    auto *previewContainer = root->findChild<QQuickItem *>("previewContainer");
    auto *previewItem = root->findChild<CameraPreviewItem *>("previewItem");

    if (previewContainer)
        previewContainer->setAcceptTouchEvents(true);
    if (previewItem)
        previewItem->setAcceptTouchEvents(true);

    auto *renderer = new PreviewWindowRenderer(&engine);
    renderer->setPreviewItem(previewItem);

    ExternalMonitorManager externalMonitorManager(&engine, &engine);
    externalMonitorManager.setHdmiExternalOnly(hdmiExternalOnly && !displayConfig.windowed());
    externalMonitorManager.setMainWindow(qobject_cast<QQuickWindow *>(root));
    externalMonitorManager.setDisplayCalibrationBridge(&displayCalibration);
    if (!displayConfig.windowed())
        externalMonitorManager.setSettingsBridge(&settings);

    return app.exec();
}
