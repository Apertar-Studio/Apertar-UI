#include "DisplayCalibration.hpp"

#include <QDir>
#include <QFileInfo>
#include <QOpenGLContext>
#include <QDebug>
#include <algorithm>
#include <cmath>
#include <tuple>

namespace {
double bounded(const QVariant &value, double fallback, double low, double high)
{
    bool ok = false;
    const double number = value.toDouble(&ok);
    return ok && std::isfinite(number) ? std::clamp(number, low, high) : fallback;
}
GLuint compileShader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024] = {};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        qWarning() << "Display calibration shader:" << log;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}
}

bool DisplayCalibrationProfile::operator==(const DisplayCalibrationProfile &p) const
{
    return std::tie(enabled, red, green, blue, gamma, black, white)
        == std::tie(p.enabled, p.red, p.green, p.blue, p.gamma, p.black, p.white);
}
bool DisplayCalibrationProfile::neutral() const
{
    DisplayCalibrationProfile identity;
    identity.enabled = enabled;
    return *this == identity;
}

DisplayCalibrationBridge::DisplayCalibrationBridge(QObject *parent)
    : QObject(parent), m_settings("CinePi", "DisplayCalibration")
{
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(200);
    connect(&m_saveTimer, &QTimer::timeout, this, &DisplayCalibrationBridge::save);
    auto reload = [this]() { if (!m_saveTimer.isActive()) load(); watchSettings(); };
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, reload);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, reload);
    load();
    watchSettings();
}
DisplayCalibrationBridge::~DisplayCalibrationBridge()
{
    if (m_saveTimer.isActive()) save();
}
QString DisplayCalibrationBridge::target() const
{
    return m_target ? QStringLiteral("HDMI") : QStringLiteral("Built-in");
}
void DisplayCalibrationBridge::setTarget(const QString &value)
{
    if (value != "Built-in" && value != "HDMI") return;
    const int target = value == "HDMI" ? 1 : 0;
    if (m_target == target) return;
    m_target = target;
    emit valuesChanged();
}
QVariantMap DisplayCalibrationBridge::profile() const
{
    const auto &p = m_profiles[m_target];
    return {{"enabled", p.enabled}, {"red", p.red}, {"green", p.green}, {"blue", p.blue},
            {"gamma", p.gamma}, {"black", p.black}, {"white", p.white}};
}
void DisplayCalibrationBridge::setValue(const QString &key, const QVariant &value)
{
    auto p = m_profiles[m_target];
    if (key == "enabled") p.enabled = value.toBool();
    else if (key == "red") p.red = bounded(value, p.red, 0.5, 1.5);
    else if (key == "green") p.green = bounded(value, p.green, 0.5, 1.5);
    else if (key == "blue") p.blue = bounded(value, p.blue, 0.5, 1.5);
    else if (key == "gamma") p.gamma = bounded(value, p.gamma, 0.5, 2.5);
    else if (key == "black") p.black = bounded(value, p.black, -0.1, 0.1);
    else if (key == "white") p.white = bounded(value, p.white, 0.7, 1.3);
    else return;
    if (p == m_profiles[m_target]) return;
    m_profiles[m_target] = p;
    m_saveTimer.start();
    emit valuesChanged();
    emit calibrationChanged(m_target != 0);
}
void DisplayCalibrationBridge::resetSelected()
{
    const bool enabled = m_profiles[m_target].enabled;
    m_profiles[m_target] = {};
    m_profiles[m_target].enabled = enabled;
    m_saveTimer.start();
    emit valuesChanged();
    emit calibrationChanged(m_target != 0);
}
void DisplayCalibrationBridge::setStatus(bool hdmi, const QString &text)
{
    const int index = hdmi ? 1 : 0;
    if (m_status[index] == text) return;
    m_status[index] = text;
    if (m_target == index) emit valuesChanged();
}
void DisplayCalibrationBridge::load()
{
    m_settings.sync();
    for (int i = 0; i < 2; ++i) {
        m_settings.beginGroup(i ? "hdmi" : "builtin");
        DisplayCalibrationProfile p;
        p.enabled = m_settings.value("enabled", false).toBool();
        p.red = bounded(m_settings.value("red", 1.0), 1.0, 0.5, 1.5);
        p.green = bounded(m_settings.value("green", 1.0), 1.0, 0.5, 1.5);
        p.blue = bounded(m_settings.value("blue", 1.0), 1.0, 0.5, 1.5);
        p.gamma = bounded(m_settings.value("gamma", 1.0), 1.0, 0.5, 2.5);
        p.black = bounded(m_settings.value("black", 0.0), 0.0, -0.1, 0.1);
        p.white = bounded(m_settings.value("white", 1.0), 1.0, 0.7, 1.3);
        m_settings.endGroup();
        if (!(p == m_profiles[i])) {
            m_profiles[i] = p;
            emit calibrationChanged(i != 0);
            if (m_target == i) emit valuesChanged();
        }
    }
}
void DisplayCalibrationBridge::save()
{
    for (int i = 0; i < 2; ++i) {
        const auto &p = m_profiles[i];
        m_settings.beginGroup(i ? "hdmi" : "builtin");
        m_settings.setValue("enabled", p.enabled);
        m_settings.setValue("red", p.red);
        m_settings.setValue("green", p.green);
        m_settings.setValue("blue", p.blue);
        m_settings.setValue("gamma", p.gamma);
        m_settings.setValue("black", p.black);
        m_settings.setValue("white", p.white);
        m_settings.endGroup();
    }
    m_settings.sync();
    if (m_settings.status() != QSettings::NoError)
        setStatus(m_target != 0, "Could not save display calibration");
    watchSettings();
}
void DisplayCalibrationBridge::watchSettings()
{
    const QString file = m_settings.fileName();
    const QString directory = QFileInfo(file).absolutePath();
    QDir().mkpath(directory);
    if (!m_watcher.directories().contains(directory)) m_watcher.addPath(directory);
    if (QFileInfo::exists(file) && !m_watcher.files().contains(file)) m_watcher.addPath(file);
}

DisplayCalibrationRenderer::DisplayCalibrationRenderer(QQuickWindow *window, DisplayCalibrationBridge *bridge, bool hdmi)
    : QObject(window), m_window(window), m_bridge(bridge), m_hdmi(hdmi)
{
    connect(window, &QQuickWindow::beforeSynchronizing, this, &DisplayCalibrationRenderer::sync, Qt::DirectConnection);
    connect(window, &QQuickWindow::afterRenderPassRecording, this, &DisplayCalibrationRenderer::render, Qt::DirectConnection);
    connect(window, &QQuickWindow::sceneGraphInvalidated, this, &DisplayCalibrationRenderer::release, Qt::DirectConnection);
    connect(bridge, &DisplayCalibrationBridge::calibrationChanged, window, [window, hdmi](bool output) {
        if (output == hdmi) window->update();
    });
}
void DisplayCalibrationRenderer::sync()
{
    const auto p = m_bridge->profileFor(m_hdmi);
    m_active = p.enabled && !p.neutral();
    if (p == m_profile) return;
    m_profile = p;
    m_lutDirty = true;
    const double gains[] = {p.red, p.green, p.blue};
    for (int i = 0; i < 256; ++i) {
        const double c = std::clamp((i / 255.0 - p.black) / (p.white - p.black), 0.0, 1.0);
        for (int channel = 0; channel < 3; ++channel)
            m_lut[i * 4 + channel] = static_cast<unsigned char>(std::lround(
                std::clamp(std::pow(c, 1.0 / p.gamma) * gains[channel], 0.0, 1.0) * 255.0));
        m_lut[i * 4 + 3] = 255;
    }
}
void DisplayCalibrationRenderer::report(const QString &text)
{
    QMetaObject::invokeMethod(m_bridge, [bridge = m_bridge, hdmi = m_hdmi, text]() {
        bridge->setStatus(hdmi, text);
    }, Qt::QueuedConnection);
}
bool DisplayCalibrationRenderer::initialize()
{
    if (m_attempted) return m_program != 0;
    m_attempted = true;
    auto *context = QOpenGLContext::currentContext();
    if (!context || !context->hasExtension("GL_EXT_shader_framebuffer_fetch")) {
        report("Unavailable: this GPU/driver lacks framebuffer fetch");
        return false;
    }
    const char *vertex = "attribute vec2 position; void main(){gl_Position=vec4(position,0.0,1.0);}";
    const char *fragment =
        "#extension GL_EXT_shader_framebuffer_fetch : require\n"
        "precision highp float; uniform sampler2D curve;\n"
        "void main(){vec4 c=gl_LastFragData[0];vec3 t=(clamp(c.rgb,0.0,1.0)*255.0+0.5)/256.0;"
        "gl_FragColor=vec4(texture2D(curve,vec2(t.r,0.5)).r,"
        "texture2D(curve,vec2(t.g,0.5)).g,texture2D(curve,vec2(t.b,0.5)).b,c.a);}";
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertex), fs = compileShader(GL_FRAGMENT_SHADER, fragment);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        report("Unavailable: display calibration shader failed");
        return false;
    }
    m_program = glCreateProgram();
    glAttachShader(m_program, vs);
    glAttachShader(m_program, fs);
    glBindAttribLocation(m_program, 0, "position");
    glLinkProgram(m_program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint linked = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &linked);
    if (!linked) { release(); m_attempted = true; report("Unavailable: display calibration shader failed"); return false; }
    glUseProgram(m_program);
    glUniform1i(glGetUniformLocation(m_program, "curve"), 0);
    const GLfloat vertices[] = {-1,-1, 3,-1, -1,3};
    glGenBuffers(1, &m_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glGenTextures(1, &m_texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    m_lutDirty = true;
    qInfo() << "Apertar display calibration: framebuffer fetch, 1 KiB curve, output" << (m_hdmi ? "HDMI" : "built-in");
    report("Ready: GPU correction, no extra full-screen buffer");
    return true;
}
void DisplayCalibrationRenderer::render()
{
    if (!m_active) return;
    m_window->beginExternalCommands();
    if (initialize()) {
        // Keep Qt's final render target bound: preview and all QML are already composited here.
        glViewport(0, 0, qRound(m_window->width() * m_window->effectiveDevicePixelRatio()),
                   qRound(m_window->height() * m_window->effectiveDevicePixelRatio()));
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glUseProgram(m_program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        if (m_lutDirty) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 1, GL_RGBA, GL_UNSIGNED_BYTE, m_lut.data());
            m_lutDirty = false;
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    m_window->endExternalCommands();
}
void DisplayCalibrationRenderer::release()
{
    if (QOpenGLContext::currentContext()) {
        if (m_program) glDeleteProgram(m_program);
        if (m_buffer) glDeleteBuffers(1, &m_buffer);
        if (m_texture) glDeleteTextures(1, &m_texture);
    }
    m_program = m_buffer = m_texture = 0;
    m_attempted = false;
    m_lutDirty = true;
}
