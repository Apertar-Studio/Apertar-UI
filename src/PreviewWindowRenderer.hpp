#pragma once

#include <QObject>
#include <QPointer>
#include <QRectF>
#include <QElapsedTimer>
#include <QSize>
#include <QVariantMap>
#include <array>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <cstdint>
#include <vector>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "PreviewFrameInfo.hpp"

class QQuickWindow;
class CameraPreviewItem;

class PreviewWindowRenderer : public QObject
{
    Q_OBJECT

public:
    explicit PreviewWindowRenderer(QObject *parent = nullptr);
    ~PreviewWindowRenderer() override;

    void setPreviewItem(CameraPreviewItem *item);
    void setDirectWindowRendering(bool enabled) { m_directWindowRendering = enabled; }

private slots:
    void handleWindowChanged(QQuickWindow *window);
    void syncState();
    void render();

private:
    struct ImportedBuffer
    {
        GLuint texture = 0;
        EGLImageKHR image = EGL_NO_IMAGE_KHR;
        int dupFd = -1;
        std::array<int, 3> dupFds = { -1, -1, -1 };
        int sourceProcId = -1;
        int sourceFd = -1;
        uint64_t sourceBufferKey = 0;
        int sourceRawFd = -1;
        unsigned int width = 0;
        unsigned int height = 0;
        unsigned int stride = 0;
        unsigned int captureWidth = 0;
        unsigned int captureHeight = 0;
    };

    void scheduleWindowUpdate();
    void initialize();
    void initializePipeline();
    void ensurePreviewTarget(const QSize &size);
    GLuint compileShader(GLenum type, const char *src);
    std::optional<PreviewFrameInfo> currentPreviewFrameFromBridge() const;
    int duplicateBridgeFd(const PreviewFrameInfo &frame, int sourceFd) const;
    uint64_t dmaBufferKeyForFd(int fd) const;
    void connectBridgeSignals(QObject *bridge);
    void ensureImported(const PreviewFrameInfo &frame);
    void cleanupImportedBuffers();
    double previewDisplayAspect(const PreviewFrameInfo &frame) const;
    QRectF previewRectInPixels(const PreviewFrameInfo &frame) const;
    void renderProcessedPreview(const PreviewFrameInfo &frame, const QSize &size);
    void renderDirectToWindow(const PreviewFrameInfo &frame, const QRectF &rect, float winH);
    void blitPreviewToWindow(const QRectF &rect, float winH);
    bool uploadPlanarYuvTexture(const PreviewFrameInfo &frame,
                                int sourceFd,
                                unsigned int width,
                                unsigned int height,
                                unsigned int stride);
    bool uploadCpuFallbackTexture(const PreviewFrameInfo &frame,
                                  int sourceFd,
                                  unsigned int width,
                                  unsigned int height,
                                  unsigned int stride);
    bool ensurePlanarProcessingTexture(const PreviewFrameInfo &frame);
    bool ensureLutTexture(const QString &path);
    bool ensureNeutralLutTexture();
    void clearLutTexture();
    void bindExposureReference(GLuint program,const QVariantMap &reference,bool needed);
    struct ExposureUniforms { GLint enabled=-1, texture=-1, matrix=-1; };
    std::unordered_map<GLuint,ExposureUniforms> m_exposureUniforms;
    GLuint m_exposureTexture=0;
    QVariantMap m_exposureReference;
    std::array<float,9> m_exposureMatrix{};
    bool m_exposureValid=false;

    struct LutAtlasData
    {
        QString path;
        QString error;
        int size = 0;
        int columns = 0;
        int width = 0;
        int height = 0;
        std::vector<unsigned char> pixels;
    };

    struct LutLoadState
    {
        std::mutex mutex;
        uint64_t serial = 0;
        QString requestedPath;
        std::optional<LutAtlasData> readyAtlas;
    };

    static LutAtlasData parseLutAtlasFile(const QString &cleanPath);

    struct PreviewCacheState
    {
        uint64_t frame = 0;
        QSize size;
        unsigned int sourceWidth = 0;
        unsigned int sourceHeight = 0;
        float zoom = 1.0f;
        float panX = 0.0f;
        float panY = 0.0f;
        bool zebraEnabled = false;
        float zebraThreshold = 0.70f;
        bool focusPeakingEnabled = false;
        float focusPeakingThreshold = 0.04f;
        QString focusPeakingColor;
        bool grayscaleEnabled = false;
        bool smpteEnabled = false;
        bool anamorphicDesqueezeEnabled = false;
        float anamorphicDesqueezeRatio = 1.33f;
        bool falseColorEnabled = false;
        int falseColorMode = 0;
        bool lutPreviewEnabled = false;
        QString lutPreviewFile;
        QVariantMap exposureReference;
        int displayRotation = 0;
        float previewAspectRatio = 0.0f;
    };

    PreviewCacheState currentPreviewState(const PreviewFrameInfo &frame, const QSize &size) const;
    bool previewStateChanged(const PreviewCacheState &state) const;

    QPointer<CameraPreviewItem> m_item;
    QPointer<QQuickWindow> m_window;
    QObject *m_bridge = nullptr;

    bool m_initialized = false;

    GLuint m_processProgram = 0;
    GLuint m_passthroughProgram = 0;
    GLuint m_processProgram2D = 0;
    GLuint m_passthroughProgram2D = 0;
    GLuint m_processProgramPlanar = 0;
    GLuint m_passthroughProgramPlanar = 0;
    GLuint m_vbo = 0;
    GLint m_processPosLoc = -1;
    GLint m_processUvLoc = -1;
    GLint m_processTexLoc = -1;
    GLint m_passthroughPosLoc = -1;
    GLint m_passthroughUvLoc = -1;
    GLint m_passthroughTexLoc = -1;
    GLint m_process2DPosLoc = -1;
    GLint m_process2DUvLoc = -1;
    GLint m_process2DTexLoc = -1;
    GLint m_passthrough2DPosLoc = -1;
    GLint m_passthrough2DUvLoc = -1;
    GLint m_passthrough2DTexLoc = -1;
    GLint m_processPlanarPosLoc = -1;
    GLint m_processPlanarUvLoc = -1;
    GLint m_processPlanarYLoc = -1;
    GLint m_processPlanarULoc = -1;
    GLint m_processPlanarVLoc = -1;
    GLint m_passthroughPlanarPosLoc = -1;
    GLint m_passthroughPlanarUvLoc = -1;
    GLint m_passthroughPlanarYLoc = -1;
    GLint m_passthroughPlanarULoc = -1;
    GLint m_passthroughPlanarVLoc = -1;

    // Zebra shader uniforms
    GLint m_uTimeLoc = -1;
    GLint m_uZebraEnabledLoc = -1;
    GLint m_uZebraThresholdLoc = -1;
    GLint m_uTime2DLoc = -1;
    GLint m_uZebraEnabled2DLoc = -1;
    GLint m_uZebraThreshold2DLoc = -1;
    GLint m_uTimePlanarLoc = -1;
    GLint m_uZebraEnabledPlanarLoc = -1;
    GLint m_uZebraThresholdPlanarLoc = -1;

    GLint m_focusPeakingEnabledLoc = -1;
    GLint m_focusPeakingThresholdLoc = -1;
    GLint m_focusPeakingColorLoc = -1;
    GLint m_focusPeakingEnabled2DLoc = -1;
    GLint m_focusPeakingThreshold2DLoc = -1;
    GLint m_focusPeakingColor2DLoc = -1;
    GLint m_focusPeakingEnabledPlanarLoc = -1;
    GLint m_focusPeakingThresholdPlanarLoc = -1;
    GLint m_focusPeakingColorPlanarLoc = -1;

    GLint m_grayscaleEnabledLoc = -1;
    GLint m_smpteEnabledLoc = -1;
    GLint m_grayscaleEnabled2DLoc = -1;
    GLint m_smpteEnabled2DLoc = -1;
    GLint m_grayscaleEnabledPlanarLoc = -1;
    GLint m_smpteEnabledPlanarLoc = -1;

    GLint m_falseColorEnabledLoc = -1;
    GLint m_falseColorModeLoc = -1;
    GLint m_falseColorEnabled2DLoc = -1;
    GLint m_falseColorMode2DLoc = -1;
    GLint m_falseColorEnabledPlanarLoc = -1;
    GLint m_falseColorModePlanarLoc = -1;

    GLint m_lutPreviewEnabledLoc = -1;
    GLint m_lutPreviewTextureLoc = -1;
    GLint m_lutPreviewSizeLoc = -1;
    GLint m_lutPreviewColumnsLoc = -1;
    GLint m_lutPreviewEnabled2DLoc = -1;
    GLint m_lutPreviewTexture2DLoc = -1;
    GLint m_lutPreviewSize2DLoc = -1;
    GLint m_lutPreviewColumns2DLoc = -1;
    GLint m_lutPreviewEnabledPlanarLoc = -1;
    GLint m_lutPreviewTexturePlanarLoc = -1;
    GLint m_lutPreviewSizePlanarLoc = -1;
    GLint m_lutPreviewColumnsPlanarLoc = -1;

    QElapsedTimer m_shaderTimer;

    std::unordered_map<uint64_t, ImportedBuffer> m_importedBuffers;
    GLuint m_currentTexture = 0;
    GLenum m_currentTextureTarget = GL_TEXTURE_EXTERNAL_OES;
    GLuint m_planarTextures[3] = { 0, 0, 0 };
    GLuint m_cpuFallbackTexture = 0;
    GLuint m_previewTexture = 0;
    GLuint m_previewFbo = 0;
    QSize m_previewSize;
    QSize m_cpuFallbackTextureSize;
    QSize m_planarTextureSizes[3];
    std::vector<unsigned char> m_cpuRgbaBuffer;
    GLuint m_lutPreviewTexture = 0;
    GLuint m_neutralLutTexture = 0;
    std::shared_ptr<LutLoadState> m_lutLoadState;
    QString m_lutPreviewTexturePath;
    QString m_lutLoadingPath;
    QString m_lutFailedPath;
    int m_lutPreviewTextureSize = 0;
    int m_lutPreviewTextureColumns = 0;
    bool m_lutPreviewTextureValid = false;
    int m_importedProcId = -1;
    unsigned int m_importedWidth = 0;
    unsigned int m_importedHeight = 0;
    unsigned int m_importedStride = 0;
    unsigned int m_importedCaptureWidth = 0;
    unsigned int m_importedCaptureHeight = 0;
    unsigned int m_loggedImportWidth = 0;
    unsigned int m_loggedImportHeight = 0;
    unsigned int m_loggedImportStride = 0;
    unsigned int m_loggedImportCaptureWidth = 0;
    unsigned int m_loggedImportCaptureHeight = 0;
    unsigned int m_lastImportedSequence = 0;
    bool m_haveImportedSequence = false;
    std::optional<PreviewCacheState> m_previewCacheState;
    std::optional<PreviewFrameInfo> m_lastRenderableFrame;
    bool m_updatePending = false;
    bool m_directWindowRendering = false;
    bool m_forcePlanarFallback = false;
    bool m_forceCpuFallback = false;
    bool m_allowCpuFallback = false;
    bool m_currentTextureIsPlanar = false;
    bool m_resetPreviewState = false;
    bool m_loggedPlanarFallback = false;
    bool m_loggedCpuFallback = false;
    bool m_loggedCpuFallbackFailure = false;
    bool m_loggedPlanarProcessingPath = false;
    bool m_loggedShaderPrograms = false;
    bool m_loggedEglImport = false;
    bool m_loggedRenderUnavailable = false;
};
