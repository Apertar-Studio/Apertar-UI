#include "ExternalMonitorManager.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QUrl>
#include <QDebug>

#include "CameraPreviewItem.hpp"
#include "PreviewWindowRenderer.hpp"
#include "SettingsBridge.hpp"
#include "DisplayCalibration.hpp"

namespace {

bool screenIsHdmi(const QScreen *screen)
{
    return screen && screen->name().contains(QStringLiteral("HDMI"), Qt::CaseInsensitive);
}

} // namespace

ExternalMonitorManager::ExternalMonitorManager(QQmlApplicationEngine *engine, QObject *parent)
    : QObject(parent),
      m_engine(engine)
{
    m_shutdownPopupTimer.setInterval(33);
    connect(&m_shutdownPopupTimer, &QTimer::timeout,
            this, &ExternalMonitorManager::syncShutdownPopupState);

    if (qGuiApp) {
        connect(qGuiApp, &QGuiApplication::screenAdded,
                this, [this](QScreen *) { syncWindow(); });
        connect(qGuiApp, &QGuiApplication::screenRemoved,
                this, [this](QScreen *) { syncWindow(); });
        connect(qGuiApp, &QGuiApplication::primaryScreenChanged,
                this, [this](QScreen *) { syncWindow(); });
    }
}

void ExternalMonitorManager::setHdmiExternalOnly(bool enabled)
{
    if (m_hdmiExternalOnly == enabled)
        return;

    m_hdmiExternalOnly = enabled;
    syncWindow();
}

void ExternalMonitorManager::setDisplayCalibrationBridge(DisplayCalibrationBridge *bridge)
{
    m_displayCalibration = bridge;
    if (m_externalWindow && bridge)
        new DisplayCalibrationRenderer(m_externalWindow, bridge, true);
}

void ExternalMonitorManager::setMainWindow(QQuickWindow *window)
{
    if (m_mainWindow == window)
        return;

    if (m_mainWindow)
        disconnect(m_mainWindow, nullptr, this, nullptr);

    m_mainWindow = window;

    if (m_mainWindow) {
        connect(m_mainWindow, &QQuickWindow::screenChanged,
                this, [this](QScreen *) { syncWindow(); });
        connect(m_mainWindow, SIGNAL(currentPageChanged()),
                this, SLOT(syncWindow()));
        connect(m_mainWindow, SIGNAL(standbyActiveChanged()),
                this, SLOT(syncWindow()));
        connect(m_mainWindow, SIGNAL(powerButtonShutdownOpenChanged()),
                this, SLOT(syncShutdownPopupState()));
        connect(m_mainWindow, SIGNAL(powerButtonShutdownTriggeredChanged()),
                this, SLOT(syncShutdownPopupState()));
        connect(m_mainWindow, SIGNAL(powerButtonShutdownHoldProgressChanged()),
                this, SLOT(syncShutdownPopupState()));
        connect(m_mainWindow, SIGNAL(powerButtonShutdownErrorTextChanged()),
                this, SLOT(syncShutdownPopupState()));
        connect(m_mainWindow, SIGNAL(powerButtonShutdownTouchPressedChanged()),
                this, SLOT(syncShutdownPopupState()));
    }

    syncWindow();
}

void ExternalMonitorManager::setSettingsBridge(SettingsBridge *settings)
{
    if (m_settings == settings)
        return;

    if (m_settings)
        disconnect(m_settings, nullptr, this, nullptr);

    m_settings = settings;

    if (m_settings) {
        connect(m_settings, &SettingsBridge::externalMonitorEnabledChanged,
                this, &ExternalMonitorManager::syncWindow);
    }

    syncWindow();
}

bool ExternalMonitorManager::mainUiAllowsExternalMonitor() const
{
    if (m_mainWindow && m_mainWindow->property("standbyActive").toBool())
        return false;
    if (m_hdmiExternalOnly)
        return true;

    if (!m_mainWindow)
        return false;

    const QVariant currentPage = m_mainWindow->property("currentPage");
    if (!currentPage.isValid())
        return true;

    return currentPage.toString() == QStringLiteral("camera");
}

QScreen *ExternalMonitorManager::externalScreen() const
{
    if (m_hdmiExternalOnly) {
        const QList<QScreen *> screens = QGuiApplication::screens();
        for (QScreen *screen : screens) {
            if (screenIsHdmi(screen))
                return screen;
        }

        if (QScreen *primary = QGuiApplication::primaryScreen())
            return primary;

        return screens.isEmpty() ? nullptr : screens.first();
    }

    if (!m_mainWindow)
        return nullptr;

    const QList<QScreen *> screens = QGuiApplication::screens();
    if (screens.size() <= 1)
        return nullptr;

    QScreen *mainScreen = m_mainWindow->screen();
    if (!mainScreen)
        mainScreen = QGuiApplication::primaryScreen();

    for (QScreen *screen : screens) {
        if (screen && screen != mainScreen && screenIsHdmi(screen))
            return screen;
    }

    return nullptr;
}

void ExternalMonitorManager::syncWindow()
{
    if (!m_engine || !m_mainWindow || !m_settings) {
        destroyWindow();
        return;
    }

    if (!mainUiAllowsExternalMonitor()) {
        destroyWindow();
        return;
    }

    if (!m_hdmiExternalOnly && !m_settings->externalMonitorEnabled()) {
        destroyWindow();
        return;
    }

    QScreen *screen = externalScreen();
    if (!screen) {
        destroyWindow();
        return;
    }

    ensureWindow(screen);
}

void ExternalMonitorManager::ensureWindow(QScreen *screen)
{
    if (!screen || !m_engine)
        return;

    if (!m_externalWindow) {
        QQmlComponent component(m_engine, QUrl(QStringLiteral("qrc:/qml/ExternalMonitorWindow.qml")));
        QObject *created = component.create(m_engine->rootContext());
        if (!created) {
            const QList<QQmlError> errors = component.errors();
            for (const QQmlError &error : errors)
                qWarning() << "External monitor QML error:" << error.toString();
            return;
        }

        auto *window = qobject_cast<QQuickWindow *>(created);
        if (!window) {
            qWarning() << "External monitor component did not create a QQuickWindow.";
            delete created;
            return;
        }

        m_externalWindow = window;
        if (m_displayCalibration)
            new DisplayCalibrationRenderer(window, m_displayCalibration, true);
        m_externalWindow->setPersistentGraphics(true);
        m_externalWindow->setPersistentSceneGraph(true);
        m_externalWindow->setColor(Qt::black);
        m_externalWindow->setFlags(Qt::FramelessWindowHint);
        m_externalWindow->hide();
        if (m_externalWindow->contentItem())
            m_externalWindow->contentItem()->setAcceptTouchEvents(false);

        connect(m_externalWindow, &QObject::destroyed,
                this, [this]() {
                    m_externalWindow = nullptr;
                    m_externalPreviewItem = nullptr;
                    m_renderer = nullptr;
                });

        connect(m_externalWindow, &QQuickWindow::screenChanged,
                this, [this](QScreen *) { syncWindow(); });

        m_externalPreviewItem = m_externalWindow->findChild<CameraPreviewItem *>(QStringLiteral("externalPreviewItem"));
        if (m_externalPreviewItem) {
            m_renderer = new PreviewWindowRenderer(m_externalWindow);
            m_renderer->setDirectWindowRendering(true);
            m_renderer->setPreviewItem(m_externalPreviewItem);
        } else {
            qWarning() << "Could not find external preview item in external monitor window.";
        }
    }

    if (!m_externalWindow)
        return;

    if (m_externalWindow->screen() != screen) {
        if (m_externalWindow->isVisible())
            m_externalWindow->hide();
        m_externalWindow->setScreen(screen);
    }

    m_externalWindow->setGeometry(screen->geometry());

    if (!m_externalWindow->isVisible()) {
        m_externalWindow->showFullScreen();
    } else if (m_externalWindow->visibility() != QWindow::FullScreen) {
        m_externalWindow->showFullScreen();
    }

    if (!m_shutdownPopupTimer.isActive())
        m_shutdownPopupTimer.start();

    syncShutdownPopupState();
}

void ExternalMonitorManager::destroyWindow()
{
    if (!m_externalWindow)
        return;

    m_shutdownPopupTimer.stop();

    QQuickWindow *window = m_externalWindow;
    m_externalWindow = nullptr;
    m_externalPreviewItem = nullptr;
    m_renderer = nullptr;

    window->close();
    window->deleteLater();
}

void ExternalMonitorManager::syncShutdownPopupState()
{
    if (!m_externalWindow) {
        m_shutdownPopupTimer.stop();
        return;
    }

    const bool visible = m_mainWindow
                         ? m_mainWindow->property("powerButtonShutdownOpen").toBool()
                         : false;
    const bool triggered = m_mainWindow
                           ? m_mainWindow->property("powerButtonShutdownTriggered").toBool()
                           : false;
    const bool holdActive = m_mainWindow
                            ? m_mainWindow->property("powerButtonShutdownHoldActive").toBool()
                            : false;
    const double holdProgress = m_mainWindow
                                ? m_mainWindow->property("powerButtonShutdownHoldProgress").toDouble()
                                : 0.0;
    const QString errorText = m_mainWindow
                              ? m_mainWindow->property("powerButtonShutdownErrorText").toString()
                              : QString();

    m_externalWindow->setProperty("shutdownPopupVisible", visible);
    m_externalWindow->setProperty("shutdownPopupTriggered", triggered);
    m_externalWindow->setProperty("shutdownHoldActive", holdActive);
    m_externalWindow->setProperty("shutdownHoldProgress", holdProgress);
    m_externalWindow->setProperty("shutdownErrorText", errorText);

    if (!visible && !triggered && !holdActive && errorText.isEmpty())
        m_shutdownPopupTimer.stop();
    else if (!m_shutdownPopupTimer.isActive())
        m_shutdownPopupTimer.start();
}
