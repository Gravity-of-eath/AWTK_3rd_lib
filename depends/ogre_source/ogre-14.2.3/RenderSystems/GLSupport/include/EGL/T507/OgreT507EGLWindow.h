/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#ifndef __T507EGLWindow_H__
#define __T507EGLWindow_H__
#include "OgreEGLWindow.h"
#include "OgreT507EGLSupport.h"
#include "fbinfo.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#ifndef EGL_COVERAGE_BUFFERS_NV
#define EGL_COVERAGE_BUFFERS_NV 0x30E0
#endif

#ifndef EGL_COVERAGE_SAMPLES_NV
#define EGL_COVERAGE_SAMPLES_NV 0x30E1
#endif

// typedef struct _egl_devices_fb_context_t
// {
//     EGLint numconfigs;
//     EGLDisplay egldisplay;
//     EGLConfig eglconfig;
//     EGLSurface eglsurface;
//     EGLContext eglcontext;
//     EGLNativeWindowType eglNativeWindow;
//     EGLNativeDisplayType eglNativeDisplayType;
// } egl_devices_fb_context_t;
class egl_devices_fb_context_t
{
public:
    EGLint numconfigs;
    EGLDisplay egldisplay;
    EGLConfig eglconfig;
    EGLSurface eglsurface;
    EGLContext eglcontext;
    EGLNativeWindowType eglNativeWindow;
    EGLNativeDisplayType eglNativeDisplayType;
};

namespace Ogre
{
struct OffscreenFrameInfo {
    uint32 width;
    uint32 height;
    uint32 glColorTexId;
    uint64 frameSerial;
};

class _OgreExport T507EGLWindow : public EGLWindow
{
private:
    egl_devices_fb_context_t fb_context;

    // 离屏渲染相关状态
    bool mOffscreenEnabled;
    bool mOffscreenUseFboTexture;
    bool mOffscreenExternal;
    GLuint mOffscreenFbo;
    GLuint mOffscreenColorTex;
    GLuint mOffscreenDepthStencilRb;
    uint32 mOffscreenWidth, mOffscreenHeight;
    GLenum mOffscreenColorFormat;
    bool mOwnColorTex, mOwnFbo, mOwnDepthRb;
    uint64 mFrameSerial;

    // 内部初始化与销毁方法
    void initOffscreenTarget(const NameValuePairList* miscParams, uint width, uint height);
    void destroyOffscreenTarget();

public:
    T507EGLWindow(T507EGLSupport* glsupport);
    void destroy();
    void create(const String& name, unsigned int width, unsigned int height, bool fullScreen,
                const NameValuePairList* miscParams) override;
    void swapBuffers() override;

    // 离屏渲染拦截与查询接口
    void beginOffscreenFrame();
    void endOffscreenFrame();
    bool getLatestOffscreenFrameInfo(OffscreenFrameInfo& out) const;
    void getCustomAttribute(const String& name, void* pData) override;

    /**
     * 设置外部 FBO 作为渲染目标（fboExternal 模式）。
     * 调用后 OGRE 将直接渲染到该 FBO，不再使用内部创建的 FBO/纹理。
     * 已有的内部 FBO 资源会被释放。
     * @param fboId  外部 FBO 句柄（如 AWTK 的 offline_fbo）
     * @param width  FBO 宽度
     * @param height FBO 高度
     */
    void setExternalFbo(GLuint fboId, uint32 width, uint32 height);
};
} // namespace Ogre

#endif
