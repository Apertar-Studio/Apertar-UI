#pragma once

#include <QObject>
#include <QString>

class DisplayConfigBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int uiWidth READ uiWidth CONSTANT)
    Q_PROPERTY(int uiHeight READ uiHeight CONSTANT)
    Q_PROPERTY(QString uiLayout READ uiLayout CONSTANT)
    Q_PROPERTY(int uiRotationDegrees READ uiRotationDegrees CONSTANT)
    Q_PROPERTY(double controlsOpacity READ controlsOpacity CONSTANT)
    Q_PROPERTY(double zoomToggleFactor READ zoomToggleFactor CONSTANT)
    Q_PROPERTY(bool windowed READ windowed CONSTANT)
    Q_PROPERTY(bool hdmiExternalOnly READ hdmiExternalOnly CONSTANT)
    Q_PROPERTY(bool squareLayout READ squareLayout CONSTANT)
    Q_PROPERTY(bool landscapeLayout READ landscapeLayout CONSTANT)
    Q_PROPERTY(bool compactLandscapeLayout READ compactLandscapeLayout CONSTANT)
    Q_PROPERTY(bool regularLandscapeLayout READ regularLandscapeLayout CONSTANT)
    Q_PROPERTY(bool portraitLayout READ portraitLayout CONSTANT)
    Q_PROPERTY(bool compactPortraitLayout READ compactPortraitLayout CONSTANT)
    Q_PROPERTY(QString configPath READ configPath CONSTANT)

public:
    explicit DisplayConfigBridge(QObject *parent = nullptr);

    int uiWidth() const { return m_uiWidth; }
    int uiHeight() const { return m_uiHeight; }
    QString uiLayout() const { return m_uiLayout; }
    int uiRotationDegrees() const { return m_uiRotationDegrees; }
    double controlsOpacity() const { return m_controlsOpacity; }
    double zoomToggleFactor() const { return m_zoomToggleFactor; }
    bool windowed() const { return m_windowed; }
    bool hdmiExternalOnly() const { return m_hdmiExternalOnly; }
    bool squareLayout() const { return m_uiLayout == QStringLiteral("square"); }
    bool landscapeLayout() const { return m_uiLayout.startsWith(QStringLiteral("landscape")); }
    bool compactLandscapeLayout() const { return m_uiLayout == QStringLiteral("landscape_compact"); }
    bool regularLandscapeLayout() const
    {
        return m_uiLayout == QStringLiteral("landscape_medium") ||
               m_uiLayout == QStringLiteral("landscape") ||
               m_uiLayout == QStringLiteral("landscape_large");
    }
    bool portraitLayout() const { return m_uiLayout.startsWith(QStringLiteral("portrait")); }
    bool compactPortraitLayout() const { return m_uiLayout == QStringLiteral("portrait_compact"); }
    QString configPath() const { return m_configPath; }

private:
    void loadConfiguration();

    int m_uiWidth = 720;
    int m_uiHeight = 720;
    QString m_uiLayout = QStringLiteral("square");
    int m_uiRotationDegrees = 0;
    double m_controlsOpacity = 1.0;
    double m_zoomToggleFactor = 4.0;
    bool m_windowed = false;
    bool m_hdmiExternalOnly = false;
    QString m_configPath;
};
