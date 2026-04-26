#pragma once

#include <QHash>
#include <QMutex>
#include <QQuickImageProvider>
#include <QSet>
#include <QThreadPool>

class CdngImageProvider : public QQuickImageProvider
{
public:
    explicit CdngImageProvider(bool forceAsynchronousLoading = false,
                               bool previewOnly = false,
                               bool enablePrefetch = false);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    QImage makePlaceholder(const QString &message, const QSize &requestedSize) const;
    QString cacheKeyForId(const QString &id, const QSize &requestedSize) const;
    QString cacheKeyForPath(const QString &filePath, const QSize &requestedSize, int radius) const;
    void insertCachedImage(const QString &cacheKey, const QImage &image);
    void schedulePrefetch(const QString &filePath, const QSize &requestedSize, int radius);

    bool m_previewOnly = false;
    bool m_enablePrefetch = false;
    QThreadPool m_prefetchThreadPool;
    mutable QMutex m_cacheMutex;
    QHash<QString, QImage> m_thumbnailCache;
    QHash<QString, int> m_prefetchFrontierByClip;
    QSet<QString> m_prefetchRequestsInFlight;
};
