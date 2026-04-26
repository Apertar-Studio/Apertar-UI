#include "PreviewWindowRenderer.hpp"
#include "ApertarPreviewSocketClient.hpp"
#include "CameraPreviewItem.hpp"

#include <QQuickWindow>
#include <QQuickItem>
#include <QMatrix4x4>
#include <QDebug>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>

#include <QVector3D>

#include <unistd.h>
#include <libdrm/drm_fourcc.h>
#include <array>
#include <algorithm>

PreviewWindowRenderer::PreviewWindowRenderer(QObject *parent)
    : QObject(parent)
{
    m_shaderTimer.start();
}

PreviewWindowRenderer::~PreviewWindowRenderer()
{
    cleanupImportedBuffers();
    if (m_processProgram)
        glDeleteProgram(m_processProgram);
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
                this, requestUpdate, Qt::QueuedConnection);
        connect(apertarBridge, &ApertarPreviewSocketClient::connectedChanged,
                this, requestUpdate, Qt::QueuedConnection);
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

void PreviewWindowRenderer::initializePipeline()
{
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

    static const char *fs = R"(
    #extension GL_OES_EGL_image_external : require
    precision mediump float;

    varying vec2 vUv;
    uniform samplerExternalOES uTex;

    uniform float uTime;

    uniform int uZebraEnabled;
    uniform float uZebraThreshold;

    uniform int uFocusPeakingEnabled;
    uniform float uFocusPeakingThreshold;
    uniform vec3 uFocusPeakingColor;

    uniform int uGrayscaleEnabled;

    uniform int uFalseColorEnabled;
    uniform int uFalseColorMode;

    float luminance(vec3 c) {
        return dot(c, vec3(0.299, 0.587, 0.114));
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
    float edgeStrength(vec2 uv) {
        vec2 texel = vec2(1.0 / 1920.0, 1.0 / 1080.0);

        float c = luminance(texture2D(uTex, uv).rgb);
        float r = luminance(texture2D(uTex, uv + vec2(texel.x, 0.0)).rgb);
        float b = luminance(texture2D(uTex, uv + vec2(0.0, texel.y)).rgb);

        return abs(r - c) + abs(b - c);
    }

    // ------------------------------
    void main() {
        vec4 src = texture2D(uTex, vUv);
        float luma = luminance(src.rgb);

        vec4 outColor = src;

        // ✅ FALSE COLOR FIRST
        if (uFalseColorEnabled == 1) {
            if (uFalseColorMode == 0)
                outColor.rgb = falseColorExposure(luma);
            else if (uFalseColorMode == 1)
                outColor.rgb = falseColorSkin(luma);
            else if (uFalseColorMode == 2)
                outColor.rgb = falseColorHighlight(luma);
            else if (uFalseColorMode == 3)
                outColor.rgb = falseColorShadow(luma);
        }
        // ✅ GRAYSCALE ONLY IF FALSE COLOR IS OFF
        else if (uGrayscaleEnabled == 1) {
            outColor.rgb = vec3(luma);
        }

        // ✅ ZEBRA
        if (uZebraEnabled == 1 && luma >= uZebraThreshold) {
            float stripe = fract((vUv.x + vUv.y) * 40.0 + uTime * 0.8);
            vec3 zebraColor = (stripe < 0.5) ? vec3(0.03) : vec3(1.0);
            outColor.rgb = mix(outColor.rgb, zebraColor, 0.72);
        }

        // ✅ FOCUS PEAKING ALWAYS LAST
        if (uFocusPeakingEnabled == 1) {
            float edge = edgeStrength(vUv);
            if (edge > uFocusPeakingThreshold) {
                outColor.rgb = mix(outColor.rgb, uFocusPeakingColor, 0.95);
            }
        }

        gl_FragColor = outColor;
    }
)";

    GLuint vert = compileShader(GL_VERTEX_SHADER, vs);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fs);
    if (!vert || !frag) {
        if (vert)
            glDeleteShader(vert);
        if (frag)
            glDeleteShader(frag);
        return;
    }

    m_processProgram = glCreateProgram();
    glAttachShader(m_processProgram, vert);
    glAttachShader(m_processProgram, frag);
    glLinkProgram(m_processProgram);

    GLint ok = 0;
    glGetProgramiv(m_processProgram, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024]{};
        glGetProgramInfoLog(m_processProgram, sizeof(log), nullptr, log);
        qWarning() << "Process program link failed:" << log;
        glDeleteProgram(m_processProgram);
        m_processProgram = 0;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);

    if (!m_processProgram)
        return;

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

    m_falseColorEnabledLoc = glGetUniformLocation(m_processProgram, "uFalseColorEnabled");
    m_falseColorModeLoc = glGetUniformLocation(m_processProgram, "uFalseColorMode");

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
}

void PreviewWindowRenderer::ensureImported(const PreviewFrameInfo &frame)
{
    if (!m_bridge)
        return;

    const bool transientFds = true;
    const bool sequenceRestarted =
        m_haveImportedSequence && frame.sequence < m_lastImportedSequence;
    const bool transientFrameChanged =
        transientFds && m_haveImportedSequence && frame.sequence != m_lastImportedSequence;
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
        transientFrameChanged ||
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

        const int frameBufferKey = transientFds ? 0 : sourceFd;
        auto it = m_importedBuffers.find(frameBufferKey);
        if (it != m_importedBuffers.end()) {
            const ImportedBuffer &existing = it->second;
            const bool matchesFrame =
                existing.sourceProcId == frame.procid &&
                existing.sourceFd == sourceFd &&
                existing.width == width &&
                existing.height == height &&
                existing.stride == stride &&
                existing.captureWidth == frame.captureWidth &&
                existing.captureHeight == frame.captureHeight;

            if (matchesFrame) {
                m_currentTexture = existing.texture;
                m_importedProcId = frame.procid;
                m_importedWidth = width;
                m_importedHeight = height;
                m_importedStride = stride;
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

        // PiSP exports YUV420 preview as a single DMABUF with three logical
        // planes. Importing that proven single-fd layout is more reliable
        // across the Qt/EGL stack than passing separate duplicated fds per
        // plane, even when libcamera reports the logical plane metadata.
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
            const int planeSourceFd = hasExplicitPlanes ? frame.planeFds[i] : sourceFd;
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
        imported.sourceFd = sourceFd;
        imported.sourceRawFd = frame.fdRaw;
        imported.width = width;
        imported.height = height;
        imported.stride = stride;
        imported.captureWidth = frame.captureWidth;
        imported.captureHeight = frame.captureHeight;

        m_importedBuffers.emplace(frameBufferKey, imported);
        m_currentTexture = texture;
        m_importedProcId = frame.procid;
        m_importedWidth = width;
        m_importedHeight = height;
        m_importedStride = stride;
        m_importedCaptureWidth = frame.captureWidth;
        m_importedCaptureHeight = frame.captureHeight;
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

    m_currentTexture = 0;
}

QRectF PreviewWindowRenderer::previewRectInPixels() const
{
    if (!m_item || !m_window)
        return {};

    QRectF rect = m_item->mapRectToScene(QRectF(0, 0, m_item->width(), m_item->height()));

    const qreal dpr = m_window->effectiveDevicePixelRatio();

    return QRectF(
        rect.x() * dpr,
        rect.y() * dpr,
        rect.width() * dpr,
        rect.height() * dpr
    );
}

PreviewWindowRenderer::PreviewCacheState PreviewWindowRenderer::currentPreviewState(const PreviewFrameInfo &frame,
                                                                                   const QSize &size) const
{
    PreviewCacheState state;
    state.frame = frame.frame;
    state.size = size;

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
        state.anamorphicDesqueezeEnabled = m_item->anamorphicDesqueezeEnabled();
        state.anamorphicDesqueezeRatio = std::max(1.0f, m_item->anamorphicDesqueezeRatio());
        state.falseColorEnabled = m_item->falseColorEnabled();
        state.falseColorMode = m_item->falseColorMode();
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
        || !qFuzzyCompare(cached.zoom, state.zoom)
        || !qFuzzyCompare(cached.panX, state.panX)
        || !qFuzzyCompare(cached.panY, state.panY)
        || cached.zebraEnabled != state.zebraEnabled
        || !qFuzzyCompare(cached.zebraThreshold, state.zebraThreshold)
        || cached.focusPeakingEnabled != state.focusPeakingEnabled
        || !qFuzzyCompare(cached.focusPeakingThreshold, state.focusPeakingThreshold)
        || cached.focusPeakingColor != state.focusPeakingColor
        || cached.grayscaleEnabled != state.grayscaleEnabled
        || cached.anamorphicDesqueezeEnabled != state.anamorphicDesqueezeEnabled
        || !qFuzzyCompare(cached.anamorphicDesqueezeRatio, state.anamorphicDesqueezeRatio)
        || cached.falseColorEnabled != state.falseColorEnabled
        || cached.falseColorMode != state.falseColorMode;
}

void PreviewWindowRenderer::renderProcessedPreview(const PreviewFrameInfo &frame, const QSize &size)
{
    ensurePreviewTarget(size);
    if (!m_previewFbo || !m_currentTexture)
        return;

    const PreviewCacheState state = currentPreviewState(frame, size);
    if (!previewStateChanged(state))
        return;

    const float uvHalfRange = 0.5f / state.zoom;
    const float maxOffset = 0.5f - uvHalfRange;
    const float centerU = 0.5f + state.panX * maxOffset;
    const float centerV = 0.5f + state.panY * maxOffset;
    const float u0 = centerU - uvHalfRange;
    const float u1 = centerU + uvHalfRange;
    const float v0 = centerV - uvHalfRange;
    const float v1 = centerV + uvHalfRange;

    float xScale = 1.0f;
    float yScale = 1.0f;

    if (state.anamorphicDesqueezeEnabled) {
        float displayAspect = 1.0f;
        const float sourceWidth = frame.captureWidth > 0 ? float(frame.captureWidth) : float(frame.width);
        const float sourceHeight = frame.captureHeight > 0 ? float(frame.captureHeight) : float(frame.height);
        if (sourceWidth > 0.0f && sourceHeight > 0.0f)
            displayAspect = (sourceWidth / sourceHeight) * std::max(1.0f, state.anamorphicDesqueezeRatio);

        const float targetAspect = float(size.width()) / float(size.height());
        if (displayAspect > targetAspect) {
            yScale = targetAspect / displayAspect;
        } else if (displayAspect < targetAspect) {
            xScale = displayAspect / targetAspect;
        }
    }

    const GLfloat quad[] = {
        -xScale, -yScale, u0, v1,
         xScale, -yScale, u1, v1,
        -xScale,  yScale, u0, v0,
         xScale,  yScale, u1, v0,
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

    glUseProgram(m_processProgram);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quad), quad);

    glEnableVertexAttribArray(m_processPosLoc);
    glEnableVertexAttribArray(m_processUvLoc);
    glVertexAttribPointer(m_processPosLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), reinterpret_cast<void *>(0));
    glVertexAttribPointer(m_processUvLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), reinterpret_cast<void *>(2 * sizeof(GLfloat)));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, m_currentTexture);
    glUniform1i(m_processTexLoc, 0);

    const float timeSeconds = float(m_shaderTimer.elapsed()) / 1000.0f;
    glUniform1f(m_uTimeLoc, timeSeconds);
    glUniform1i(m_uZebraEnabledLoc, state.zebraEnabled ? 1 : 0);
    glUniform1f(m_uZebraThresholdLoc, state.zebraThreshold);
    glUniform1i(m_focusPeakingEnabledLoc, state.focusPeakingEnabled ? 1 : 0);
    glUniform1f(m_focusPeakingThresholdLoc, state.focusPeakingThreshold);
    glUniform1i(m_grayscaleEnabledLoc, state.grayscaleEnabled ? 1 : 0);
    glUniform1i(m_falseColorEnabledLoc, state.falseColorEnabled ? 1 : 0);
    glUniform1i(m_falseColorModeLoc, state.falseColorMode);

    QVector3D peakColor(1.0f, 0.1f, 0.1f);
    if (state.focusPeakingColor == "Green")
        peakColor = QVector3D(0.1f, 1.0f, 0.1f);
    else if (state.focusPeakingColor == "Blue")
        peakColor = QVector3D(0.1f, 0.5f, 1.0f);
    else if (state.focusPeakingColor == "Yellow")
        peakColor = QVector3D(1.0f, 1.0f, 0.1f);
    else if (state.focusPeakingColor == "Pink")
        peakColor = QVector3D(1.0f, 0.2f, 0.7f);
    glUniform3f(m_focusPeakingColorLoc, peakColor.x(), peakColor.y(), peakColor.z());

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
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

void PreviewWindowRenderer::render()
{
    if (!m_window || !m_item)
        return;

    m_updatePending = false;
    m_window->beginExternalCommands();

    initialize();

    std::optional<PreviewFrameInfo> latestFrame = currentPreviewFrameFromBridge();
    if (latestFrame)
        ensureImported(*latestFrame);

    if (!m_processProgram || !m_currentTexture || !latestFrame) {
        m_window->endExternalCommands();
        return;
    }

    QRectF r = previewRectInPixels();
    if (r.isEmpty()) {
        m_window->endExternalCommands();
        return;
    }

    const float winH = float(m_window->height() * m_window->effectiveDevicePixelRatio());
    const QSize targetSize(qMax(1, int(std::round(r.width()))),
                           qMax(1, int(std::round(r.height()))));
    renderProcessedPreview(*latestFrame, targetSize);
    blitPreviewToWindow(r, winH);
    m_window->endExternalCommands();
}
