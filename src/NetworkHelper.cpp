#include "WifiBridge.hpp"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QLockFile>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Apertar Network"));
    qputenv("APERTAR_NETWORK_HELPER", "1");
    const QString directory = qEnvironmentVariable("APERTAR_NETWORK_RUNTIME", QStringLiteral("/run/apertar-network"));
    if (!QDir().mkpath(directory)) return 1;
    QLockFile lock(directory + QStringLiteral("/helper.lock"));
    if (!lock.tryLock(0)) {
        qWarning() << "Apertar network helper is already running";
        return 1;
    }
    QFile marker(directory + QStringLiteral("/managed"));
    if (!marker.open(QIODevice::WriteOnly)) return 1;
    marker.close();
    WifiBridge wifi;
    QObject::connect(&wifi, &WifiBridge::lastErrorChanged, &app, [&wifi]() {
        if (!wifi.lastError().isEmpty()) qWarning() << "Apertar network:" << wifi.lastError();
    });
    qInfo() << "Apertar network: independent hotspot fallback ready; no display required";
    const int result = app.exec();
    marker.remove();
    return result;
}
