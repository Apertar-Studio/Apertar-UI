#include "DisplayStandbyController.hpp"
#include <QGuiApplication>
#include <QScreen>
#include <QTimer>
#include <QWindow>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
namespace {
bool standbyScreen(const QScreen *screen)
{
    if (!screen) return false;
    const auto name = screen->name();
    if (name.contains("HDMI", Qt::CaseInsensitive)) return true;
    if (qgetenv("APERTAR_STANDBY_PANEL_POWER") == "0") return false;
    if (name.contains("DPI", Qt::CaseInsensitive) || name.contains("DSI", Qt::CaseInsensitive))
        return true;
    // Qt versions without a DPI connector name call it UNKNOWN<n>. Match the
    // real connector on this process's DRM card, never an arbitrary screen.
    if (!name.startsWith("UNKNOWN")) return false;
    bool valid = false;
    const int index = name.mid(7).toInt(&valid);
    if (!valid || index < 1) return false;
    QFile config(qEnvironmentVariable("QT_QPA_EGLFS_KMS_CONFIG"));
    if (!config.open(QIODevice::ReadOnly)) return false;
    const auto device = QJsonDocument::fromJson(config.readAll()).object().value("device").toString();
    const auto card = QFileInfo(device).fileName();
    if (!card.startsWith("card")) return false;
    return QFileInfo::exists(QStringLiteral("/sys/class/drm/%1-DPI-%2").arg(card).arg(index));
}
}
#if APERTAR_HAS_DISPLAY_POWER
#include <qpa/qplatformscreen.h>
#endif

DisplayStandbyController::DisplayStandbyController(QObject *parent) : QObject(parent)
{
    if (qGuiApp) {
        connect(qGuiApp, &QGuiApplication::screenAdded, this, [this](QScreen *) {
            if (m_standby)
                QTimer::singleShot(0, this, [this]() { turnOffHdmi(); });
        });
        connect(qGuiApp, &QGuiApplication::screenRemoved, this, [this](QScreen *screen) {
            m_previousPower.remove(screen);
        });
    }
}
DisplayStandbyController::~DisplayStandbyController() { restoreHdmi(); }

void DisplayStandbyController::setStandby(bool standby)
{
    if (m_standby == standby)
        return;
    m_standby = standby;
    if (standby) {
        // Stop window updates before DPMS. Keep Qt/DRM ownership for wake.
        for (QWindow *window : QGuiApplication::allWindows()) {
            if (window->isVisible() && standbyScreen(window->screen())) {
                m_hiddenWindows.append(window);
                window->hide();
            }
        }
        QTimer::singleShot(0, this, [this]() { turnOffHdmi(); });
    } else {
        restoreHdmi();
        for (const auto &window : m_hiddenWindows) {
            if (window)
                window->show();
        }
        m_hiddenWindows.clear();
    }
}
void DisplayStandbyController::turnOffHdmi()
{
    if (!m_standby || !qGuiApp)
        return;
#if APERTAR_HAS_DISPLAY_POWER
    if (!QGuiApplication::platformName().startsWith("eglfs"))
        return;
    for (QScreen *screen : QGuiApplication::screens()) {
        qInfo().noquote() << "Apertar standby: display candidate:" << screen->name();
        if (!standbyScreen(screen) ||
            m_previousPower.contains(screen) || !screen->handle())
            continue;
        m_previousPower.insert(screen, int(screen->handle()->powerState()));
        screen->handle()->setPowerState(QPlatformScreen::PowerStateOff);
        qInfo().noquote() << "Apertar standby: display power-off requested:" << screen->name();
    }
#else
    qWarning() << "Apertar standby: HDMI blanked; this build lacks Qt display-power support";
#endif
}
void DisplayStandbyController::restoreHdmi()
{
#if APERTAR_HAS_DISPLAY_POWER
    for (auto it = m_previousPower.constBegin(); it != m_previousPower.constEnd(); ++it) {
        QScreen *screen = it.key();
        if (screen && screen->handle()) {
            screen->handle()->setPowerState(QPlatformScreen::PowerState(it.value()));
            qInfo().noquote() << "Apertar standby: display power restored:" << screen->name();
        }
    }
#endif
    m_previousPower.clear();
}
