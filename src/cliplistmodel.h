#pragma once

#include <QAbstractListModel>

class ClipListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString rootPath READ rootPath WRITE setRootPath NOTIFY rootPathChanged)
    Q_PROPERTY(bool stillMode READ stillMode WRITE setStillMode NOTIFY stillModeChanged)
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
        ShotDateRole
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
    };

    void scanClips();
    void setLoading(bool loading);
    void setStatusText(const QString &statusText);

    QList<ClipInfo> m_clips;
    QString m_rootPath;
    bool m_stillMode = false;
    bool m_loading = false;
    QString m_statusText;
};
