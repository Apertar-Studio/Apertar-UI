#pragma once

#include <QAbstractListModel>
#include <QFileSystemWatcher>
#include <QTimer>

class ClipListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString rootPath READ rootPath WRITE setRootPath NOTIFY rootPathChanged)
    Q_PROPERTY(bool stillMode READ stillMode WRITE setStillMode NOTIFY stillModeChanged)
    Q_PROPERTY(QString sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)
    Q_PROPERTY(bool browserOpen READ browserOpen WRITE setBrowserOpen NOTIFY browserOpenChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
    enum ClipRoles {
        NameRole = Qt::UserRole + 1,
        PathRole,
        FrameCountRole,
        DurationTextRole,
        ThumbnailSourceRole,
        ShotDateRole,
        FormatLabelRole
    };
    Q_ENUM(ClipRoles)

    explicit ClipListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString rootPath() const;
    void setRootPath(const QString &rootPath);
    bool stillMode() const;
    void setStillMode(bool stillMode);
    QString sortMode() const;
    void setSortMode(const QString &sortMode);
    bool browserOpen() const;
    void setBrowserOpen(bool open);
    int count() const;

    bool loading() const;
    QString statusText() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE int indexOfPath(const QString &path) const;
    Q_INVOKABLE QString pathAt(int index) const;
    Q_INVOKABLE QString nameAt(int index) const;
    Q_INVOKABLE int frameCountAt(int index) const;
    Q_INVOKABLE QString durationTextAt(int index) const;
    Q_INVOKABLE bool removeClip(const QString &path);

signals:
    void countChanged();
    void rootPathChanged();
    void stillModeChanged();
    void sortModeChanged();
    void browserOpenChanged();
    void loadingChanged();
    void statusTextChanged();

private:
    struct ClipInfo {
        QString name;
        QString path;
        int frameCount = 0;
        QString durationText;
        QString thumbnailSource;
        QString shotDate;
        QString formatLabel = QStringLiteral("cDNG clip");
        qint64 shotTimestampMs = 0;
    };

    void scanClips();
    void updateDirectoryWatches();
    void setLoading(bool loading);
    void setStatusText(const QString &statusText);

    QList<ClipInfo> m_clips;
    QString m_rootPath;
    bool m_stillMode = false;
    QString m_sortMode = QStringLiteral("Newest First");
    bool m_browserOpen = false;
    QFileSystemWatcher m_mediaWatcher;
    QTimer m_refreshTimer;
    bool m_hasScanned = false;
    bool m_loading = false;
    QString m_statusText;
};
