/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2019 NVIDIA Inc.

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef KWIN_EGL_STREAM_BACKEND_H
#define KWIN_EGL_STREAM_BACKEND_H
#include "abstract_egl_drm_backend.h"
#include "basiceglsurfacetexture_wayland.h"
#include <KWaylandServer/surface_interface.h>
#include <KWaylandServer/eglstream_controller_interface.h>
#include <wayland-server-core.h>

namespace KWin
{

class DrmAbstractOutput;
class DrmDumbBuffer;
class DumbSwapchain;
class ShadowBuffer;

/**
 * @brief OpenGL Backend using Egl with an EGLDevice.
 */
class EglStreamBackend : public AbstractEglDrmBackend
{
    Q_OBJECT
public:
    EglStreamBackend(DrmBackend *b, DrmGpu *gpu);
    ~EglStreamBackend() override;
    SurfaceTexture *createSurfaceTextureInternal(SurfacePixmapInternal *pixmap) override;
    SurfaceTexture *createSurfaceTextureWayland(SurfacePixmapWayland *pixmap) override;
    QRegion beginFrame(AbstractOutput *output) override;
    void endFrame(AbstractOutput *output, const QRegion &damage, const QRegion &damagedRegion) override;
    void init() override;

    bool hasOutput(AbstractOutput *output) const override;
    bool addOutput(DrmAbstractOutput *output) override;
    void removeOutput(DrmAbstractOutput *output) override;

    QSharedPointer<DrmBuffer> renderTestFrame(DrmAbstractOutput *output) override;

protected:
    void cleanupSurfaces() override;

private:
    bool initializeEgl();
    bool initBufferConfigs();
    bool initRenderingContext();
    struct StreamTexture
    {
        EGLStreamKHR stream;
        GLuint texture;
    };
    StreamTexture *lookupStreamTexture(KWaylandServer::SurfaceInterface *surface);
    void destroyStreamTexture(KWaylandServer::SurfaceInterface *surface);
    void attachStreamConsumer(KWaylandServer::SurfaceInterface *surface,
                              void *eglStream,
                              wl_array *attribs);
    struct Output
    {
        DrmOutput *output = nullptr;
        QSharedPointer<DrmDumbBuffer> buffer;
        EGLSurface eglSurface = EGL_NO_SURFACE;
        EGLStreamKHR eglStream = EGL_NO_STREAM_KHR;
        QSharedPointer<ShadowBuffer> shadowBuffer;

        // for operation as secondary GPU
        QSharedPointer<DumbSwapchain> dumbSwapchain;
    };
    bool resetOutput(Output &output);
    bool makeContextCurrent(const Output &output);
    void cleanupOutput(Output &output);

    QMap<AbstractOutput *, Output> m_outputs;
    KWaylandServer::EglStreamControllerInterface *m_eglStreamControllerInterface;
    QHash<KWaylandServer::SurfaceInterface *, StreamTexture> m_streamTextures;

    friend class EglStreamSurfaceTextureWayland;
};

class EglStreamSurfaceTextureWayland : public BasicEGLSurfaceTextureWayland
{
public:
    EglStreamSurfaceTextureWayland(EglStreamBackend *backend, SurfacePixmapWayland *pixmap);
    ~EglStreamSurfaceTextureWayland() override;

    bool create() override;
    void update(const QRegion &region) override;

private:
    bool acquireStreamFrame(EGLStreamKHR stream);
    void createFbo();
    void copyExternalTexture(GLuint tex);
    bool attachBuffer(KWaylandServer::ClientBuffer *buffer);
    bool checkBuffer(KWaylandServer::SurfaceInterface *surface,
                     KWaylandServer::ClientBuffer *buffer);

    EglStreamBackend *m_backend;
    GLuint m_fbo, m_rbo, m_textureId;
    GLenum m_format;
};

} // namespace

#endif
