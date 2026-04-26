#pragma once

#include <QQuickItem>
#include <QString>


class CameraPreviewItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QObject* bridge READ bridge WRITE setBridge NOTIFY bridgeChanged)
    Q_PROPERTY(float zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(float panX READ panX WRITE setPanX NOTIFY panXChanged)
    Q_PROPERTY(float panY READ panY WRITE setPanY NOTIFY panYChanged)
    Q_PROPERTY(bool zebraEnabled READ zebraEnabled WRITE setZebraEnabled NOTIFY zebraEnabledChanged)
    Q_PROPERTY(float zebraThreshold READ zebraThreshold WRITE setZebraThreshold NOTIFY zebraThresholdChanged)
    Q_PROPERTY(bool focusPeakingEnabled READ focusPeakingEnabled WRITE setFocusPeakingEnabled NOTIFY focusPeakingEnabledChanged)
    Q_PROPERTY(float focusPeakingThreshold READ focusPeakingThreshold WRITE setFocusPeakingThreshold NOTIFY focusPeakingThresholdChanged)
    Q_PROPERTY(QString focusPeakingColor READ focusPeakingColor WRITE setFocusPeakingColor NOTIFY focusPeakingColorChanged)
    Q_PROPERTY(bool grayscaleEnabled READ grayscaleEnabled WRITE setGrayscaleEnabled NOTIFY grayscaleEnabledChanged)
    Q_PROPERTY(bool anamorphicDesqueezeEnabled READ anamorphicDesqueezeEnabled WRITE setAnamorphicDesqueezeEnabled NOTIFY anamorphicDesqueezeEnabledChanged)
    Q_PROPERTY(float anamorphicDesqueezeRatio READ anamorphicDesqueezeRatio WRITE setAnamorphicDesqueezeRatio NOTIFY anamorphicDesqueezeRatioChanged)
    Q_PROPERTY(bool falseColorEnabled READ falseColorEnabled WRITE setFalseColorEnabled NOTIFY falseColorEnabledChanged)
    Q_PROPERTY(int falseColorMode READ falseColorMode WRITE setFalseColorMode NOTIFY falseColorModeChanged)

public:
    explicit CameraPreviewItem(QQuickItem *parent = nullptr);

    QObject *bridge() const { return m_bridge; }
    void setBridge(QObject *bridge);
    float zoom() const { return m_zoom; }
    void setZoom(float zoom);
    float panX() const { return m_panX; }
    float panY() const { return m_panY; }

    void setPanX(float panX);
    void setPanY(float panY);

    bool zebraEnabled() const { return m_zebraEnabled; }
    float zebraThreshold() const { return m_zebraThreshold; }

    void setZebraEnabled(bool enabled);
    void setZebraThreshold(float threshold);

    bool focusPeakingEnabled() const { return m_focusPeakingEnabled; }
    void setFocusPeakingEnabled(bool v);

    float focusPeakingThreshold() const { return m_focusPeakingThreshold; }
    void setFocusPeakingThreshold(float v);

    QString focusPeakingColor() const { return m_focusPeakingColor; }
    void setFocusPeakingColor(const QString &value);

    bool grayscaleEnabled() const { return m_grayscaleEnabled; }
    void setGrayscaleEnabled(bool value);

    bool anamorphicDesqueezeEnabled() const { return m_anamorphicDesqueezeEnabled; }
    void setAnamorphicDesqueezeEnabled(bool value);

    float anamorphicDesqueezeRatio() const { return m_anamorphicDesqueezeRatio; }
    void setAnamorphicDesqueezeRatio(float value);

    bool falseColorEnabled() const { return m_falseColorEnabled; }
    void setFalseColorEnabled(bool value);

    int falseColorMode() const { return m_falseColorMode; }
    void setFalseColorMode(int value);

signals:
    void bridgeChanged();
    void geometryChangedSignal();

    void zoomChanged();

    void panXChanged();
    void panYChanged();

    void zebraEnabledChanged();
    void zebraThresholdChanged();

    void focusPeakingEnabledChanged();
    void focusPeakingThresholdChanged();

    void focusPeakingColorChanged();

    void grayscaleEnabledChanged();
    void anamorphicDesqueezeEnabledChanged();
    void anamorphicDesqueezeRatioChanged();

    void falseColorEnabledChanged();
    void falseColorModeChanged();

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    QObject *m_bridge = nullptr;
    
    float m_zoom = 1.0f;

    float m_panX = 0.0f;
    float m_panY = 0.0f;

    bool m_zebraEnabled = false;
    float m_zebraThreshold = 0.70f;

    bool m_focusPeakingEnabled = false;
    float m_focusPeakingThreshold = 0.08f;

    QString m_focusPeakingColor = "Red";

    bool m_grayscaleEnabled = false;
    bool m_anamorphicDesqueezeEnabled = false;
    float m_anamorphicDesqueezeRatio = 1.33f;

    bool m_falseColorEnabled = false;
    int m_falseColorMode = 0;

};
