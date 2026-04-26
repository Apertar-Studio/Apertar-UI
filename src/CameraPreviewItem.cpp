#include "CameraPreviewItem.hpp"
#include <QQuickWindow>
#include <algorithm>

CameraPreviewItem::CameraPreviewItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, false);
}

void CameraPreviewItem::setBridge(QObject *bridge)
{
    if (m_bridge == bridge)
        return;

    m_bridge = bridge;
    emit bridgeChanged();
}

void CameraPreviewItem::setZoom(float zoom)
{
    if (qFuzzyCompare(m_zoom, zoom))
        return;

    m_zoom = zoom;
    emit zoomChanged();
}

void CameraPreviewItem::setPanX(float panX)
{
    panX = std::max(-1.0f, std::min(1.0f, panX));

    if (qFuzzyCompare(m_panX, panX))
        return;

    m_panX = panX;
    emit panXChanged();
}

void CameraPreviewItem::setPanY(float panY)
{
    panY = std::max(-1.0f, std::min(1.0f, panY));

    if (qFuzzyCompare(m_panY, panY))
        return;

    m_panY = panY;
    emit panYChanged();
}

void CameraPreviewItem::setZebraEnabled(bool enabled)
{
    if (m_zebraEnabled == enabled)
        return;

    m_zebraEnabled = enabled;
    emit zebraEnabledChanged();

    if (window())
        window()->update();
}

void CameraPreviewItem::setZebraThreshold(float threshold)
{
    threshold = std::max(0.0f, std::min(1.0f, threshold));

    if (qFuzzyCompare(m_zebraThreshold, threshold))
        return;

    m_zebraThreshold = threshold;
    emit zebraThresholdChanged();

    if (window())
        window()->update();
}

void CameraPreviewItem::setFocusPeakingEnabled(bool v)
{
    if (m_focusPeakingEnabled == v)
        return;

    m_focusPeakingEnabled = v;
    emit focusPeakingEnabledChanged();

    if (window())
        window()->update();
}

void CameraPreviewItem::setFocusPeakingThreshold(float v)
{
    if (qFuzzyCompare(m_focusPeakingThreshold, v))
        return;

    m_focusPeakingThreshold = v;
    emit focusPeakingThresholdChanged();

    if (window())
        window()->update();
}

void CameraPreviewItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    emit geometryChangedSignal();
}

void CameraPreviewItem::setFocusPeakingColor(const QString &value)
{
    if (m_focusPeakingColor == value)
        return;

    m_focusPeakingColor = value;
    emit focusPeakingColorChanged();

    if (window())
        window()->update();
}

void CameraPreviewItem::setGrayscaleEnabled(bool value)
{
    if (m_grayscaleEnabled == value)
        return;

    m_grayscaleEnabled = value;
    emit grayscaleEnabledChanged();

    if (window())
        window()->update();
}

void CameraPreviewItem::setAnamorphicDesqueezeEnabled(bool value)
{
    if (m_anamorphicDesqueezeEnabled == value)
        return;

    m_anamorphicDesqueezeEnabled = value;
    emit anamorphicDesqueezeEnabledChanged();

    if (window())
        window()->update();
}

void CameraPreviewItem::setAnamorphicDesqueezeRatio(float value)
{
    value = std::max(1.0f, std::min(3.0f, value));

    if (qFuzzyCompare(m_anamorphicDesqueezeRatio, value))
        return;

    m_anamorphicDesqueezeRatio = value;
    emit anamorphicDesqueezeRatioChanged();

    if (window())
        window()->update();
}

void CameraPreviewItem::setFalseColorEnabled(bool value)
{
    if (m_falseColorEnabled == value)
        return;

    m_falseColorEnabled = value;
    emit falseColorEnabledChanged();

    if (window())
        window()->update();
}

void CameraPreviewItem::setFalseColorMode(int value)
{
    if (m_falseColorMode == value)
        return;

    m_falseColorMode = value;
    emit falseColorModeChanged();

    if (window())
        window()->update();
}
