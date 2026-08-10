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

#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgreRoot.h"
#include "OgreStringConverter.h"

#include "OgreGLRenderSystemCommon.h"

#include "OgreT507EGLSupport.h"
#include "OgreT507EGLWindow.h"
#include "OgreViewport.h"
// #include "fbinfo.h"

#include <algorithm>
#include <climits>
#include <fcntl.h>
#include <iostream>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <unistd.h>

struct shadow_fbdev_window
{
    unsigned short width;
    unsigned short height;
};

static int fb_info(const char* filename, int* width, int* height)
{
    int fd = -1;
    struct fb_var_screeninfo vinfo;

    memset(&vinfo, 0, sizeof(vinfo));
    fd = open(filename, O_RDWR);
    if (fd < 0)
    {
        printf("open: %s failed\n", filename);
        return -1;
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) < 0)
    {
        printf("fbioget err\n");
        return -2;
    }

    *width = vinfo.xres;
    *height = vinfo.yres;

    close(fd);
    return 0;
}

static EGLNativeWindowType createNativeWindow(unsigned short width, unsigned short height)
{
    struct shadow_fbdev_window* fbwin = (struct shadow_fbdev_window*)malloc(sizeof(struct shadow_fbdev_window));
    if (fbwin == NULL)
    {
        return 0;
    }
    fbwin->width = width;
    fbwin->height = height;
    return (EGLNativeWindowType)fbwin;
}

namespace Ogre
{
T507EGLWindow::T507EGLWindow(T507EGLSupport* glsupport) : EGLWindow(glsupport)
{
    mContext = nullptr;
    mEglDisplay = EGL_NO_DISPLAY;
    mEglSurface = EGL_NO_SURFACE;
    mEglConfig = nullptr;
    mIsExternal = false;

    // 初始化离屏渲染状态
    mOffscreenEnabled = false;
    mOffscreenUseFboTexture = false;
    mOffscreenExternal = false;
    mOffscreenFbo = 0;
    mOffscreenColorTex = 0;
    mOffscreenDepthStencilRb = 0;
    mOffscreenWidth = 0;
    mOffscreenHeight = 0;
    mOffscreenColorFormat = GL_RGBA;
    mOwnColorTex = false;
    mOwnFbo = false;
    mOwnDepthRb = false;
    mFrameSerial = 0;
}

void T507EGLWindow::destroy()
{
    if (mClosed)
        return;

    mClosed = true;
    mActive = false;
    printf("*****************************T507EGLWindow %d***********************************\n", __LINE__);

    destroyOffscreenTarget();

    if (mIsExternalGLControl)
    {
        // currentGLContext 模式：EGL display/surface/context 由宿主拥有，不接管销毁
        if (mContext)
        {
            delete mContext;
            mContext = nullptr;
        }
        printf("*****************************T507EGLWindow %d (external) ********\n", __LINE__);
        return;
    }

    if (mContext)
    {
        printf("*****************************T507EGLWindow %d***********************************\n", __LINE__);
        eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, mContext);
        eglDestroyContext(mEglDisplay, mContext);
        eglDestroySurface(mEglDisplay, mEglSurface);
        eglTerminate(mEglDisplay);

        free((void*)mContext);
        delete mContext;
        mContext = nullptr;
    }
    printf("*****************************T507EGLWindow %d***********************************\n", __LINE__);
}

void T507EGLWindow::swapBuffers()
{
    // printf("T507EGLWindow::swapBuffers\n");
    if (mContext) mContext->setCurrent();

    if (mOffscreenUseFboTexture)
    {
        // FBO 模式下不需要真正的 eglSwapBuffers，仅完成当前帧并准备下一帧
        endOffscreenFrame();
        beginOffscreenFrame();
        return;
    }

    if (mIsExternalGLControl)
    {
        // currentGLContext 模式：宿主拥有 surface，不接管 swap
        return;
    }

    if (eglSwapBuffers(mEglDisplay, mEglSurface) == EGL_FALSE)
    {
        EGL_CHECK_ERROR
        OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR, "Fail to SwapBuffers");
    }
}

void T507EGLWindow::create(const String& name, uint width, uint height, bool fullScreen,
                           const NameValuePairList* miscParams)
{
    printf("*****************************T507EGLWindow %d***********************************\n", __LINE__);
    mName = name;
    mOffscreenEnabled = false;

    // currentGLContext: 复用调用方已经准备好的 EGL display/surface/context，
    // 不再 eglInitialize / eglCreateContext / eglMakeCurrent。
    // 这样 OGRE 与宿主（如 AWTK）共享同一个 GL 名称空间，FBO/VAO 也直接通用。
    if (miscParams)
    {
        auto opt = miscParams->find("currentGLContext");
        if (opt != miscParams->end() && StringConverter::parseBool(opt->second))
        {
            ::EGLContext eglctx = eglGetCurrentContext();
            if (!eglctx)
            {
                OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                            "currentGLContext was specified with no current GL context",
                            "T507EGLWindow::create");
            }
            mEglDisplay  = eglGetCurrentDisplay();
            mEglSurface  = eglGetCurrentSurface(EGL_DRAW);
            mEglConfig   = nullptr;
            mIsExternalGLControl = true;
            mWidth  = width;
            mHeight = height;

            mContext = createEGLContext(eglctx);
            mContext->setCurrent();

            // offScreen 仍按需建立离屏 FBO/纹理（或外部 FBO）
            auto opt2 = miscParams->find("offScreen");
            if (opt2 != miscParams->end() && StringConverter::parseBool(opt2->second))
            {
                mOffscreenEnabled = true;
                initOffscreenTarget(miscParams, width, height);
                if (mOffscreenUseFboTexture)
                {
                    beginOffscreenFrame();
                }
            }

            mActive    = true;
            mClosed    = false;
            mAutoUpdate = true;
            printf("T507EGLWindow: using currentGLContext, w=%u h=%u\n", width, height);
            return;
        }
    }

    int cache_width = 0;
    int cache_height = 0;
    egl_devices_fb_context_t* ctx = &fb_context;

    ctx->eglNativeDisplayType = (EGLNativeDisplayType)0;
    ctx->egldisplay = eglGetDisplay(ctx->eglNativeDisplayType);
    eglInitialize(ctx->egldisplay, NULL, NULL);
    mIsExternalGLControl = false;
    mEglDisplay = ctx->egldisplay;
    assert(eglGetError() == EGL_SUCCESS);
    eglBindAPI(EGL_OPENGL_ES_API);

    EGLint s_configAttribs[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_RED_SIZE,
                                8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
                                // EGL_STENCIL_SIZE,
                                // 8,
                                EGL_NONE};

    eglChooseConfig(ctx->egldisplay, s_configAttribs, &(ctx->eglconfig), 1, &(ctx->numconfigs));

    mEglConfig = ctx->eglconfig;
    assert(eglGetError() == EGL_SUCCESS);
    // assert(ctx->numconfigs == 1);
    fb_info("/dev/fb0", &cache_width, &cache_height);
    mWidth = cache_width;
    mHeight = cache_height;

    ctx->eglNativeWindow = (EGLNativeWindowType)createNativeWindow(mWidth, mHeight);

    // assert(eglGetError() == EGL_SUCCESS);
    EGLint ContextAttribList[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    if (miscParams)
    {
        NameValuePairList::const_iterator opt;
        NameValuePairList::const_iterator end = miscParams->end();
        if ((opt = miscParams->find("sharedGLContext")) != end && StringConverter::parseBool(opt->second))
        {
            ctx->eglcontext = eglGetCurrentContext();
            if (ctx->eglcontext)
            {
                ctx->eglcontext = eglCreateContext(ctx->egldisplay, ctx->eglconfig, eglGetCurrentContext(), ContextAttribList);
                printf("shared eglcontext\n");
            }
            else
            {
                ctx->eglcontext = eglCreateContext(ctx->egldisplay, ctx->eglconfig, 0, ContextAttribList);
                printf("shared eglcontext fail,main GlContext is null!,create noshared context\n");
            }
        }
        else
        {
            ctx->eglcontext = eglCreateContext(ctx->egldisplay, ctx->eglconfig, 0, ContextAttribList);
        }

        if ((opt = miscParams->find("offScreen")) != end && StringConverter::parseBool(opt->second))
        {
            mOffscreenEnabled = true;
            EGLint surfaceAttribs[] = {EGL_WIDTH, static_cast<EGLint>(width), EGL_HEIGHT, static_cast<EGLint>(height),
                                       EGL_NONE};
            ctx->eglsurface = eglCreatePbufferSurface(ctx->egldisplay, ctx->eglconfig, surfaceAttribs);
            printf("offScreen render\n");
        }
        else
        {
            ctx->eglsurface = eglCreateWindowSurface(ctx->egldisplay, ctx->eglconfig, ctx->eglNativeWindow, NULL);
        }
    }
    else
    {
        ctx->eglcontext = eglCreateContext(ctx->egldisplay, ctx->eglconfig, 0, ContextAttribList);
        ctx->eglsurface = eglCreateWindowSurface(ctx->egldisplay, ctx->eglconfig, ctx->eglNativeWindow, NULL);
    }

    mEglSurface = ctx->eglsurface;
    mContext = createEGLContext(ctx->eglcontext);
    mContext->setCurrent();

    // assert(eglGetError() == EGL_SUCCESS);
    eglMakeCurrent(ctx->egldisplay, ctx->eglsurface, ctx->eglsurface, ctx->eglcontext);

    // 初始化离屏渲染目标 (V1)
    if (mOffscreenEnabled && miscParams)
    {
        initOffscreenTarget(miscParams, width, height);
        if (mOffscreenUseFboTexture)
        {
            beginOffscreenFrame();
        }
    }
    // assert(eglGetError() == EGL_SUCCESS);
    glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // glDisable(GL_DEPTH_TEST);
    // glDisable(GL_SCISSOR_TEST);
    eglSwapInterval(ctx->egldisplay, 1);
    eglSwapBuffers(mEglDisplay, mEglSurface);

    printf("*****************************T507EGLWindow %d***********************************\n", __LINE__);
    mActive = true;
    mClosed = false;
    mAutoUpdate = true;
    EGLWindow::setVSyncEnabled(true);
}

void T507EGLWindow::initOffscreenTarget(const NameValuePairList* miscParams, uint width, uint height)
{
    NameValuePairList::const_iterator opt;
    NameValuePairList::const_iterator end = miscParams->end();

    // 解析参数
    String target = "pbuffer";
    if ((opt = miscParams->find("offScreenTarget")) != end) {
        target = opt->second;
    }

    if (target == "fboTexture") {
        mOffscreenUseFboTexture = true;
        mOffscreenWidth = width;
        mOffscreenHeight = height;

        // 检查是否使用外部 FBO
        bool wantExternal = false;
        if ((opt = miscParams->find("fboExternal")) != end) {
            wantExternal = StringConverter::parseBool(opt->second);
        }

        if (wantExternal) {
            // 外部 FBO 模式：可在 miscParams 直接传入 externalFboId / externalColorTexId 一步到位，
            // 也可以保持空，等待后续调用 setExternalFbo() 传入。
            mOffscreenExternal = true;
            mOffscreenFbo = 0;
            mOffscreenColorTex = 0;
            mOwnFbo = false;
            mOwnColorTex = false;

            if ((opt = miscParams->find("externalFboId")) != end) {
                mOffscreenFbo = (GLuint)StringConverter::parseUnsignedInt(opt->second);
            }
            if ((opt = miscParams->find("externalColorTexId")) != end) {
                mOffscreenColorTex = (GLuint)StringConverter::parseUnsignedInt(opt->second);
            }
            if ((opt = miscParams->find("externalFboWidth")) != end) {
                mOffscreenWidth = StringConverter::parseUnsignedInt(opt->second);
            }
            if ((opt = miscParams->find("externalFboHeight")) != end) {
                mOffscreenHeight = StringConverter::parseUnsignedInt(opt->second);
            }

            printf("Offscreen FBO external mode: fbo=%u tex=%u size=%ux%u\n",
                   mOffscreenFbo, mOffscreenColorTex, mOffscreenWidth, mOffscreenHeight);
        } else {
            // 内部创建模式（V1 原有逻辑）
            mOffscreenExternal = false;

            // 创建纹理
            glGenTextures(1, &mOffscreenColorTex);
            glBindTexture(GL_TEXTURE_2D, mOffscreenColorTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mOffscreenWidth, mOffscreenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            mOwnColorTex = true;

            // 创建 FBO
            glGenFramebuffers(1, &mOffscreenFbo);
            glBindFramebuffer(GL_FRAMEBUFFER, mOffscreenFbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mOffscreenColorTex, 0);
            mOwnFbo = true;

            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE)
            {
                printf("Error: Offscreen FBO is incomplete (0x%x). Falling back to PBuffer.\n", status);
                destroyOffscreenTarget();
                mOffscreenUseFboTexture = false;
            }
            else
            {
                printf("Offscreen FBO created successfully: TexId=%u, Size=%ux%u\n", mOffscreenColorTex, mOffscreenWidth,
                       mOffscreenHeight);
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }
}

void T507EGLWindow::destroyOffscreenTarget()
{
    if (mOwnFbo && mOffscreenFbo) {
        glDeleteFramebuffers(1, &mOffscreenFbo);
    }
    if (mOwnColorTex && mOffscreenColorTex) {
        glDeleteTextures(1, &mOffscreenColorTex);
    }
    if (mOwnDepthRb && mOffscreenDepthStencilRb) {
        glDeleteRenderbuffers(1, &mOffscreenDepthStencilRb);
    }

    mOffscreenFbo = 0;
    mOffscreenColorTex = 0;
    mOffscreenDepthStencilRb = 0;
    mOwnFbo = mOwnColorTex = mOwnDepthRb = false;
    mOffscreenUseFboTexture = false;
    mOffscreenEnabled = false;
}

void T507EGLWindow::beginOffscreenFrame()
{
    if (mOffscreenUseFboTexture && mOffscreenFbo) {
        glBindFramebuffer(GL_FRAMEBUFFER, mOffscreenFbo);
        glViewport(0, 0, mOffscreenWidth, mOffscreenHeight);
    }
}

void T507EGLWindow::setExternalFbo(GLuint fboId, uint32 width, uint32 height)
{
    // 切换到外部 FBO：先释放可能存在的内部资源，避免泄漏。
    if (mOwnFbo && mOffscreenFbo) {
        glDeleteFramebuffers(1, &mOffscreenFbo);
    }
    if (mOwnColorTex && mOffscreenColorTex) {
        glDeleteTextures(1, &mOffscreenColorTex);
    }
    if (mOwnDepthRb && mOffscreenDepthStencilRb) {
        glDeleteRenderbuffers(1, &mOffscreenDepthStencilRb);
    }
    mOwnFbo = false;
    mOwnColorTex = false;
    mOwnDepthRb = false;

    mOffscreenExternal      = true;
    mOffscreenUseFboTexture = (fboId != 0);
    mOffscreenFbo           = fboId;
    mOffscreenColorTex      = 0;   // 颜色纹理由外部拥有，本类不再管理
    mOffscreenWidth         = width;
    mOffscreenHeight        = height;
    mOffscreenEnabled       = (fboId != 0);

    if (fboId != 0) {
        beginOffscreenFrame();
    }
    printf("setExternalFbo: fbo=%u size=%ux%u\n", fboId, width, height);
}

void T507EGLWindow::endOffscreenFrame()
{
    if (mOffscreenUseFboTexture && mOffscreenFbo)
    {
        glFlush(); // V1 同步策略
        mFrameSerial++;
    }
}

bool T507EGLWindow::getLatestOffscreenFrameInfo(OffscreenFrameInfo& out) const
{
    if (!mOffscreenUseFboTexture || !mOffscreenColorTex) {
        return false;
    }
    out.width = mOffscreenWidth;
    out.height = mOffscreenHeight;
    out.glColorTexId = mOffscreenColorTex;
    out.frameSerial = mFrameSerial;
    return true;
}

void T507EGLWindow::getCustomAttribute(const String& name, void* pData)
{
    if (name == "OffscreenFrameInfo")
    {
        OffscreenFrameInfo* out = static_cast<OffscreenFrameInfo*>(pData);
        getLatestOffscreenFrameInfo(*out);
        return;
    }
    else if (name == "OffscreenColorTexId")
    {
        *static_cast<uint32*>(pData) = mOffscreenColorTex;
        return;
    }
    else if (name == "OffscreenFrameSerial")
    {
        *static_cast<uint64*>(pData) = mFrameSerial;
        return;
    }
    else if (name == "GLFBO")
    {
        // GLES2FBOManager::bind() 在窗口目标上查询 GLFBO 决定要绑定的 FBO 句柄。
        // 离屏 FBO 模式下返回 mOffscreenFbo，确保渲染真正进入离屏纹理；
        // 否则返回 0（默认 framebuffer）。
        *static_cast<GLuint*>(pData) = (mOffscreenUseFboTexture ? mOffscreenFbo : 0);
        return;
    }

    EGLWindow::getCustomAttribute(name, pData);
}

} // namespace Ogre
