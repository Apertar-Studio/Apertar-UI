#pragma once

#include <QByteArray>
#include <QObject>
#include <QSocketNotifier>
#include <QString>
#include <QTimer>
#include <QVector>
#include <optional>
#include <cstdint>

#include "PreviewFrameInfo.hpp"

class ApertarPreviewSocketClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(qulonglong frameCounter READ frameCounter NOTIFY frameArrived)

public:
    explicit ApertarPreviewSocketClient(QObject *parent = nullptr);
    ~ApertarPreviewSocketClient() override;

    bool connected() const { return m_connected; }
    QString statusText() const { return m_statusText; }
    qulonglong frameCounter() const { return m_lastFrame; }

    Q_INVOKABLE bool connectToCore(const QString &socketPath = QStringLiteral("/tmp/apertar-core.sock"));
    Q_INVOKABLE void disconnectFromCore();

    std::optional<PreviewFrameInfo> currentPreviewFrame() const;
    int duplicateProducerFd(int procid, int fd) const;

signals:
    void connectedChanged();
    void statusTextChanged();
    void frameArrived();
    void previewFrameReady();

private slots:
    void readAvailableMessages();

private:
    static constexpr int kOwnedFdProcId = -4242;

    void closeSocket();
    void closePendingFds();
    void scheduleReconnect();
    void processReadBuffer();
    QVector<int> takePendingFdsForMessage(const QByteArray &message);
    void setConnected(bool connected);
    void setStatusText(const QString &text);
    void closePreviewFd();
    void handleMessage(const QByteArray &message, QVector<int> attachedFds);
    void handlePreviewFrame(const QByteArray &message, QVector<int> attachedFds);
    bool jsonStringValue(const QByteArray &message, const QByteArray &key, QByteArray *value) const;
    bool jsonIntegerValue(const QByteArray &message, const QByteArray &key, qlonglong *value) const;

    int m_socketFd = -1;
    QSocketNotifier *m_notifier = nullptr;
    QTimer m_reconnectTimer;
    QString m_socketPath;
    QByteArray m_readBuffer;
    QVector<int> m_pendingFds;
    bool m_connected = false;
    QString m_statusText;

    int m_previewFd = -1;
    uint64_t m_lastFrame = 0;
    PreviewFrameInfo m_latestFrame;
};
