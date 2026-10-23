#include "PreviewWindowRenderer.hpp"
#include "ApertarPreviewSocketClient.hpp"
#include "CameraPreviewItem.hpp"
#include "PreviewExposureReference.hpp"

#include <QQuickWindow>
#include <QQuickItem>
#include <QByteArray>
#include <QDir>
#include <QMatrix4x4>
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QRegularExpression>
#include <QTextStream>

#include <QVector3D>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <libdrm/drm_fourcc.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <string>
#include <thread>

namespace {

bool runningOnRaspberryPi3()
{
    QFile modelFile(QStringLiteral("/proc/device-tree/model"));
    if (!modelFile.open(QIODevice::ReadOnly))
        return false;

    const QByteArray model = modelFile.readAll().toLower();
    return model.contains("raspberry pi 3");
}

bool envFlagEnabled(const char *name)
{
    const QByteArray value = qgetenv(name).toLower();
    return !value.isEmpty() &&
           value != "0" &&
           value != "false" &&
           value != "off";
}

QRectF centeredAspectFitRect(QRectF rect, double aspect)
{
    if (rect.isEmpty() || !(aspect > 0.0))
        return rect;

    const double rectAspect = rect.width() / rect.height();
    if (rectAspect > aspect) {
        const double fittedWidth = rect.height() * aspect;
        return QRectF(rect.x() + (rect.width() - fittedWidth) * 0.5,
                      rect.y(),
                      fittedWidth,
                      rect.height());
    } else {
        const double fittedHeight = rect.width() / aspect;
        return QRectF(rect.x(),
                      rect.y() + (rect.height() - fittedHeight) * 0.5,
                      rect.width(),
                      fittedHeight);
    }
}

struct PreviewUvRect
{
    float u0 = 0.0f;
    float u1 = 1.0f;
    float v0 = 0.0f;
    float v1 = 1.0f;
};

PreviewUvRect croppedPreviewUvRect(unsigned int sourceWidth,
                                   unsigned int sourceHeight,
                                   float targetAspect,
                                   float zoom,
                                   float panX,
                                   float panY)
{
    PreviewUvRect rect;
    if (sourceWidth == 0 || sourceHeight == 0)
        return rect;

    const float sourceAspect = float(sourceWidth) / float(sourceHeight);
    float halfU = 0.5f;
    float halfV = 0.5f;

    if (std::isfinite(targetAspect) && targetAspect > 0.01f && sourceAspect > 0.0f) {
        if (targetAspect > sourceAspect) {
            halfV = 0.5f * sourceAspect / targetAspect;
        } else if (targetAspect < sourceAspect) {
            halfU = 0.5f * targetAspect / sourceAspect;
        }
    }

    const float safeZoom = std::max(1.0f, zoom);
    halfU = std::clamp(halfU / safeZoom, 0.0001f, 0.5f);
    halfV = std::clamp(halfV / safeZoom, 0.0001f, 0.5f);

    const float clampedPanX = std::clamp(panX, -1.0f, 1.0f);
    const float clampedPanY = std::clamp(panY, -1.0f, 1.0f);
    const float centerU = std::clamp(0.5f + clampedPanX * (0.5f - halfU), halfU, 1.0f - halfU);
    const float centerV = std::clamp(0.5f + clampedPanY * (0.5f - halfV), halfV, 1.0f - halfV);

    rect.u0 = centerU - halfU;
    rect.u1 = centerU + halfU;
    rect.v0 = centerV - halfV;
    rect.v1 = centerV + halfV;
    return rect;
}

void uploadLumaPlane(GLuint *texture,
                     QSize *storedSize,
                     const unsigned char *data,
                     unsigned int width,
                     unsigned int height,
                     unsigned int pitch)
{
    if (!texture || !storedSize || !data || width == 0 || height == 0 || pitch == 0)
        return;

    if (!*texture)
        glGenTextures(1, texture);

    const QSize size{int(width), int(height)};
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLint previousAlignment = 4;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (*storedSize != size) {
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_LUMINANCE,
                     int(width),
                     int(height),
                     0,
                     GL_LUMINANCE,
                     GL_UNSIGNED_BYTE,
                     nullptr);
        *storedSize = size;
    }

    if (pitch == width) {
        glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        0,
                        0,
                        int(width),
                        int(height),
                        GL_LUMINANCE,
                        GL_UNSIGNED_BYTE,
                        data);
    } else {
        for (unsigned int row = 0; row < height; ++row) {
            glTexSubImage2D(GL_TEXTURE_2D,
                            0,
                            0,
                            int(row),
                            int(width),
                            1,
                            GL_LUMINANCE,
                            GL_UNSIGNED_BYTE,
                            data + size_t(row) * pitch);
        }
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace

PreviewWindowRenderer::LutAtlasData PreviewWindowRenderer::parseLutAtlasFile(const QString &cleanPath)
{
    PreviewWindowRenderer::LutAtlasData atlas;
    atlas.path = cleanPath;

    QFile file(cleanPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        atlas.error = QStringLiteral("cannot open LUT");
        return atlas;
    }

    int lutSize = 0;
    std::vector<float> values;
    QTextStream stream(&file);
    const QRegularExpression whitespace(QStringLiteral("\\s+"));
    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;

        const int commentIndex = line.indexOf(QLatin1Char('#'));
        if (commentIndex >= 0)
            line = line.left(commentIndex).trimmed();
        if (line.isEmpty())
            continue;

        const QStringList parts = line.split(whitespace, Qt::SkipEmptyParts);
        if (parts.isEmpty())
            continue;

        const QString keyword = parts[0].toUpper();
        if (keyword == QStringLiteral("LUT_3D_SIZE") && parts.size() >= 2) {
            bool ok = false;
            const int parsedSize = parts[1].toInt(&ok);
            if (ok && parsedSize >= 2 && parsedSize <= 128)
                lutSize = parsedSize;
            continue;
        }
        if (keyword.startsWith(QStringLiteral("TITLE")) ||
            keyword.startsWith(QStringLiteral("DOMAIN_")) ||
            keyword.startsWith(QStringLiteral("LUT_1D_SIZE"))) {
            continue;
        }

        if (parts.size() < 3)
            continue;

        bool okR = false;
        bool okG = false;
        bool okB = false;
        const float r = parts[0].toFloat(&okR);
        const float g = parts[1].toFloat(&okG);
        const float b = parts[2].toFloat(&okB);
        if (!okR || !okG || !okB)
            continue;

        values.push_back(qBound(0.0f, r, 1.0f));
        values.push_back(qBound(0.0f, g, 1.0f));
        values.push_back(qBound(0.0f, b, 1.0f));
    }

    if (lutSize <= 0) {
        const int entries = static_cast<int>(values.size() / 3);
        for (int candidate = 2; candidate <= 128; ++candidate) {
            if (candidate * candidate * candidate == entries) {
                lutSize = candidate;
                break;
            }
        }
    }

    const int expectedValues = lutSize * lutSize * lutSize * 3;
    if (lutSize <= 0 || static_cast<int>(values.size()) < expectedValues) {
        atlas.error = QStringLiteral("invalid 3D .cube LUT");
        return atlas;
    }

    atlas.size = lutSize;
    atlas.columns = static_cast<int>(std::ceil(std::sqrt(float(lutSize))));
    const int atlasRows = (lutSize + atlas.columns - 1) / atlas.columns;
    atlas.width = atlas.columns * lutSize;
    atlas.height = atlasRows * lutSize;
    atlas.pixels.assign(static_cast<size_t>(atlas.width) * static_cast<size_t>(atlas.height) * 3, 0);

    for (int b = 0; b < lutSize; ++b) {
        const int tileX = b % atlas.columns;
        const int tileY = b / atlas.columns;
        for (int g = 0; g < lutSize; ++g) {
            for (int r = 0; r < lutSize; ++r) {
                // Standard .cube ordering is R fastest, then G, then B. The atlas stores B as the tile index.
                const size_t src = (static_cast<size_t>((b * lutSize + g) * lutSize + r) * 3);
                const size_t dst = (static_cast<size_t>(tileY * lutSize + g) * static_cast<size_t>(atlas.width)
                                    + static_cast<size_t>(tileX * lutSize + r)) * 3;
                atlas.pixels[dst + 0] = static_cast<unsigned char>(std::lround(values[src + 0] * 255.0f));
                atlas.pixels[dst + 1] = static_cast<unsigned char>(std::lround(values[src + 1] * 255.0f));
                atlas.pixels[dst + 2] = static_cast<unsigned char>(std::lround(values[src + 2] * 255.0f));
            }
        }
    }

    return atlas;
}

PreviewWindowRenderer::PreviewWindowRenderer(QObject *parent)
    : QObject(parent)
{
    m_lutLoadState = std::make_shared<LutLoadState>();

    const bool pi3 = runningOnRaspberryPi3();
    m_forceCpuFallback = envFlagEnabled("APERTAR_PREVIEW_FORCE_CPU");
    if (m_forceCpuFallback)
        qInfo() << "Apertar preview: using CPU texture preview fallback because APERTAR_PREVIEW_FORCE_CPU is set.";

    m_forcePlanarFallback = envFlagEnabled("APERTAR_PREVIEW_FORCE_PLANAR") ||
                            (pi3 && !envFlagEnabled("APERTAR_PREVIEW_FORCE_EGL"));
    if (pi3)
        m_directWindowRendering = true;
    if (m_forcePlanarFallback && !m_forceCpuFallback)
        qInfo() << "Apertar preview: using planar YUV texture path for Pi3/VC4 compatibility.";
    m_allowCpuFallback = m_forceCpuFallback || m_forcePlanarFallback;

    m_shaderTimer.start();
}

PreviewWindowRenderer::~PreviewWindowRenderer()
{
    if (m_exposureTexture) glDeleteTextures(1,&m_exposureTexture);
    cleanupImportedBuffers();
    clearLutTexture();
    if (m_neutralLutTexture)
        glDeleteTextures(1, &m_neutralLutTexture);
    if (m_processProgram)
        glDeleteProgram(m_processProgram);
    if (m_passthroughProgram)
        glDeleteProgram(m_passthroughProgram);
    if (m_processProgram2D)
        glDeleteProgram(m_processProgram2D);
    if (m_passthroughProgram2D)
        glDeleteProgram(m_passthroughProgram2D);
    if (m_processProgramPlanar)
        glDeleteProgram(m_processProgramPlanar);
    if (m_passthroughProgramPlanar)
        glDeleteProgram(m_passthroughProgramPlanar);
    if (m_planarTextures[0] || m_planarTextures[1] || m_planarTextures[2])
        glDeleteTextures(3, m_planarTextures);
    if (m_cpuFallbackTexture)
        glDeleteTextures(1, &m_cpuFallbackTexture);
    if (m_previewTexture)
        glDeleteTextures(1, &m_previewTexture);
    if (m_previewFbo)
        glDeleteFramebuffers(1, &m_previewFbo);
    if (m_vbo)
        glDeleteBuffers(1, &m_vbo);
}

void PreviewWindowRenderer::scheduleWindowUpdate()
{
    if (!m_window || !m_item)
        return;

    if (!m_item->isVisible() || m_item->width() <= 0 || m_item->height() <= 0)
        return;

    if (m_updatePending)
        return;

    m_updatePending = true;
    m_window->update();
}

void PreviewWindowRenderer::setPreviewItem(CameraPreviewItem *item)
{
    if (m_item == item)
        return;

    m_item = item;

    if (!m_item)
        return;

    connect(m_item, &QQuickItem::windowChanged,
            this, &PreviewWindowRenderer::handleWindowChanged,
            Qt::UniqueConnection);

    connect(m_item, &CameraPreviewItem::bridgeChanged,
            this, &PreviewWindowRenderer::syncState,
            Qt::UniqueConnection);

    connect(m_item, &CameraPreviewItem::geometryChangedSignal,
            this, [this]() {
                scheduleWindowUpdate();
            });

    handleWindowChanged(m_item->window());
    syncState();
}

void PreviewWindowRenderer::handleWindowChanged(QQuickWindow *window)
{
    if (m_window == window)
        return;

    if (m_window)
        disconnect(m_window, nullptr, this, nullptr);

    m_window = window;
    m_initialized = false;
    m_previewCacheState.reset();

    if (!m_window)
        return;

    m_window->setColor(Qt::black);

    connect(m_window, &QQuickWindow::beforeSynchronizing,
            this, &PreviewWindowRenderer::syncState,
            Qt::DirectConnection);

    connect(m_window, &QQuickWindow::beforeRenderPassRecording,
            this, &PreviewWindowRenderer::render,
            Qt::DirectConnection);
}

void PreviewWindowRenderer::syncState()
{
    if (!m_item)
        return;

    QObject *newBridge = m_item->bridge();
    if (m_bridge != newBridge) {
        if (m_bridge)
            disconnect(m_bridge, nullptr, this, nullptr);

        m_bridge = newBridge;

        if (m_bridge)
            connectBridgeSignals(m_bridge);
    }

}

void PreviewWindowRenderer::connectBridgeSignals(QObject *bridge)
{
    auto requestUpdate = [this]() {
        scheduleWindowUpdate();
    };

    if (auto *apertarBridge = qobject_cast<ApertarPreviewSocketClient *>(bridge)) {
        connect(apertarBridge, &ApertarPreviewSocketClient::previewFrameReady,
                this, requestUpdate, Qt::DirectConnection);
        connect(apertarBridge, &ApertarPreviewSocketClient::connectedChanged,
                this, [this, requestUpdate]() {
                    m_resetPreviewState = true;
                    m_loggedRenderUnavailable = false;
                    requestUpdate();
                }, Qt::DirectConnection);
    }
}

std::optional<PreviewFrameInfo> PreviewWindowRenderer::currentPreviewFrameFromBridge() const
{
    if (auto *apertarBridge = qobject_cast<ApertarPreviewSocketClient *>(m_bridge))
        return apertarBridge->currentPreviewFrame();

    return std::nullopt;
}

int PreviewWindowRenderer::duplicateBridgeFd(const PreviewFrameInfo &frame, int sourceFd) const
{
    if (auto *apertarBridge = qobject_cast<ApertarPreviewSocketClient *>(m_bridge))
        return apertarBridge->duplicateProducerFd(frame.procid, sourceFd);

    return -1;
}

uint64_t PreviewWindowRenderer::dmaBufferKeyForFd(int fd) const
{
    if (fd < 0)
        return 0;

    struct stat st {};
    if (::fstat(fd, &st) == 0) {
        return (static_cast<uint64_t>(static_cast<uint32_t>(st.st_dev)) << 32)
               | static_cast<uint64_t>(st.st_ino & 0xffffffffULL);
    }

    return static_cast<uint64_t>(static_cast<uint32_t>(fd));
}

void PreviewWindowRenderer::initialize()
{
    if (m_initialized)
        return;

    initializePipeline();
    m_initialized = true;
}

GLuint PreviewWindowRenderer::compileShader(GLenum type, const char *src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024]{};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        qWarning() << "Shader compile failed for type" << type << ":" << log;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

void PreviewWindowRenderer::clearLutTexture()
{
    if (m_lutPreviewTexture) {
        glDeleteTextures(1, &m_lutPreviewTexture);
        m_lutPreviewTexture = 0;
    }
    m_lutPreviewTexturePath.clear();
    m_lutPreviewTextureSize = 0;
    m_lutPreviewTextureColumns = 0;
    m_lutPreviewTextureValid = false;
}

bool PreviewWindowRenderer::ensureNeutralLutTexture()
{
    if (m_neutralLutTexture)
        return true;

    const unsigned char neutralPixel[3] = { 0, 0, 0 };
    glGenTextures(1, &m_neutralLutTexture);
    glBindTexture(GL_TEXTURE_2D, m_neutralLutTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB,
                 1,
                 1,
                 0,
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 neutralPixel);
    glBindTexture(GL_TEXTURE_2D, 0);
    return m_neutralLutTexture != 0;
}

bool PreviewWindowRenderer::ensureLutTexture(const QString &path)
{
    const QString trimmedPath = path.trimmed();
    if (trimmedPath.isEmpty())
        return false;
    const QString cleanPath = QDir::cleanPath(trimmedPath);
    if (m_lutPreviewTextureValid && m_lutPreviewTexturePath == cleanPath && m_lutPreviewTexture)
        return true;

    if (m_lutFailedPath == cleanPath && m_lutLoadingPath != cleanPath)
        return false;

    if (!m_lutLoadState)
        m_lutLoadState = std::make_shared<LutLoadState>();

    LutAtlasData atlas;
    bool haveAtlas = false;
    bool startLoad = false;
    uint64_t loadSerial = 0;

    {
        std::lock_guard<std::mutex> lock(m_lutLoadState->mutex);
        if (m_lutLoadState->readyAtlas && m_lutLoadState->readyAtlas->path == cleanPath) {
            atlas = std::move(*m_lutLoadState->readyAtlas);
            m_lutLoadState->readyAtlas.reset();
            m_lutLoadState->requestedPath.clear();
            m_lutLoadingPath.clear();
            haveAtlas = true;
        } else if (m_lutLoadingPath != cleanPath && m_lutLoadState->requestedPath != cleanPath) {
            loadSerial = ++m_lutLoadState->serial;
            m_lutLoadState->requestedPath = cleanPath;
            m_lutLoadState->readyAtlas.reset();
            m_lutLoadingPath = cleanPath;
            m_lutFailedPath.clear();
            startLoad = true;
        }
    }

    if (startLoad) {
        clearLutTexture();
        const std::shared_ptr<LutLoadState> state = m_lutLoadState;
        std::thread([state, loadSerial, cleanPath]() {
            LutAtlasData loaded = PreviewWindowRenderer::parseLutAtlasFile(cleanPath);
            std::lock_guard<std::mutex> lock(state->mutex);
            if (state->serial == loadSerial)
                state->readyAtlas = std::move(loaded);
        }).detach();
        qInfo() << "Apertar LUT preview loading asynchronously:" << cleanPath;
        return false;
    }

    if (!haveAtlas)
        return false;

    if (!atlas.error.isEmpty()) {
        clearLutTexture();
        m_lutFailedPath = cleanPath;
        qWarning() << "Apertar LUT preview:" << atlas.error << cleanPath;
        return false;
    }

    GLint maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    if (maxTextureSize > 0 && (atlas.width > maxTextureSize || atlas.height > maxTextureSize)) {
        clearLutTexture();
        m_lutFailedPath = cleanPath;
        qWarning() << "Apertar LUT preview: LUT atlas too large for GPU"
                   << cleanPath
                   << "atlas" << atlas.width << "x" << atlas.height
                   << "max" << maxTextureSize;
        return false;
    }

    clearLutTexture();
    glGenTextures(1, &m_lutPreviewTexture);
    glBindTexture(GL_TEXTURE_2D, m_lutPreviewTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB,
                 atlas.width,
                 atlas.height,
                 0,
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 atlas.pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    m_lutPreviewTexturePath = cleanPath;
    m_lutPreviewTextureSize = atlas.size;
    m_lutPreviewTextureColumns = atlas.columns;
    m_lutPreviewTextureValid = m_lutPreviewTexture != 0;
    m_lutFailedPath.clear();
    qInfo() << "Apertar LUT preview loaded:" << cleanPath << "size" << atlas.size
            << "atlas" << atlas.width << "x" << atlas.height;
    return m_lutPreviewTextureValid;
}

void PreviewWindowRenderer::bindExposureReference(GLuint program,const QVariantMap &reference,bool needed)
{
    auto [it,inserted]=m_exposureUniforms.try_emplace(program);
    auto &uniform=it->second;
    if (inserted) {
        uniform.enabled=glGetUniformLocation(program,"uExposureReferenceEnabled");
        uniform.texture=glGetUniformLocation(program,"uExposureReferenceTex");
        uniform.matrix=glGetUniformLocation(program,"uExposureTo709");
    }
    glActiveTexture(GL_TEXTURE4);
    if (needed && !reference.isEmpty() && (reference!=m_exposureReference || !m_exposureTexture)) {
        const PreviewExposureLut lut(reference);
        m_exposureReference=reference;
        m_exposureValid=lut.valid;
        if (lut.valid) {
            if (!m_exposureTexture) glGenTextures(1,&m_exposureTexture);
            glBindTexture(GL_TEXTURE_2D,m_exposureTexture);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,PreviewExposureLut::size,2,0,GL_RGBA,GL_UNSIGNED_BYTE,lut.pixels.data());
            m_exposureMatrix=lut.matrix;
            qInfo()<<"Apertar preview exposure reference: Rec709/tuning curves ready (8 KiB).";
        }
    }
    const bool enabled=needed && !reference.isEmpty() && m_exposureValid && m_exposureTexture;
    glUniform1i(uniform.enabled,enabled ? 1 : 0);
    glUniform1i(uniform.texture,4);
    glUniformMatrix3fv(uniform.matrix,1,GL_FALSE,m_exposureMatrix.data());
    // A distinct, complete 2D sampler prevents external-camera sampler aliasing,
    // even on the Standard path where the exposure transform is disabled.
    glBindTexture(GL_TEXTURE_2D,enabled ? m_exposureTexture : (ensureNeutralLutTexture() ? m_neutralLutTexture : 0));
}

void PreviewWindowRenderer::initializePipeline()
{
    m_exposureUniforms.clear();
    static const char *vs = R"(
        precision mediump float;
        attribute vec2 aPos;
        attribute vec2 aUv;
        varying vec2 vUv;
        void main() {
            vUv = aUv;
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
    )";

    static const char *fsPreamble = R"(
    #ifdef GL_FRAGMENT_PRECISION_HIGH
    precision highp float;
    #else
    precision mediump float;
    #endif

    varying vec2 vUv;
)";

    static const char *fsCommon = R"(
    uniform float uTime;

    uniform int uZebraEnabled;
    uniform float uZebraThreshold;

    uniform int uFocusPeakingEnabled;
    uniform float uFocusPeakingThreshold;
    uniform vec3 uFocusPeakingColor;

    uniform int uGrayscaleEnabled;
    uniform int uSmpteEnabled;

    uniform int uFalseColorEnabled;
    uniform int uFalseColorMode;

    uniform int uLutPreviewEnabled;
    uniform sampler2D uLutPreviewTex;
    uniform float uLutPreviewSize;
    uniform float uLutPreviewColumns;

    uniform int uExposureReferenceEnabled;
    uniform sampler2D uExposureReferenceTex;
    uniform mat3 uExposureTo709;
    float exposureLookup(float value,float row) {
        vec2 encodedPair=texture2D(uExposureReferenceTex,vec2((clamp(value,0.0,1.0)*1023.0+0.5)/1024.0,row)).rg;
        return dot(encodedPair,vec2(65280.0/65535.0,255.0/65535.0));
    }
    vec3 exposureReferenceRgb(vec3 color) {
        vec3 linear=vec3(exposureLookup(color.r,0.25),exposureLookup(color.g,0.25),exposureLookup(color.b,0.25));
        vec3 rgb=uExposureTo709*linear;
        return vec3(exposureLookup(rgb.r,0.75),exposureLookup(rgb.g,0.75),exposureLookup(rgb.b,0.75));
    }

    float luminance(vec3 c) {
        return dot(c, vec3(0.299, 0.587, 0.114));
    }

    vec3 applyPreviewLut(vec3 color) {
        float size = max(uLutPreviewSize, 2.0);
        vec3 c = clamp(color, 0.0, 1.0);
        float blue = c.b * (size - 1.0);
        float slice0 = floor(blue);
        float slice1 = min(slice0 + 1.0, size - 1.0);
        float mixB = blue - slice0;

        float columns = max(uLutPreviewColumns, 1.0);
        float rows = ceil(size / columns);
        float tileX0 = mod(slice0, columns);
        float tileY0 = floor(slice0 / columns);
        float tileX1 = mod(slice1, columns);
        float tileY1 = floor(slice1 / columns);

        vec2 uv0 = vec2((tileX0 * size + c.r * (size - 1.0) + 0.5) / (columns * size),
                        (tileY0 * size + c.g * (size - 1.0) + 0.5) / (rows * size));
        vec2 uv1 = vec2((tileX1 * size + c.r * (size - 1.0) + 0.5) / (columns * size),
                        (tileY1 * size + c.g * (size - 1.0) + 0.5) / (rows * size));
        return mix(texture2D(uLutPreviewTex, uv0).rgb,
                   texture2D(uLutPreviewTex, uv1).rgb,
                   mixB);
    }

    vec3 smpteTopBars(float x) {
        if (x < 1.0 / 7.0) return vec3(0.75, 0.75, 0.75);
        if (x < 2.0 / 7.0) return vec3(0.75, 0.75, 0.0);
        if (x < 3.0 / 7.0) return vec3(0.0, 0.75, 0.75);
        if (x < 4.0 / 7.0) return vec3(0.0, 0.75, 0.0);
        if (x < 5.0 / 7.0) return vec3(0.75, 0.0, 0.75);
        if (x < 6.0 / 7.0) return vec3(0.75, 0.0, 0.0);
        return vec3(0.0, 0.0, 0.75);
    }

    vec3 smpteMiddleBars(float x) {
        if (x < 1.0 / 7.0) return vec3(0.0, 0.0, 0.75);
        if (x < 2.0 / 7.0) return vec3(0.07, 0.07, 0.07);
        if (x < 3.0 / 7.0) return vec3(0.75, 0.0, 0.75);
        if (x < 4.0 / 7.0) return vec3(0.07, 0.07, 0.07);
        if (x < 5.0 / 7.0) return vec3(0.0, 0.75, 0.75);
        if (x < 6.0 / 7.0) return vec3(0.07, 0.07, 0.07);
        return vec3(0.75, 0.75, 0.75);
    }

    vec3 smpteBottomBars(float x) {
        if (x < 0.2) return vec3(0.0, 0.13, 0.25);
        if (x < 0.4) return vec3(1.0, 1.0, 1.0);
        if (x < 0.55) return vec3(0.2, 0.0, 0.3);
        if (x < 0.7) return vec3(0.07, 0.07, 0.07);
        if (x < 0.8) return vec3(0.15, 0.15, 0.15);
        if (x < 0.9) return vec3(0.07, 0.07, 0.07);
        return vec3(0.22, 0.22, 0.22);
    }

    vec3 smpteColorBars(vec2 uv) {
        float x = clamp(uv.x, 0.0, 0.999999);
        float y = clamp(uv.y, 0.0, 0.999999);

        if (y < 0.67)
            return smpteTopBars(x);
        if (y < 0.76)
            return smpteMiddleBars(x);
        return smpteBottomBars(x);
    }

    // ------------------------------
    // 🎬 ARRI STYLE EXPOSURE MAP
    // ------------------------------
    vec3 falseColorExposure(float l) {
        if (l > 0.95) return vec3(1.0, 0.0, 0.0);   // clip
        if (l > 0.85) return vec3(1.0, 0.5, 0.0);   // orange
        if (l > 0.70) return vec3(1.0, 1.0, 0.0);   // yellow
        if (l > 0.55) return vec3(0.6, 1.0, 0.6);   // light green
        if (l > 0.45) return vec3(0.18, 0.18, 0.18); // middle gray
        if (l > 0.35) return vec3(0.0, 1.0, 0.0);   // green
        if (l > 0.20) return vec3(0.0, 0.5, 1.0);   // blue
        if (l > 0.10) return vec3(0.0, 0.0, 1.0);   // deep blue
        return vec3(0.5, 0.0, 0.5);                 // crushed shadows
    }

    // ------------------------------
    // 🎬 SKIN TONE MODE
    // ------------------------------
    vec3 falseColorSkin(float l) {
        if (l > 0.75) return vec3(1.0, 0.0, 0.0);   // overexposed
        if (l > 0.65) return vec3(1.0, 1.0, 0.0);   // slightly hot
        if (l > 0.45) return vec3(0.0, 1.0, 0.0);   // skin sweet spot
        if (l > 0.30) return vec3(0.0, 0.5, 1.0);
        return vec3(0.0, 0.0, 1.0);
    }

    // ------------------------------
    // 🎬 HIGHLIGHT PRIORITY
    // ------------------------------
    vec3 falseColorHighlight(float l) {
        if (l > 0.98) return vec3(1.0, 0.0, 0.0);   // hard clip
        if (l > 0.92) return vec3(1.0, 0.3, 0.0);
        if (l > 0.85) return vec3(1.0, 0.7, 0.0);
        if (l > 0.70) return vec3(1.0, 1.0, 0.0);
        return vec3(l); // grayscale elsewhere
    }

    // ------------------------------
    // 🎬 SHADOW PRIORITY
    // ------------------------------
    vec3 falseColorShadow(float l) {
        if (l < 0.02) return vec3(0.5, 0.0, 0.5);   // crushed
        if (l < 0.05) return vec3(0.0, 0.0, 1.0);
        if (l < 0.10) return vec3(0.0, 0.5, 1.0);
        if (l < 0.20) return vec3(0.0, 1.0, 1.0);
        return vec3(l); // grayscale elsewhere
    }

    // ------------------------------
    float peakingLuminance(vec3 color) {
        return luminance(uExposureReferenceEnabled == 1 ? exposureReferenceRgb(color) : color);
    }
    float edgeStrength(vec2 uv, float centerLuma) {
        vec2 texel = vec2(1.0 / 1920.0, 1.0 / 1080.0);

        float r = peakingLuminance(sourceColor(uv + vec2(texel.x, 0.0)).rgb);
        float b = peakingLuminance(sourceColor(uv + vec2(0.0, texel.y)).rgb);

        return abs(r - centerLuma) + abs(b - centerLuma);
    }

    // ------------------------------
    void main() {
        vec4 src = uSmpteEnabled == 1
            ? vec4(smpteColorBars(vUv), 1.0)
            : sourceColor(vUv);
        float luma = luminance(src.rgb);
        float exposureLuma=luma;
        if (uExposureReferenceEnabled==1 && (uZebraEnabled==1 || uFalseColorEnabled==1 || uFocusPeakingEnabled==1))
            exposureLuma=luminance(exposureReferenceRgb(src.rgb));

        vec4 outColor = src;

        if (uSmpteEnabled != 1) {
            if (uLutPreviewEnabled == 1)
                outColor.rgb = applyPreviewLut(outColor.rgb);

            // False color first
            if (uFalseColorEnabled == 1) {
                if (uFalseColorMode == 0)
                    outColor.rgb = falseColorExposure(exposureLuma);
                else if (uFalseColorMode == 1)
                    outColor.rgb = falseColorSkin(exposureLuma);
                else if (uFalseColorMode == 2)
                    outColor.rgb = falseColorHighlight(exposureLuma);
                else if (uFalseColorMode == 3)
                    outColor.rgb = falseColorShadow(exposureLuma);
            } else if (uGrayscaleEnabled == 1) {
                outColor.rgb = vec3(luma);
            }

            if (uZebraEnabled == 1 && exposureLuma >= uZebraThreshold) {
                float stripe = fract((vUv.x + vUv.y) * 40.0 + uTime * 0.8);
                vec3 zebraColor = (stripe < 0.5) ? vec3(0.03) : vec3(1.0);
                outColor.rgb = mix(outColor.rgb, zebraColor, 0.72);
            }

            if (uFocusPeakingEnabled == 1) {
                float edge = edgeStrength(vUv, exposureLuma);
                if (edge > uFocusPeakingThreshold) {
                    outColor.rgb = mix(outColor.rgb, uFocusPeakingColor, 0.95);
                }
            }
        }

        gl_FragColor = outColor;
    }
)";

    static const char *passthroughFsCommon = R"(
    void main() {
        gl_FragColor = sourceColor(vUv);
    }
)";

    static const char *externalSourceCommon = R"(
    uniform SOURCE_SAMPLER uTex;
    vec4 sourceColor(vec2 uv) {
        return texture2D(uTex, uv);
    }
)";

    static const char *planarSourceCommon = R"(
    uniform sampler2D uTexY;
    uniform sampler2D uTexU;
    uniform sampler2D uTexV;

    vec4 sourceColor(vec2 uv) {
        float y = max(0.0, texture2D(uTexY, uv).r - 0.0625);
        float u = texture2D(uTexU, uv).r - 0.5;
        float v = texture2D(uTexV, uv).r - 0.5;
        vec3 rgb;
        rgb.r = 1.164383 * y + 1.792741 * v;
        rgb.g = 1.164383 * y - 0.213249 * u - 0.532909 * v;
        rgb.b = 1.164383 * y + 2.112402 * u;
        return vec4(clamp(rgb, 0.0, 1.0), 1.0);
    }
)";

    const std::string externalFs =
        "#extension GL_OES_EGL_image_external : require\n"
        "#define SOURCE_SAMPLER samplerExternalOES\n" +
        std::string(fsPreamble) +
        std::string(externalSourceCommon) +
        std::string(fsCommon);
    const std::string texture2DFs =
        "#define SOURCE_SAMPLER sampler2D\n" +
        std::string(fsPreamble) +
        std::string(externalSourceCommon) +
        std::string(fsCommon);
    const std::string planarFs =
        std::string(fsPreamble) +
        std::string(planarSourceCommon) +
        std::string(fsCommon);
    const std::string externalPassthroughFs =
        "#extension GL_OES_EGL_image_external : require\n"
        "#define SOURCE_SAMPLER samplerExternalOES\n" +
        std::string(fsPreamble) +
        std::string(externalSourceCommon) +
        std::string(passthroughFsCommon);
    const std::string texture2DPassthroughFs =
        "#define SOURCE_SAMPLER sampler2D\n" +
        std::string(fsPreamble) +
        std::string(externalSourceCommon) +
        std::string(passthroughFsCommon);
    const std::string planarPassthroughFs =
        std::string(fsPreamble) +
        std::string(planarSourceCommon) +
        std::string(passthroughFsCommon);

    GLuint vert = compileShader(GL_VERTEX_SHADER, vs);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, externalFs.c_str());
    GLuint passthroughFrag = compileShader(GL_FRAGMENT_SHADER, externalPassthroughFs.c_str());
    GLint ok = 0;
    if (vert && frag) {
        m_processProgram = glCreateProgram();
        glAttachShader(m_processProgram, vert);
        glAttachShader(m_processProgram, frag);
        glLinkProgram(m_processProgram);

        glGetProgramiv(m_processProgram, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[1024]{};
            glGetProgramInfoLog(m_processProgram, sizeof(log), nullptr, log);
            qWarning() << "Process program link failed:" << log;
            glDeleteProgram(m_processProgram);
            m_processProgram = 0;
        }
    }

    if (vert)
        glDeleteShader(vert);
    if (frag)
        glDeleteShader(frag);

    if (passthroughFrag) {
        GLuint passthroughVert = compileShader(GL_VERTEX_SHADER, vs);
        if (passthroughVert) {
            m_passthroughProgram = glCreateProgram();
            glAttachShader(m_passthroughProgram, passthroughVert);
            glAttachShader(m_passthroughProgram, passthroughFrag);
            glLinkProgram(m_passthroughProgram);

            ok = 0;
            glGetProgramiv(m_passthroughProgram, GL_LINK_STATUS, &ok);
            if (!ok) {
                char log[1024]{};
                glGetProgramInfoLog(m_passthroughProgram, sizeof(log), nullptr, log);
                qWarning() << "Passthrough program link failed:" << log;
                glDeleteProgram(m_passthroughProgram);
                m_passthroughProgram = 0;
            }
            glDeleteShader(passthroughVert);
        }
        glDeleteShader(passthroughFrag);
    }

    GLuint vert2D = compileShader(GL_VERTEX_SHADER, vs);
    GLuint frag2D = compileShader(GL_FRAGMENT_SHADER, texture2DFs.c_str());
    if (vert2D && frag2D) {
        m_processProgram2D = glCreateProgram();
        glAttachShader(m_processProgram2D, vert2D);
        glAttachShader(m_processProgram2D, frag2D);
        glLinkProgram(m_processProgram2D);

        ok = 0;
        glGetProgramiv(m_processProgram2D, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[1024]{};
            glGetProgramInfoLog(m_processProgram2D, sizeof(log), nullptr, log);
            qWarning() << "2D process program link failed:" << log;
            glDeleteProgram(m_processProgram2D);
            m_processProgram2D = 0;
        }
    }
    if (vert2D)
        glDeleteShader(vert2D);
    if (frag2D)
        glDeleteShader(frag2D);

    GLuint passthroughVert2D = compileShader(GL_VERTEX_SHADER, vs);
    GLuint passthroughFrag2D = compileShader(GL_FRAGMENT_SHADER, texture2DPassthroughFs.c_str());
    if (passthroughVert2D && passthroughFrag2D) {
        m_passthroughProgram2D = glCreateProgram();
        glAttachShader(m_passthroughProgram2D, passthroughVert2D);
        glAttachShader(m_passthroughProgram2D, passthroughFrag2D);
        glLinkProgram(m_passthroughProgram2D);

        ok = 0;
        glGetProgramiv(m_passthroughProgram2D, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[1024]{};
            glGetProgramInfoLog(m_passthroughProgram2D, sizeof(log), nullptr, log);
            qWarning() << "2D passthrough program link failed:" << log;
            glDeleteProgram(m_passthroughProgram2D);
            m_passthroughProgram2D = 0;
        }
    }
    if (passthroughVert2D)
        glDeleteShader(passthroughVert2D);
    if (passthroughFrag2D)
        glDeleteShader(passthroughFrag2D);

    GLuint vertPlanar = compileShader(GL_VERTEX_SHADER, vs);
    GLuint fragPlanar = compileShader(GL_FRAGMENT_SHADER, planarFs.c_str());
    if (vertPlanar && fragPlanar) {
        m_processProgramPlanar = glCreateProgram();
        glAttachShader(m_processProgramPlanar, vertPlanar);
        glAttachShader(m_processProgramPlanar, fragPlanar);
        glLinkProgram(m_processProgramPlanar);

        ok = 0;
        glGetProgramiv(m_processProgramPlanar, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[1024]{};
            glGetProgramInfoLog(m_processProgramPlanar, sizeof(log), nullptr, log);
            qWarning() << "Planar process program link failed:" << log;
            glDeleteProgram(m_processProgramPlanar);
            m_processProgramPlanar = 0;
        }
    }
    if (vertPlanar)
        glDeleteShader(vertPlanar);
    if (fragPlanar)
        glDeleteShader(fragPlanar);

    GLuint passthroughVertPlanar = compileShader(GL_VERTEX_SHADER, vs);
    GLuint passthroughFragPlanar = compileShader(GL_FRAGMENT_SHADER, planarPassthroughFs.c_str());
    if (passthroughVertPlanar && passthroughFragPlanar) {
        m_passthroughProgramPlanar = glCreateProgram();
        glAttachShader(m_passthroughProgramPlanar, passthroughVertPlanar);
        glAttachShader(m_passthroughProgramPlanar, passthroughFragPlanar);
        glLinkProgram(m_passthroughProgramPlanar);

        ok = 0;
        glGetProgramiv(m_passthroughProgramPlanar, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[1024]{};
            glGetProgramInfoLog(m_passthroughProgramPlanar, sizeof(log), nullptr, log);
            qWarning() << "Planar passthrough program link failed:" << log;
            glDeleteProgram(m_passthroughProgramPlanar);
            m_passthroughProgramPlanar = 0;
        }
    }
    if (passthroughVertPlanar)
        glDeleteShader(passthroughVertPlanar);
    if (passthroughFragPlanar)
        glDeleteShader(passthroughFragPlanar);

    if (m_processProgram) {
        m_processPosLoc = glGetAttribLocation(m_processProgram, "aPos");
        m_processUvLoc = glGetAttribLocation(m_processProgram, "aUv");
        m_processTexLoc = glGetUniformLocation(m_processProgram, "uTex");

        m_uTimeLoc = glGetUniformLocation(m_processProgram, "uTime");
        m_uZebraEnabledLoc = glGetUniformLocation(m_processProgram, "uZebraEnabled");
        m_uZebraThresholdLoc = glGetUniformLocation(m_processProgram, "uZebraThreshold");

        m_focusPeakingEnabledLoc = glGetUniformLocation(m_processProgram, "uFocusPeakingEnabled");
        m_focusPeakingThresholdLoc = glGetUniformLocation(m_processProgram, "uFocusPeakingThreshold");
        m_focusPeakingColorLoc = glGetUniformLocation(m_processProgram, "uFocusPeakingColor");

        m_grayscaleEnabledLoc = glGetUniformLocation(m_processProgram, "uGrayscaleEnabled");
        m_smpteEnabledLoc = glGetUniformLocation(m_processProgram, "uSmpteEnabled");

        m_falseColorEnabledLoc = glGetUniformLocation(m_processProgram, "uFalseColorEnabled");
        m_falseColorModeLoc = glGetUniformLocation(m_processProgram, "uFalseColorMode");

        m_lutPreviewEnabledLoc = glGetUniformLocation(m_processProgram, "uLutPreviewEnabled");
        m_lutPreviewTextureLoc = glGetUniformLocation(m_processProgram, "uLutPreviewTex");
        m_lutPreviewSizeLoc = glGetUniformLocation(m_processProgram, "uLutPreviewSize");
        m_lutPreviewColumnsLoc = glGetUniformLocation(m_processProgram, "uLutPreviewColumns");
    }

    if (m_passthroughProgram) {
        m_passthroughPosLoc = glGetAttribLocation(m_passthroughProgram, "aPos");
        m_passthroughUvLoc = glGetAttribLocation(m_passthroughProgram, "aUv");
        m_passthroughTexLoc = glGetUniformLocation(m_passthroughProgram, "uTex");
    }

    if (m_processProgram2D) {
        m_process2DPosLoc = glGetAttribLocation(m_processProgram2D, "aPos");
        m_process2DUvLoc = glGetAttribLocation(m_processProgram2D, "aUv");
        m_process2DTexLoc = glGetUniformLocation(m_processProgram2D, "uTex");

        m_uTime2DLoc = glGetUniformLocation(m_processProgram2D, "uTime");
        m_uZebraEnabled2DLoc = glGetUniformLocation(m_processProgram2D, "uZebraEnabled");
        m_uZebraThreshold2DLoc = glGetUniformLocation(m_processProgram2D, "uZebraThreshold");

        m_focusPeakingEnabled2DLoc = glGetUniformLocation(m_processProgram2D, "uFocusPeakingEnabled");
        m_focusPeakingThreshold2DLoc = glGetUniformLocation(m_processProgram2D, "uFocusPeakingThreshold");
        m_focusPeakingColor2DLoc = glGetUniformLocation(m_processProgram2D, "uFocusPeakingColor");

        m_grayscaleEnabled2DLoc = glGetUniformLocation(m_processProgram2D, "uGrayscaleEnabled");
        m_smpteEnabled2DLoc = glGetUniformLocation(m_processProgram2D, "uSmpteEnabled");

        m_falseColorEnabled2DLoc = glGetUniformLocation(m_processProgram2D, "uFalseColorEnabled");
        m_falseColorMode2DLoc = glGetUniformLocation(m_processProgram2D, "uFalseColorMode");

        m_lutPreviewEnabled2DLoc = glGetUniformLocation(m_processProgram2D, "uLutPreviewEnabled");
        m_lutPreviewTexture2DLoc = glGetUniformLocation(m_processProgram2D, "uLutPreviewTex");
        m_lutPreviewSize2DLoc = glGetUniformLocation(m_processProgram2D, "uLutPreviewSize");
        m_lutPreviewColumns2DLoc = glGetUniformLocation(m_processProgram2D, "uLutPreviewColumns");
    }

    if (m_passthroughProgram2D) {
        m_passthrough2DPosLoc = glGetAttribLocation(m_passthroughProgram2D, "aPos");
        m_passthrough2DUvLoc = glGetAttribLocation(m_passthroughProgram2D, "aUv");
        m_passthrough2DTexLoc = glGetUniformLocation(m_passthroughProgram2D, "uTex");
    }

    if (m_processProgramPlanar) {
        m_processPlanarPosLoc = glGetAttribLocation(m_processProgramPlanar, "aPos");
        m_processPlanarUvLoc = glGetAttribLocation(m_processProgramPlanar, "aUv");
        m_processPlanarYLoc = glGetUniformLocation(m_processProgramPlanar, "uTexY");
        m_processPlanarULoc = glGetUniformLocation(m_processProgramPlanar, "uTexU");
        m_processPlanarVLoc = glGetUniformLocation(m_processProgramPlanar, "uTexV");

        m_uTimePlanarLoc = glGetUniformLocation(m_processProgramPlanar, "uTime");
        m_uZebraEnabledPlanarLoc = glGetUniformLocation(m_processProgramPlanar, "uZebraEnabled");
        m_uZebraThresholdPlanarLoc = glGetUniformLocation(m_processProgramPlanar, "uZebraThreshold");

        m_focusPeakingEnabledPlanarLoc = glGetUniformLocation(m_processProgramPlanar, "uFocusPeakingEnabled");
        m_focusPeakingThresholdPlanarLoc = glGetUniformLocation(m_processProgramPlanar, "uFocusPeakingThreshold");
        m_focusPeakingColorPlanarLoc = glGetUniformLocation(m_processProgramPlanar, "uFocusPeakingColor");

        m_grayscaleEnabledPlanarLoc = glGetUniformLocation(m_processProgramPlanar, "uGrayscaleEnabled");
        m_smpteEnabledPlanarLoc = glGetUniformLocation(m_processProgramPlanar, "uSmpteEnabled");

        m_falseColorEnabledPlanarLoc = glGetUniformLocation(m_processProgramPlanar, "uFalseColorEnabled");
        m_falseColorModePlanarLoc = glGetUniformLocation(m_processProgramPlanar, "uFalseColorMode");

        m_lutPreviewEnabledPlanarLoc = glGetUniformLocation(m_processProgramPlanar, "uLutPreviewEnabled");
        m_lutPreviewTexturePlanarLoc = glGetUniformLocation(m_processProgramPlanar, "uLutPreviewTex");
        m_lutPreviewSizePlanarLoc = glGetUniformLocation(m_processProgramPlanar, "uLutPreviewSize");
        m_lutPreviewColumnsPlanarLoc = glGetUniformLocation(m_processProgramPlanar, "uLutPreviewColumns");
    }

    if (m_passthroughProgramPlanar) {
        m_passthroughPlanarPosLoc = glGetAttribLocation(m_passthroughProgramPlanar, "aPos");
        m_passthroughPlanarUvLoc = glGetAttribLocation(m_passthroughProgramPlanar, "aUv");
        m_passthroughPlanarYLoc = glGetUniformLocation(m_passthroughProgramPlanar, "uTexY");
        m_passthroughPlanarULoc = glGetUniformLocation(m_passthroughProgramPlanar, "uTexU");
        m_passthroughPlanarVLoc = glGetUniformLocation(m_passthroughProgramPlanar, "uTexV");
    }

    if (!m_loggedShaderPrograms) {
        qInfo() << "Apertar preview shaders:"
                << "external" << bool(m_processProgram || m_passthroughProgram)
                << "texture2D" << bool(m_processProgram2D || m_passthroughProgram2D)
                << "planarYuv" << bool(m_processProgramPlanar || m_passthroughProgramPlanar);
        m_loggedShaderPrograms = true;
    }

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(GLfloat), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void PreviewWindowRenderer::ensurePreviewTarget(const QSize &size)
{
    const QSize normalized(qMax(1, size.width()), qMax(1, size.height()));
    if (m_previewTexture && m_previewFbo && m_previewSize == normalized)
        return;

    if (m_previewTexture) {
        glDeleteTextures(1, &m_previewTexture);
        m_previewTexture = 0;
    }
    if (m_previewFbo) {
        glDeleteFramebuffers(1, &m_previewFbo);
        m_previewFbo = 0;
    }

    glGenTextures(1, &m_previewTexture);
    glBindTexture(GL_TEXTURE_2D, m_previewTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, normalized.width(), normalized.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &m_previewFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_previewFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_previewTexture, 0);
    const GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
        qWarning() << "Preview FBO incomplete:" << Qt::hex << int(fboStatus);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    m_previewSize = normalized;
    m_previewCacheState.reset();
}

void PreviewWindowRenderer::cleanupImportedBuffers()
{
    auto destroyImage = reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"));
    EGLDisplay dpy = eglGetCurrentDisplay();

    for (auto &entry : m_importedBuffers) {
        auto &buf = entry.second;
        if (buf.texture)
            glDeleteTextures(1, &buf.texture);
        if (buf.image != EGL_NO_IMAGE_KHR && destroyImage && dpy != EGL_NO_DISPLAY)
            destroyImage(dpy, buf.image);
        for (int fd : buf.dupFds) {
            if (fd >= 0)
                close(fd);
        }
        if (buf.dupFd >= 0)
            close(buf.dupFd);
    }

    m_importedBuffers.clear();
    m_currentTexture = 0;
    m_importedProcId = -1;
    m_importedWidth = 0;
    m_importedHeight = 0;
    m_importedStride = 0;
    m_importedCaptureWidth = 0;
    m_importedCaptureHeight = 0;
    m_lastImportedSequence = 0;
    m_haveImportedSequence = false;
    m_lastRenderableFrame.reset();
    m_currentTextureIsPlanar = false;
}

bool PreviewWindowRenderer::uploadPlanarYuvTexture(const PreviewFrameInfo &frame,
                                                   int sourceFd,
                                                   unsigned int width,
                                                   unsigned int height,
                                                   unsigned int stride)
{
    if (sourceFd < 0 || width == 0 || height == 0 || stride == 0 ||
        (!m_processProgramPlanar && !m_passthroughProgramPlanar)) {
        return false;
    }

    const bool hasExplicitPlanes =
        frame.planeCount >= 3 &&
        frame.planePitches[0] > 0 &&
        frame.planePitches[1] > 0 &&
        frame.planePitches[2] > 0;

    const bool hasPlaneFds = hasExplicitPlanes &&
        frame.planeFds[0] >= 0 &&
        frame.planeFds[1] >= 0 &&
        frame.planeFds[2] >= 0;

    const unsigned int yOffset = hasExplicitPlanes ? frame.planeOffsets[0] : 0;
    const unsigned int uOffset = hasExplicitPlanes ? frame.planeOffsets[1] : stride * height;
    const unsigned int vOffset = hasExplicitPlanes
        ? frame.planeOffsets[2]
        : stride * height + (std::max(1u, stride / 2u) * ((height + 1u) / 2u));
    const unsigned int yPitch = hasExplicitPlanes ? frame.planePitches[0] : stride;
    const unsigned int uPitch = hasExplicitPlanes ? frame.planePitches[1] : std::max(1u, stride / 2u);
    const unsigned int vPitch = hasExplicitPlanes ? frame.planePitches[2] : uPitch;
    const unsigned int chromaWidth = (width + 1u) / 2u;
    const unsigned int chromaHeight = (height + 1u) / 2u;

    const size_t yEnd = size_t(yOffset) + size_t(yPitch) * size_t(height);
    const size_t uEnd = size_t(uOffset) + size_t(uPitch) * size_t(chromaHeight);
    const size_t vEnd = size_t(vOffset) + size_t(vPitch) * size_t(chromaHeight);
    size_t mapBytes = std::max(yEnd, uEnd);
    mapBytes = std::max(mapBytes, vEnd);
    if (mapBytes == 0)
        return false;

    const int yFd = hasPlaneFds ? frame.planeFds[0] : sourceFd;
    const int uFd = hasPlaneFds ? frame.planeFds[1] : sourceFd;
    const int vFd = hasPlaneFds ? frame.planeFds[2] : sourceFd;

    const size_t yMapBytes = (hasPlaneFds && (uFd != yFd || vFd != yFd)) ? yEnd : mapBytes;
    void *mappedY = ::mmap(nullptr, yMapBytes, PROT_READ, MAP_SHARED, yFd, 0);
    if (mappedY == MAP_FAILED)
        return false;

    void *mappedU = mappedY;
    void *mappedV = mappedY;
    size_t uMapBytes = yMapBytes;
    size_t vMapBytes = yMapBytes;

    if (hasPlaneFds && uFd != yFd) {
        uMapBytes = uEnd;
        mappedU = ::mmap(nullptr, uMapBytes, PROT_READ, MAP_SHARED, uFd, 0);
        if (mappedU == MAP_FAILED) {
            ::munmap(mappedY, yMapBytes);
            return false;
        }
    }

    if (hasPlaneFds && vFd != yFd && vFd != uFd) {
        vMapBytes = vEnd;
        mappedV = ::mmap(nullptr, vMapBytes, PROT_READ, MAP_SHARED, vFd, 0);
        if (mappedV == MAP_FAILED) {
            if (mappedU != mappedY)
                ::munmap(mappedU, uMapBytes);
            ::munmap(mappedY, yMapBytes);
            return false;
        }
    } else if (hasPlaneFds && vFd == uFd) {
        mappedV = mappedU;
        vMapBytes = uMapBytes;
    }

    const auto *yBase = static_cast<const unsigned char *>(mappedY);
    const auto *uBase = static_cast<const unsigned char *>(mappedU);
    const auto *vBase = static_cast<const unsigned char *>(mappedV);
    const unsigned char *yPlane = yBase + yOffset;
    const unsigned char *uPlane = uBase + uOffset;
    const unsigned char *vPlane = vBase + vOffset;

    uploadLumaPlane(&m_planarTextures[0], &m_planarTextureSizes[0], yPlane, width, height, yPitch);
    uploadLumaPlane(&m_planarTextures[1], &m_planarTextureSizes[1], uPlane, chromaWidth, chromaHeight, uPitch);
    uploadLumaPlane(&m_planarTextures[2], &m_planarTextureSizes[2], vPlane, chromaWidth, chromaHeight, vPitch);

    if (mappedV != mappedY && mappedV != mappedU)
        ::munmap(mappedV, vMapBytes);
    if (mappedU != mappedY)
        ::munmap(mappedU, uMapBytes);
    ::munmap(mappedY, yMapBytes);

    if (!m_planarTextures[0] || !m_planarTextures[1] || !m_planarTextures[2])
        return false;

    if (!m_loggedPlanarFallback) {
        qWarning() << "Apertar preview using planar YUV texture path for"
                   << width << "x" << height
                   << "stride" << stride;
        m_loggedPlanarFallback = true;
    }

    m_currentTexture = m_planarTextures[0];
    m_currentTextureTarget = GL_TEXTURE_2D;
    m_currentTextureIsPlanar = true;
    m_importedProcId = frame.procid;
    m_importedWidth = width;
    m_importedHeight = height;
    m_importedStride = stride;
    m_importedCaptureWidth = frame.captureWidth;
    m_importedCaptureHeight = frame.captureHeight;
    return true;
}

bool PreviewWindowRenderer::uploadCpuFallbackTexture(const PreviewFrameInfo &frame,
                                                     int sourceFd,
                                                     unsigned int width,
                                                     unsigned int height,
                                                     unsigned int stride)
{
    if (sourceFd < 0 || width == 0 || height == 0 || stride == 0 ||
        (!m_processProgram2D && !m_passthroughProgram2D)) {
        return false;
    }

    const QSize textureSize{int(width), int(height)};

    const bool hasExplicitPlanes =
        frame.planeCount >= 3 &&
        frame.planePitches[0] > 0 &&
        frame.planePitches[1] > 0 &&
        frame.planePitches[2] > 0;

    const bool hasPlaneFds = hasExplicitPlanes &&
        frame.planeFds[0] >= 0 &&
        frame.planeFds[1] >= 0 &&
        frame.planeFds[2] >= 0;

    const unsigned int yOffset = hasExplicitPlanes ? frame.planeOffsets[0] : 0;
    const unsigned int uOffset = hasExplicitPlanes ? frame.planeOffsets[1] : stride * height;
    const unsigned int vOffset = hasExplicitPlanes
        ? frame.planeOffsets[2]
        : stride * height + (std::max(1u, stride / 2u) * ((height + 1u) / 2u));
    const unsigned int yPitch = hasExplicitPlanes ? frame.planePitches[0] : stride;
    const unsigned int chromaStride = hasExplicitPlanes ? frame.planePitches[1] : std::max(1u, stride / 2u);
    const unsigned int vStride = hasExplicitPlanes ? frame.planePitches[2] : chromaStride;
    const unsigned int chromaHeight = (height + 1u) / 2u;
    const size_t yEnd = size_t(yOffset) + size_t(yPitch) * size_t(height);
    const size_t uEnd = size_t(uOffset) + size_t(chromaStride) * size_t(chromaHeight);
    const size_t vEnd = size_t(vOffset) + size_t(vStride) * size_t(chromaHeight);
    size_t mapBytes = std::max(yEnd, uEnd);
    mapBytes = std::max(mapBytes, vEnd);
    if (mapBytes == 0)
        return false;

    const int yFd = hasPlaneFds ? frame.planeFds[0] : sourceFd;
    const int uFd = hasPlaneFds ? frame.planeFds[1] : sourceFd;
    const int vFd = hasPlaneFds ? frame.planeFds[2] : sourceFd;

    const size_t yMapBytes = (hasPlaneFds && (uFd != yFd || vFd != yFd)) ? yEnd : mapBytes;
    void *mapped = ::mmap(nullptr, yMapBytes, PROT_READ, MAP_SHARED, yFd, 0);
    void *mappedU = MAP_FAILED;
    void *mappedV = MAP_FAILED;
    if (mapped == MAP_FAILED) {
        if (!m_loggedCpuFallbackFailure) {
            qWarning() << "Apertar preview CPU fallback mmap failed for"
                       << width << "x" << height
                       << "stride" << stride;
            m_loggedCpuFallbackFailure = true;
        }
        return false;
    }

    const auto *base = static_cast<const unsigned char *>(mapped);
    const unsigned char *yPlane = base + yOffset;
    const unsigned char *uPlane = base + uOffset;
    const unsigned char *vPlane = base + vOffset;

    if (hasPlaneFds && (uFd != yFd || vFd != yFd)) {
        const size_t uMapBytes = size_t(uOffset) + size_t(chromaStride) * size_t(chromaHeight);
        const size_t vMapBytes = size_t(vOffset) + size_t(vStride) * size_t(chromaHeight);
        mappedU = uFd == yFd ? mapped : ::mmap(nullptr, uMapBytes, PROT_READ, MAP_SHARED, uFd, 0);
        mappedV = vFd == yFd ? mapped : (vFd == uFd ? mappedU : ::mmap(nullptr, vMapBytes, PROT_READ, MAP_SHARED, vFd, 0));
        if (mappedU == MAP_FAILED || mappedV == MAP_FAILED) {
            if (mappedV != MAP_FAILED && mappedV != mapped && mappedV != mappedU)
                ::munmap(mappedV, vMapBytes);
            if (mappedU != MAP_FAILED && mappedU != mapped)
                ::munmap(mappedU, uMapBytes);
            ::munmap(mapped, yMapBytes);
            return false;
        }
        uPlane = static_cast<const unsigned char *>(mappedU) + uOffset;
        vPlane = static_cast<const unsigned char *>(mappedV) + vOffset;
    }

    const size_t rgbaBytes = size_t(width) * size_t(height) * 4u;
    if (m_cpuRgbaBuffer.size() != rgbaBytes)
        m_cpuRgbaBuffer.resize(rgbaBytes);

    auto clamp8 = [](int value) -> unsigned char {
        return static_cast<unsigned char>(std::max(0, std::min(255, value)));
    };

    unsigned char *dst = m_cpuRgbaBuffer.data();
    for (unsigned int row = 0; row < height; ++row) {
        const unsigned char *yRow = yPlane + size_t(row) * yPitch;
        const unsigned char *uRow = uPlane + size_t(row / 2u) * chromaStride;
        const unsigned char *vRow = vPlane + size_t(row / 2u) * vStride;
        for (unsigned int col = 0; col < width; ++col) {
            const int y = std::max(0, int(yRow[col]) - 16);
            const int u = int(uRow[col / 2u]) - 128;
            const int v = int(vRow[col / 2u]) - 128;

            // Rec.709 limited-range YUV420 to RGB. This path is a compatibility
            // fallback for VC4/Pi3 when direct EGL DMABUF import is unavailable.
            const int r = (298 * y + 459 * v + 128) >> 8;
            const int g = (298 * y - 55 * u - 136 * v + 128) >> 8;
            const int b = (298 * y + 541 * u + 128) >> 8;

            *dst++ = clamp8(r);
            *dst++ = clamp8(g);
            *dst++ = clamp8(b);
            *dst++ = 255;
        }
    }

    if (mappedV != MAP_FAILED && mappedV != mapped && mappedV != mappedU)
        ::munmap(mappedV, size_t(vOffset) + size_t(vStride) * size_t(chromaHeight));
    if (mappedU != MAP_FAILED && mappedU != mapped)
        ::munmap(mappedU, size_t(uOffset) + size_t(chromaStride) * size_t(chromaHeight));
    ::munmap(mapped, yMapBytes);

    if (!m_cpuFallbackTexture)
        glGenTextures(1, &m_cpuFallbackTexture);

    glBindTexture(GL_TEXTURE_2D, m_cpuFallbackTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (m_cpuFallbackTextureSize != textureSize) {
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     int(width),
                     int(height),
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     m_cpuRgbaBuffer.data());
        m_cpuFallbackTextureSize = textureSize;
    } else {
        glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        0,
                        0,
                        int(width),
                        int(height),
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        m_cpuRgbaBuffer.data());
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    if (!m_loggedCpuFallback) {
        qWarning() << "Apertar preview using CPU texture fallback for"
                   << width << "x" << height
                   << "stride" << stride;
        m_loggedCpuFallback = true;
    }

    m_currentTexture = m_cpuFallbackTexture;
    m_currentTextureTarget = GL_TEXTURE_2D;
    m_currentTextureIsPlanar = false;
    m_importedProcId = frame.procid;
    m_importedWidth = width;
    m_importedHeight = height;
    m_importedStride = stride;
    m_importedCaptureWidth = frame.captureWidth;
    m_importedCaptureHeight = frame.captureHeight;
    return true;
}

bool PreviewWindowRenderer::ensurePlanarProcessingTexture(const PreviewFrameInfo &frame)
{
    if (m_currentTextureIsPlanar)
        return true;

    if (m_currentTextureTarget != GL_TEXTURE_EXTERNAL_OES || !m_processProgramPlanar)
        return false;

    const bool uploaded =
        uploadPlanarYuvTexture(frame, frame.fdIsp, frame.width, frame.height, frame.stride) ||
        uploadPlanarYuvTexture(frame, frame.fallbackFdIsp, frame.fallbackWidth, frame.fallbackHeight, frame.fallbackStride);
    if (!uploaded)
        return false;

    if (!m_loggedPlanarProcessingPath) {
        qInfo() << "Apertar preview: using planar YUV processing path for preview aids.";
        m_loggedPlanarProcessingPath = true;
    }

    m_previewCacheState.reset();
    return true;
}

void PreviewWindowRenderer::ensureImported(const PreviewFrameInfo &frame)
{
    if (!m_bridge)
        return;

    const bool sequenceRestarted =
        m_haveImportedSequence && frame.sequence < m_lastImportedSequence;
    auto layoutMatches = [&](unsigned int width, unsigned int height, unsigned int stride) {
        return width > 0 &&
               height > 0 &&
               stride > 0 &&
               m_importedWidth == width &&
               m_importedHeight == height &&
               m_importedStride == stride;
    };
    const bool primaryLayoutMatches = layoutMatches(frame.width, frame.height, frame.stride);
    const bool fallbackLayoutMatches =
        frame.fallbackFdIsp >= 0 &&
        layoutMatches(frame.fallbackWidth, frame.fallbackHeight, frame.fallbackStride);
    const bool captureModeChanged =
        frame.captureWidth > 0 &&
        frame.captureHeight > 0 &&
        m_importedCaptureWidth > 0 &&
        m_importedCaptureHeight > 0 &&
        (m_importedCaptureWidth != frame.captureWidth ||
         m_importedCaptureHeight != frame.captureHeight);
    const bool previewLayoutChanged =
        sequenceRestarted ||
        captureModeChanged ||
        m_importedProcId != frame.procid ||
        (!primaryLayoutMatches && !fallbackLayoutMatches);

    if (previewLayoutChanged) {
        cleanupImportedBuffers();
        m_previewCacheState.reset();
    }
    m_lastImportedSequence = frame.sequence;
    m_haveImportedSequence = true;

    auto tryImportSource = [&](int sourceFd, unsigned int width, unsigned int height, unsigned int stride) -> bool {
        if (sourceFd < 0 || width == 0 || height == 0 || stride == 0)
            return false;

        if (m_forceCpuFallback)
            return uploadCpuFallbackTexture(frame, sourceFd, width, height, stride);

        if (m_forcePlanarFallback)
            return uploadPlanarYuvTexture(frame, sourceFd, width, height, stride);

        if (!m_processProgram && !m_passthroughProgram)
            return uploadPlanarYuvTexture(frame, sourceFd, width, height, stride) ||
                   uploadCpuFallbackTexture(frame, sourceFd, width, height, stride);

        const uint64_t frameBufferKey = dmaBufferKeyForFd(sourceFd);
        if (frameBufferKey == 0)
            return false;
        auto it = m_importedBuffers.find(frameBufferKey);
        if (it != m_importedBuffers.end()) {
            const ImportedBuffer &existing = it->second;
            const bool matchesFrame =
                existing.sourceProcId == frame.procid &&
                existing.sourceBufferKey == frameBufferKey &&
                existing.width == width &&
                existing.height == height &&
                existing.stride == stride &&
                existing.captureWidth == frame.captureWidth &&
                existing.captureHeight == frame.captureHeight;

            if (matchesFrame) {
                m_currentTexture = existing.texture;
                m_currentTextureTarget = GL_TEXTURE_EXTERNAL_OES;
                m_currentTextureIsPlanar = false;
                m_importedProcId = frame.procid;
                m_importedWidth = width;
                m_importedHeight = height;
                m_importedStride = stride;
                m_importedCaptureWidth = frame.captureWidth;
                m_importedCaptureHeight = frame.captureHeight;
                return true;
            }

            auto destroyImage = reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"));
            EGLDisplay dpy = eglGetCurrentDisplay();
            ImportedBuffer stale = it->second;
            m_importedBuffers.erase(it);
            m_currentTexture = 0;

            if (stale.texture)
                glDeleteTextures(1, &stale.texture);
            if (stale.image != EGL_NO_IMAGE_KHR && destroyImage && dpy != EGL_NO_DISPLAY)
                destroyImage(dpy, stale.image);
            for (int fd : stale.dupFds) {
                if (fd >= 0)
                    close(fd);
            }
            if (stale.dupFd >= 0)
                close(stale.dupFd);
        }

        auto createImage = reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
        auto imageTarget = reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));

        if (!createImage || !imageTarget) {
            return false;
        }

        EGLDisplay dpy = eglGetCurrentDisplay();
        if (dpy == EGL_NO_DISPLAY) {
            return false;
        }

        // Pi4/Pi5 preview buffers are imported most reliably as one DMABUF
        // with three logical YUV420 planes. The Pi3 planar path exits before
        // this EGL path, so keep the normal VC4/V3D path on the proven layout.
        const bool hasExplicitPlanes = false;

        std::array<int, 3> dupFds = { -1, -1, -1 };
        std::array<unsigned int, 3> offsets = {
            0,
            stride * height,
            stride * height + (stride / 2) * (height / 2)
        };
        std::array<unsigned int, 3> pitches = {
            stride,
            std::max(1u, stride / 2),
            std::max(1u, stride / 2)
        };

        for (int i = 0; i < 3; ++i) {
            const int planeSourceFd = hasExplicitPlanes && frame.planeFds[i] >= 0
                ? frame.planeFds[i]
                : sourceFd;
            if (hasExplicitPlanes) {
                offsets[i] = frame.planeOffsets[i];
                pitches[i] = frame.planePitches[i];
            }

            dupFds[i] = duplicateBridgeFd(frame, planeSourceFd);
            if (dupFds[i] < 0) {
                for (int fd : dupFds) {
                    if (fd >= 0)
                        close(fd);
                }
                return false;
            }
        }

        std::array<EGLint, 29> attr = {
            EGL_WIDTH, static_cast<EGLint>(width),
            EGL_HEIGHT, static_cast<EGLint>(height),
            EGL_LINUX_DRM_FOURCC_EXT, DRM_FORMAT_YUV420,
            EGL_DMA_BUF_PLANE0_FD_EXT, dupFds[0],
            EGL_DMA_BUF_PLANE0_OFFSET_EXT, static_cast<EGLint>(offsets[0]),
            EGL_DMA_BUF_PLANE0_PITCH_EXT, static_cast<EGLint>(pitches[0]),
            EGL_DMA_BUF_PLANE1_FD_EXT, dupFds[1],
            EGL_DMA_BUF_PLANE1_OFFSET_EXT, static_cast<EGLint>(offsets[1]),
            EGL_DMA_BUF_PLANE1_PITCH_EXT, static_cast<EGLint>(pitches[1]),
            EGL_DMA_BUF_PLANE2_FD_EXT, dupFds[2],
            EGL_DMA_BUF_PLANE2_OFFSET_EXT, static_cast<EGLint>(offsets[2]),
            EGL_DMA_BUF_PLANE2_PITCH_EXT, static_cast<EGLint>(pitches[2]),
            EGL_YUV_COLOR_SPACE_HINT_EXT, EGL_ITU_REC709_EXT,
            EGL_SAMPLE_RANGE_HINT_EXT, EGL_YUV_NARROW_RANGE_EXT,
            EGL_NONE
        };

        EGLImageKHR image = createImage(dpy, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, attr.data());
        if (image == EGL_NO_IMAGE_KHR) {
            qWarning() << "eglCreateImageKHR failed"
                       << width << "x" << height
                       << "stride" << stride
                       << "source fd" << sourceFd
                       << "error 0x" << QString::number(eglGetError(), 16);
            for (int fd : dupFds) {
                if (fd >= 0)
                    close(fd);
            }
            return false;
        }

        GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        imageTarget(GL_TEXTURE_EXTERNAL_OES, image);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);

        ImportedBuffer imported;
        imported.texture = texture;
        imported.image = image;
        imported.dupFd = -1;
        imported.dupFds = dupFds;
        imported.sourceProcId = frame.procid;
        imported.sourceBufferKey = frameBufferKey;
        imported.sourceFd = sourceFd;
        imported.sourceRawFd = frame.fdRaw;
        imported.width = width;
        imported.height = height;
        imported.stride = stride;
        imported.captureWidth = frame.captureWidth;
        imported.captureHeight = frame.captureHeight;

        m_importedBuffers.emplace(frameBufferKey, imported);
        m_currentTexture = texture;
        m_currentTextureTarget = GL_TEXTURE_EXTERNAL_OES;
        m_currentTextureIsPlanar = false;
        m_importedProcId = frame.procid;
        m_importedWidth = width;
        m_importedHeight = height;
        m_importedStride = stride;
        m_importedCaptureWidth = frame.captureWidth;
        m_importedCaptureHeight = frame.captureHeight;
        if (!m_loggedEglImport) {
            qInfo() << "Apertar preview EGL import:"
                    << width << "x" << height
                    << "stride" << stride
                    << "planes" << frame.planeCount
                    << "target external";
            m_loggedEglImport = true;
        }
        if (m_loggedImportWidth != width ||
            m_loggedImportHeight != height ||
            m_loggedImportStride != stride ||
            m_loggedImportCaptureWidth != frame.captureWidth ||
            m_loggedImportCaptureHeight != frame.captureHeight) {
            m_loggedImportWidth = width;
            m_loggedImportHeight = height;
            m_loggedImportStride = stride;
            m_loggedImportCaptureWidth = frame.captureWidth;
            m_loggedImportCaptureHeight = frame.captureHeight;
        }
        return true;
    };

    if (tryImportSource(frame.fdIsp, frame.width, frame.height, frame.stride))
        return;

    if (tryImportSource(frame.fallbackFdIsp, frame.fallbackWidth, frame.fallbackHeight, frame.fallbackStride))
        return;

    if (m_allowCpuFallback && !m_forceCpuFallback) {
        if (uploadCpuFallbackTexture(frame, frame.fdIsp, frame.width, frame.height, frame.stride))
            return;

        if (uploadCpuFallbackTexture(frame, frame.fallbackFdIsp, frame.fallbackWidth, frame.fallbackHeight, frame.fallbackStride))
            return;
    }

    m_currentTexture = 0;
}

double PreviewWindowRenderer::previewDisplayAspect(const PreviewFrameInfo &frame) const
{
    const double width = frame.captureWidth > 0 ? frame.captureWidth : frame.width;
    const double height = frame.captureHeight > 0 ? frame.captureHeight : frame.height;
    double aspect = width > 0.0 && height > 0.0 ? width / height : 16.0 / 9.0;
    if (m_item) {
        if (m_item->previewAspectRatio() > 0.01f)
            aspect = m_item->previewAspectRatio();
        if (!m_item->smpteEnabled() && m_item->anamorphicDesqueezeEnabled())
            aspect *= std::max(1.0f, m_item->anamorphicDesqueezeRatio());
        if (m_item->displayRotation() == 90 || m_item->displayRotation() == -90)
            aspect = 1.0 / aspect;
    }
    return aspect;
}

QRectF PreviewWindowRenderer::previewRectInPixels(const PreviewFrameInfo &frame) const
{
    if (!m_item || !m_window)
        return {};

    QRectF rect = m_item->mapRectToScene(QRectF(0, 0, m_item->width(), m_item->height()));

    const qreal dpr = m_window->effectiveDevicePixelRatio();

    QRectF pixelRect(
        rect.x() * dpr,
        rect.y() * dpr,
        rect.width() * dpr,
        rect.height() * dpr
    );

    return centeredAspectFitRect(pixelRect, previewDisplayAspect(frame));
}

PreviewWindowRenderer::PreviewCacheState PreviewWindowRenderer::currentPreviewState(const PreviewFrameInfo &frame,
                                                                                   const QSize &size) const
{
    PreviewCacheState state;
    state.frame = frame.frame;
    state.size = size;
    // UVs address the imported ISP image, which may have a different aspect
    // from RAW capture (processed output, fallback buffers, or ISP alignment).
    state.sourceWidth = m_importedWidth > 0 ? m_importedWidth : frame.width;
    state.sourceHeight = m_importedHeight > 0 ? m_importedHeight : frame.height;

    if (m_item) {
        state.zoom = std::max(1.0f, m_item->zoom());
        state.panX = std::max(-1.0f, std::min(1.0f, m_item->panX()));
        state.panY = std::max(-1.0f, std::min(1.0f, m_item->panY()));
        state.zebraEnabled = m_item->zebraEnabled();
        state.zebraThreshold = m_item->zebraThreshold();
        state.focusPeakingEnabled = m_item->focusPeakingEnabled();
        state.focusPeakingThreshold = m_item->focusPeakingThreshold();
        state.focusPeakingColor = m_item->focusPeakingColor();
        state.grayscaleEnabled = m_item->grayscaleEnabled();
        state.smpteEnabled = m_item->smpteEnabled();
        state.anamorphicDesqueezeEnabled = m_item->anamorphicDesqueezeEnabled();
        state.anamorphicDesqueezeRatio = std::max(1.0f, m_item->anamorphicDesqueezeRatio());
        state.falseColorEnabled = m_item->falseColorEnabled();
        state.falseColorMode = m_item->falseColorMode();
        state.lutPreviewEnabled = m_item->lutPreviewEnabled();
        state.exposureReference = m_item->exposureReference();
        const QString lutFile = m_item->lutPreviewFile().trimmed();
        state.lutPreviewFile = lutFile.isEmpty() ? QString() : QDir::cleanPath(lutFile);
        state.displayRotation = m_item->displayRotation();
        state.previewAspectRatio = m_item->previewAspectRatio();
    }

    if (state.smpteEnabled) {
        state.frame = 0;
        state.zoom = 1.0f;
        state.panX = 0.0f;
        state.panY = 0.0f;
        state.anamorphicDesqueezeEnabled = false;
        state.lutPreviewEnabled = false;
    }

    return state;
}

bool PreviewWindowRenderer::previewStateChanged(const PreviewCacheState &state) const
{
    if (!m_previewCacheState.has_value())
        return true;

    const PreviewCacheState &cached = *m_previewCacheState;
    return cached.frame != state.frame
        || cached.size != state.size
        || cached.sourceWidth != state.sourceWidth
        || cached.sourceHeight != state.sourceHeight
        || !qFuzzyCompare(cached.zoom, state.zoom)
        || !qFuzzyCompare(cached.panX, state.panX)
        || !qFuzzyCompare(cached.panY, state.panY)
        || cached.zebraEnabled != state.zebraEnabled
        || !qFuzzyCompare(cached.zebraThreshold, state.zebraThreshold)
        || cached.focusPeakingEnabled != state.focusPeakingEnabled
        || !qFuzzyCompare(cached.focusPeakingThreshold, state.focusPeakingThreshold)
        || cached.focusPeakingColor != state.focusPeakingColor
        || cached.grayscaleEnabled != state.grayscaleEnabled
        || cached.smpteEnabled != state.smpteEnabled
        || cached.anamorphicDesqueezeEnabled != state.anamorphicDesqueezeEnabled
        || !qFuzzyCompare(cached.anamorphicDesqueezeRatio, state.anamorphicDesqueezeRatio)
        || cached.falseColorEnabled != state.falseColorEnabled
        || cached.falseColorMode != state.falseColorMode
        || cached.lutPreviewEnabled != state.lutPreviewEnabled
        || cached.lutPreviewFile != state.lutPreviewFile
        || cached.exposureReference != state.exposureReference
        || cached.displayRotation != state.displayRotation
        || !qFuzzyCompare(cached.previewAspectRatio + 1.0f, state.previewAspectRatio + 1.0f);
}

void PreviewWindowRenderer::renderProcessedPreview(const PreviewFrameInfo &frame, const QSize &size)
{
    ensurePreviewTarget(size);
    if (!m_previewFbo || !m_currentTexture)
        return;

    const PreviewCacheState state = currentPreviewState(frame, size);
    if (!previewStateChanged(state))
        return;

    const PreviewUvRect uv = croppedPreviewUvRect(state.sourceWidth,
                                                  state.sourceHeight,
                                                  state.previewAspectRatio,
                                                  state.zoom,
                                                  state.panX,
                                                  state.panY);
    const float u0 = uv.u0;
    const float u1 = uv.u1;
    const float v0 = uv.v0;
    const float v1 = uv.v1;

    float xScale = 1.0f;
    float yScale = 1.0f;

    const float displayAspect = float(previewDisplayAspect(frame));
    const float targetAspect = float(size.width()) / float(size.height());
    if (displayAspect > targetAspect) {
        yScale = targetAspect / displayAspect;
    } else if (displayAspect < targetAspect) {
        xScale = displayAspect / targetAspect;
    }

    GLfloat blU = u0, blV = v1;
    GLfloat brU = u1, brV = v1;
    GLfloat tlU = u0, tlV = v0;
    GLfloat trU = u1, trV = v0;

    if (state.displayRotation == 90) {
        blU = u0; blV = v0;
        brU = u0; brV = v1;
        tlU = u1; tlV = v0;
        trU = u1; trV = v1;
    } else if (state.displayRotation == -90) {
        blU = u1; blV = v1;
        brU = u1; brV = v0;
        tlU = u0; tlV = v1;
        trU = u0; trV = v0;
    } else if (state.displayRotation == 180) {
        blU = u1; blV = v0;
        brU = u0; brV = v0;
        tlU = u1; tlV = v1;
        trU = u0; trV = v1;
    }

    const GLfloat quad[] = {
        -xScale, -yScale, blU, blV,
         xScale, -yScale, brU, brV,
        -xScale,  yScale, tlU, tlV,
         xScale,  yScale, trU, trV,
    };

    glBindFramebuffer(GL_FRAMEBUFFER, m_previewFbo);
    glViewport(0, 0, size.width(), size.height());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    const bool lutPreviewReady = state.lutPreviewEnabled
                                 && !state.lutPreviewFile.isEmpty()
                                 && ensureLutTexture(state.lutPreviewFile);
    const bool needsProcessing = state.zebraEnabled
                                 || state.focusPeakingEnabled
                                 || state.grayscaleEnabled
                                 || state.smpteEnabled
                                 || state.falseColorEnabled
                                 || lutPreviewReady;
    const bool planarTexture = m_currentTextureIsPlanar;
    const bool cpuTexture = m_currentTextureTarget == GL_TEXTURE_2D && !planarTexture;
    const GLuint processProgram = planarTexture ? m_processProgramPlanar : (cpuTexture ? m_processProgram2D : m_processProgram);
    const GLuint passthroughProgram = planarTexture ? m_passthroughProgramPlanar : (cpuTexture ? m_passthroughProgram2D : m_passthroughProgram);

    const bool usePassthrough = !needsProcessing && passthroughProgram;
    const GLuint activeProgram = usePassthrough ? passthroughProgram : processProgram;
    if (!activeProgram) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }
    const GLint activePosLoc = usePassthrough
        ? (planarTexture ? m_passthroughPlanarPosLoc : (cpuTexture ? m_passthrough2DPosLoc : m_passthroughPosLoc))
        : (planarTexture ? m_processPlanarPosLoc : (cpuTexture ? m_process2DPosLoc : m_processPosLoc));
    const GLint activeUvLoc = usePassthrough
        ? (planarTexture ? m_passthroughPlanarUvLoc : (cpuTexture ? m_passthrough2DUvLoc : m_passthroughUvLoc))
        : (planarTexture ? m_processPlanarUvLoc : (cpuTexture ? m_process2DUvLoc : m_processUvLoc));
    const GLint activeTexLoc = usePassthrough
        ? (cpuTexture ? m_passthrough2DTexLoc : m_passthroughTexLoc)
        : (cpuTexture ? m_process2DTexLoc : m_processTexLoc);
    const GLint activeYLoc = usePassthrough ? m_passthroughPlanarYLoc : m_processPlanarYLoc;
    const GLint activeULoc = usePassthrough ? m_passthroughPlanarULoc : m_processPlanarULoc;
    const GLint activeVLoc = usePassthrough ? m_passthroughPlanarVLoc : m_processPlanarVLoc;

    glUseProgram(activeProgram);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quad), quad);

    glEnableVertexAttribArray(activePosLoc);
    glEnableVertexAttribArray(activeUvLoc);
    glVertexAttribPointer(activePosLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), reinterpret_cast<void *>(0));
    glVertexAttribPointer(activeUvLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), reinterpret_cast<void *>(2 * sizeof(GLfloat)));

    if (planarTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_planarTextures[0]);
        glUniform1i(activeYLoc, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_planarTextures[1]);
        glUniform1i(activeULoc, 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_planarTextures[2]);
        glUniform1i(activeVLoc, 2);
    } else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(m_currentTextureTarget, m_currentTexture);
        glUniform1i(activeTexLoc, 0);
    }

    if (!usePassthrough) {
        const float timeSeconds = float(m_shaderTimer.elapsed()) / 1000.0f;
        glUniform1f(planarTexture ? m_uTimePlanarLoc : (cpuTexture ? m_uTime2DLoc : m_uTimeLoc), timeSeconds);
        glUniform1i(planarTexture ? m_uZebraEnabledPlanarLoc : (cpuTexture ? m_uZebraEnabled2DLoc : m_uZebraEnabledLoc), state.zebraEnabled ? 1 : 0);
        glUniform1f(planarTexture ? m_uZebraThresholdPlanarLoc : (cpuTexture ? m_uZebraThreshold2DLoc : m_uZebraThresholdLoc), state.zebraThreshold);
        glUniform1i(planarTexture ? m_focusPeakingEnabledPlanarLoc : (cpuTexture ? m_focusPeakingEnabled2DLoc : m_focusPeakingEnabledLoc), state.focusPeakingEnabled ? 1 : 0);
        glUniform1f(planarTexture ? m_focusPeakingThresholdPlanarLoc : (cpuTexture ? m_focusPeakingThreshold2DLoc : m_focusPeakingThresholdLoc), state.focusPeakingThreshold);
        glUniform1i(planarTexture ? m_grayscaleEnabledPlanarLoc : (cpuTexture ? m_grayscaleEnabled2DLoc : m_grayscaleEnabledLoc), state.grayscaleEnabled ? 1 : 0);
        glUniform1i(planarTexture ? m_smpteEnabledPlanarLoc : (cpuTexture ? m_smpteEnabled2DLoc : m_smpteEnabledLoc), state.smpteEnabled ? 1 : 0);
        glUniform1i(planarTexture ? m_falseColorEnabledPlanarLoc : (cpuTexture ? m_falseColorEnabled2DLoc : m_falseColorEnabledLoc), state.falseColorEnabled ? 1 : 0);
        glUniform1i(planarTexture ? m_falseColorModePlanarLoc : (cpuTexture ? m_falseColorMode2DLoc : m_falseColorModeLoc), state.falseColorMode);
        glUniform1i(planarTexture ? m_lutPreviewEnabledPlanarLoc : (cpuTexture ? m_lutPreviewEnabled2DLoc : m_lutPreviewEnabledLoc), lutPreviewReady ? 1 : 0);
        glUniform1f(planarTexture ? m_lutPreviewSizePlanarLoc : (cpuTexture ? m_lutPreviewSize2DLoc : m_lutPreviewSizeLoc),
                    float(std::max(2, m_lutPreviewTextureSize)));
        glUniform1f(planarTexture ? m_lutPreviewColumnsPlanarLoc : (cpuTexture ? m_lutPreviewColumns2DLoc : m_lutPreviewColumnsLoc),
                    float(std::max(1, m_lutPreviewTextureColumns)));
        // Keep the LUT sampler off texture unit 0 even when LUT preview is disabled.
        // Some GLES drivers render black when samplerExternalOES and sampler2D uniforms
        // both point at the camera texture unit.
        glUniform1i(planarTexture ? m_lutPreviewTexturePlanarLoc : (cpuTexture ? m_lutPreviewTexture2DLoc : m_lutPreviewTextureLoc), 3);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D,
                      lutPreviewReady ? m_lutPreviewTexture
                                      : (ensureNeutralLutTexture() ? m_neutralLutTexture : 0));

        bindExposureReference(activeProgram,state.exposureReference,!state.smpteEnabled && (state.zebraEnabled || state.falseColorEnabled || state.focusPeakingEnabled));
        QVector3D peakColor(1.0f, 0.1f, 0.1f);
        if (state.focusPeakingColor == "Green")
            peakColor = QVector3D(0.1f, 1.0f, 0.1f);
        else if (state.focusPeakingColor == "Blue")
            peakColor = QVector3D(63.0f / 255.0f, 208.0f / 255.0f, 1.0f);
        else if (state.focusPeakingColor == "Yellow")
            peakColor = QVector3D(1.0f, 1.0f, 0.1f);
        else if (state.focusPeakingColor == "Pink")
            peakColor = QVector3D(1.0f, 0.2f, 0.7f);
        glUniform3f(planarTexture ? m_focusPeakingColorPlanarLoc : (cpuTexture ? m_focusPeakingColor2DLoc : m_focusPeakingColorLoc),
                    peakColor.x(),
                    peakColor.y(),
                    peakColor.z());
    }

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    if (!usePassthrough) {
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    if (planarTexture) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    } else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(m_currentTextureTarget, 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (QOpenGLContext *context = QOpenGLContext::currentContext()) {
        if (QOpenGLExtraFunctions *extra = context->extraFunctions())
            extra->glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    }
    m_previewCacheState = state;
}

void PreviewWindowRenderer::blitPreviewToWindow(const QRectF &rect, float winH)
{
    if (!m_previewFbo)
        return;

    QOpenGLContext *context = QOpenGLContext::currentContext();
    if (!context)
        return;

    QOpenGLExtraFunctions *extra = context->extraFunctions();
    if (!extra)
        return;

    const GLint dstLeft = GLint(std::round(rect.left()));
    const GLint dstRight = GLint(std::round(rect.right()));
    const GLint dstBottom = GLint(std::round(winH - rect.bottom()));
    const GLint dstTop = GLint(std::round(winH - rect.top()));
    const GLuint drawFbo = context->defaultFramebufferObject();

    extra->glBindFramebuffer(GL_READ_FRAMEBUFFER, m_previewFbo);
    extra->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFbo);
    extra->glBlitFramebuffer(
        0, 0, m_previewSize.width(), m_previewSize.height(),
        dstLeft, dstBottom, dstRight, dstTop,
        GL_COLOR_BUFFER_BIT, GL_LINEAR);
    extra->glBindFramebuffer(GL_READ_FRAMEBUFFER, drawFbo);
    extra->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFbo);
}

void PreviewWindowRenderer::renderDirectToWindow(const PreviewFrameInfo &frame, const QRectF &rect, float winH)
{
    const bool planarTexture = m_currentTextureIsPlanar;
    const bool cpuTexture = m_currentTextureTarget == GL_TEXTURE_2D && !planarTexture;
    const GLuint processProgram = planarTexture ? m_processProgramPlanar : (cpuTexture ? m_processProgram2D : m_processProgram);
    const GLuint passthroughProgram = planarTexture ? m_passthroughProgramPlanar : (cpuTexture ? m_passthroughProgram2D : m_passthroughProgram);
    if (!m_currentTexture || !m_window)
        return;

    QOpenGLContext *context = QOpenGLContext::currentContext();
    if (!context)
        return;

    const QSize targetSize(qMax(1, int(std::round(rect.width()))),
                           qMax(1, int(std::round(rect.height()))));
    const PreviewCacheState state = currentPreviewState(frame, targetSize);

    const PreviewUvRect uv = croppedPreviewUvRect(state.sourceWidth,
                                                  state.sourceHeight,
                                                  state.previewAspectRatio,
                                                  state.zoom,
                                                  state.panX,
                                                  state.panY);
    const float u0 = uv.u0;
    const float u1 = uv.u1;
    const float v0 = uv.v0;
    const float v1 = uv.v1;

    float xScale = 1.0f;
    float yScale = 1.0f;

    const float displayAspect = float(previewDisplayAspect(frame));
    const float targetAspect = float(targetSize.width()) / float(targetSize.height());
    if (displayAspect > targetAspect) {
        yScale = targetAspect / displayAspect;
    } else if (displayAspect < targetAspect) {
        xScale = displayAspect / targetAspect;
    }

    GLfloat blU = u0, blV = v1;
    GLfloat brU = u1, brV = v1;
    GLfloat tlU = u0, tlV = v0;
    GLfloat trU = u1, trV = v0;

    if (state.displayRotation == 90) {
        blU = u0; blV = v0;
        brU = u0; brV = v1;
        tlU = u1; tlV = v0;
        trU = u1; trV = v1;
    } else if (state.displayRotation == -90) {
        blU = u1; blV = v1;
        brU = u1; brV = v0;
        tlU = u0; tlV = v1;
        trU = u0; trV = v0;
    } else if (state.displayRotation == 180) {
        blU = u1; blV = v0;
        brU = u0; brV = v0;
        tlU = u1; tlV = v1;
        trU = u0; trV = v1;
    }

    const GLfloat quad[] = {
        -xScale, -yScale, blU, blV,
         xScale, -yScale, brU, brV,
        -xScale,  yScale, tlU, tlV,
         xScale,  yScale, trU, trV,
    };

    const GLuint drawFbo = context->defaultFramebufferObject();
    const GLint dstLeft = GLint(std::round(rect.left()));
    const GLint dstBottom = GLint(std::round(winH - rect.bottom()));

    glBindFramebuffer(GL_FRAMEBUFFER, drawFbo);
    glViewport(dstLeft, dstBottom, targetSize.width(), targetSize.height());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    const bool lutPreviewReady = state.lutPreviewEnabled
                                 && !state.lutPreviewFile.isEmpty()
                                 && ensureLutTexture(state.lutPreviewFile);
    const bool needsProcessing = state.zebraEnabled
                                 || state.focusPeakingEnabled
                                 || state.grayscaleEnabled
                                 || state.smpteEnabled
                                 || state.falseColorEnabled
                                 || lutPreviewReady;
    const bool usePassthrough = !needsProcessing && passthroughProgram;
    const GLuint activeProgram = usePassthrough ? passthroughProgram : processProgram;
    if (!activeProgram)
        return;
    const GLint activePosLoc = usePassthrough
        ? (planarTexture ? m_passthroughPlanarPosLoc : (cpuTexture ? m_passthrough2DPosLoc : m_passthroughPosLoc))
        : (planarTexture ? m_processPlanarPosLoc : (cpuTexture ? m_process2DPosLoc : m_processPosLoc));
    const GLint activeUvLoc = usePassthrough
        ? (planarTexture ? m_passthroughPlanarUvLoc : (cpuTexture ? m_passthrough2DUvLoc : m_passthroughUvLoc))
        : (planarTexture ? m_processPlanarUvLoc : (cpuTexture ? m_process2DUvLoc : m_processUvLoc));
    const GLint activeTexLoc = usePassthrough
        ? (cpuTexture ? m_passthrough2DTexLoc : m_passthroughTexLoc)
        : (cpuTexture ? m_process2DTexLoc : m_processTexLoc);
    const GLint activeYLoc = usePassthrough ? m_passthroughPlanarYLoc : m_processPlanarYLoc;
    const GLint activeULoc = usePassthrough ? m_passthroughPlanarULoc : m_processPlanarULoc;
    const GLint activeVLoc = usePassthrough ? m_passthroughPlanarVLoc : m_processPlanarVLoc;

    glUseProgram(activeProgram);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quad), quad);

    glEnableVertexAttribArray(activePosLoc);
    glEnableVertexAttribArray(activeUvLoc);
    glVertexAttribPointer(activePosLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), reinterpret_cast<void *>(0));
    glVertexAttribPointer(activeUvLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), reinterpret_cast<void *>(2 * sizeof(GLfloat)));

    if (planarTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_planarTextures[0]);
        glUniform1i(activeYLoc, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_planarTextures[1]);
        glUniform1i(activeULoc, 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_planarTextures[2]);
        glUniform1i(activeVLoc, 2);
    } else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(m_currentTextureTarget, m_currentTexture);
        glUniform1i(activeTexLoc, 0);
    }

    if (!usePassthrough) {
        const float timeSeconds = float(m_shaderTimer.elapsed()) / 1000.0f;
        glUniform1f(planarTexture ? m_uTimePlanarLoc : (cpuTexture ? m_uTime2DLoc : m_uTimeLoc), timeSeconds);
        glUniform1i(planarTexture ? m_uZebraEnabledPlanarLoc : (cpuTexture ? m_uZebraEnabled2DLoc : m_uZebraEnabledLoc), state.zebraEnabled ? 1 : 0);
        glUniform1f(planarTexture ? m_uZebraThresholdPlanarLoc : (cpuTexture ? m_uZebraThreshold2DLoc : m_uZebraThresholdLoc), state.zebraThreshold);
        glUniform1i(planarTexture ? m_focusPeakingEnabledPlanarLoc : (cpuTexture ? m_focusPeakingEnabled2DLoc : m_focusPeakingEnabledLoc), state.focusPeakingEnabled ? 1 : 0);
        glUniform1f(planarTexture ? m_focusPeakingThresholdPlanarLoc : (cpuTexture ? m_focusPeakingThreshold2DLoc : m_focusPeakingThresholdLoc), state.focusPeakingThreshold);
        glUniform1i(planarTexture ? m_grayscaleEnabledPlanarLoc : (cpuTexture ? m_grayscaleEnabled2DLoc : m_grayscaleEnabledLoc), state.grayscaleEnabled ? 1 : 0);
        glUniform1i(planarTexture ? m_smpteEnabledPlanarLoc : (cpuTexture ? m_smpteEnabled2DLoc : m_smpteEnabledLoc), state.smpteEnabled ? 1 : 0);
        glUniform1i(planarTexture ? m_falseColorEnabledPlanarLoc : (cpuTexture ? m_falseColorEnabled2DLoc : m_falseColorEnabledLoc), state.falseColorEnabled ? 1 : 0);
        glUniform1i(planarTexture ? m_falseColorModePlanarLoc : (cpuTexture ? m_falseColorMode2DLoc : m_falseColorModeLoc), state.falseColorMode);
        glUniform1i(planarTexture ? m_lutPreviewEnabledPlanarLoc : (cpuTexture ? m_lutPreviewEnabled2DLoc : m_lutPreviewEnabledLoc), lutPreviewReady ? 1 : 0);
        glUniform1f(planarTexture ? m_lutPreviewSizePlanarLoc : (cpuTexture ? m_lutPreviewSize2DLoc : m_lutPreviewSizeLoc),
                    float(std::max(2, m_lutPreviewTextureSize)));
        glUniform1f(planarTexture ? m_lutPreviewColumnsPlanarLoc : (cpuTexture ? m_lutPreviewColumns2DLoc : m_lutPreviewColumnsLoc),
                    float(std::max(1, m_lutPreviewTextureColumns)));
        // Keep the LUT sampler off texture unit 0 even when LUT preview is disabled.
        // Some GLES drivers render black when samplerExternalOES and sampler2D uniforms
        // both point at the camera texture unit.
        glUniform1i(planarTexture ? m_lutPreviewTexturePlanarLoc : (cpuTexture ? m_lutPreviewTexture2DLoc : m_lutPreviewTextureLoc), 3);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D,
                      lutPreviewReady ? m_lutPreviewTexture
                                      : (ensureNeutralLutTexture() ? m_neutralLutTexture : 0));

        bindExposureReference(activeProgram,state.exposureReference,!state.smpteEnabled && (state.zebraEnabled || state.falseColorEnabled || state.focusPeakingEnabled));
        QVector3D peakColor(1.0f, 0.1f, 0.1f);
        if (state.focusPeakingColor == "Green")
            peakColor = QVector3D(0.1f, 1.0f, 0.1f);
        else if (state.focusPeakingColor == "Blue")
            peakColor = QVector3D(63.0f / 255.0f, 208.0f / 255.0f, 1.0f);
        else if (state.focusPeakingColor == "Yellow")
            peakColor = QVector3D(1.0f, 1.0f, 0.1f);
        else if (state.focusPeakingColor == "Pink")
            peakColor = QVector3D(1.0f, 0.2f, 0.7f);
        glUniform3f(planarTexture ? m_focusPeakingColorPlanarLoc : (cpuTexture ? m_focusPeakingColor2DLoc : m_focusPeakingColorLoc),
                    peakColor.x(),
                    peakColor.y(),
                    peakColor.z());
    }

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    if (!usePassthrough) {
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    if (planarTexture) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    } else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(m_currentTextureTarget, 0);
    }
    m_previewCacheState = state;
}

void PreviewWindowRenderer::render()
{
    if (!m_window || !m_item)
        return;

    m_updatePending = false;
    m_window->beginExternalCommands();

    initialize();

    if (m_resetPreviewState) {
        cleanupImportedBuffers();
        m_previewCacheState.reset();
        m_resetPreviewState = false;
        m_loggedRenderUnavailable = false;
    }

    std::optional<PreviewFrameInfo> latestFrame = currentPreviewFrameFromBridge();
    if (latestFrame) {
        ensureImported(*latestFrame);
        if (m_currentTexture)
            m_lastRenderableFrame = latestFrame;
    } else if (m_currentTexture && m_lastRenderableFrame) {
        latestFrame = m_lastRenderableFrame;
    }

    const bool planarTexture = m_currentTextureIsPlanar;
    const bool cpuTexture = m_currentTextureTarget == GL_TEXTURE_2D && !planarTexture;
    const bool hasRenderableProgram = planarTexture
        ? bool(m_processProgramPlanar || m_passthroughProgramPlanar)
        : (cpuTexture
            ? bool(m_processProgram2D || m_passthroughProgram2D)
            : bool(m_processProgram || m_passthroughProgram));
    if (!m_currentTexture || !latestFrame || !hasRenderableProgram) {
        if (!m_loggedRenderUnavailable) {
            qWarning() << "Apertar preview render unavailable:"
                       << "hasFrame" << bool(latestFrame)
                       << "texture" << m_currentTexture
                       << "target" << (planarTexture ? "planarYuv" : (cpuTexture ? "texture2D" : "external"))
                       << "externalPrograms" << bool(m_processProgram || m_passthroughProgram)
                       << "texture2DPrograms" << bool(m_processProgram2D || m_passthroughProgram2D)
                       << "planarPrograms" << bool(m_processProgramPlanar || m_passthroughProgramPlanar);
            m_loggedRenderUnavailable = true;
        }
        m_window->endExternalCommands();
        return;
    }

    QRectF r = previewRectInPixels(*latestFrame);
    if (r.isEmpty()) {
        m_window->endExternalCommands();
        return;
    }

    const float winH = float(m_window->height() * m_window->effectiveDevicePixelRatio());
    const QSize targetSize(qMax(1, int(std::round(r.width()))),
                           qMax(1, int(std::round(r.height()))));
    const PreviewCacheState drawState = currentPreviewState(*latestFrame, targetSize);
    const bool previewAidEnabled =
        drawState.zebraEnabled ||
        drawState.focusPeakingEnabled ||
        drawState.grayscaleEnabled ||
        drawState.smpteEnabled ||
        drawState.falseColorEnabled ||
        (drawState.lutPreviewEnabled && !drawState.lutPreviewFile.isEmpty());
    const bool hasProcessingProgram = planarTexture
        ? bool(m_processProgramPlanar)
        : (cpuTexture ? bool(m_processProgram2D) : bool(m_processProgram));
    if (previewAidEnabled && !hasProcessingProgram)
        ensurePlanarProcessingTexture(*latestFrame);

    if (m_directWindowRendering) {
        renderDirectToWindow(*latestFrame, r, winH);
    } else {
        renderProcessedPreview(*latestFrame, targetSize);
        blitPreviewToWindow(r, winH);
    }
    m_window->endExternalCommands();
}
