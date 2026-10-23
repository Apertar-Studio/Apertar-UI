#pragma once

#include <QObject>
#include <QFileSystemWatcher>
#include <QSettings>
#include <QTimer>
#include <QVariantMap>
#include <QQuickWindow>
#include <array>
#include <GLES2/gl2.h>

struct DisplayCalibrationProfile {
    bool enabled = false;
    double red = 1.0, green = 1.0, blue = 1.0;
    double gamma = 1.0, black = 0.0, white = 1.0;
    bool operator==(const DisplayCalibrationProfile &other) const;
    bool neutral() const;
};

class DisplayCalibrationBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString target READ target WRITE setTarget NOTIFY valuesChanged)
    Q_PROPERTY(QVariantMap profile READ profile NOTIFY valuesChanged)
    Q_PROPERTY(QString status READ status NOTIFY valuesChanged)
public:
    explicit DisplayCalibrationBridge(QObject *parent = nullptr);
    ~DisplayCalibrationBridge() override;
    QString target() const;
    void setTarget(const QString &value);
    QVariantMap profile() const;
    QString status() const { return m_status[m_target]; }
    DisplayCalibrationProfile profileFor(bool hdmi) const { return m_profiles[hdmi ? 1 : 0]; }
    Q_INVOKABLE void setValue(const QString &key, const QVariant &value);
    Q_INVOKABLE void resetSelected();
    void setStatus(bool hdmi, const QString &status);
signals:
    void valuesChanged();
    void calibrationChanged(bool hdmi);
private:
    void load();
    void save();
    void watchSettings();
    QSettings m_settings;
    QTimer m_saveTimer;
    QFileSystemWatcher m_watcher;
    std::array<DisplayCalibrationProfile, 2> m_profiles;
    std::array<QString, 2> m_status { "Enabled when correction is applied", "Enabled when correction is applied" };
    int m_target = 0;
};

class DisplayCalibrationRenderer : public QObject {
    Q_OBJECT
public:
    DisplayCalibrationRenderer(QQuickWindow *window, DisplayCalibrationBridge *bridge, bool hdmi);
private:
    void sync();
    void render();
    void release();
    bool initialize();
    void report(const QString &text);
    QQuickWindow *m_window;
    DisplayCalibrationBridge *m_bridge;
    bool m_hdmi;
    DisplayCalibrationProfile m_profile;
    bool m_active = false, m_attempted = false, m_lutDirty = true;
    GLuint m_program = 0, m_buffer = 0, m_texture = 0;
    std::array<unsigned char, 1024> m_lut {};
};
