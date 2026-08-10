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

#include "OgreGLUtil.h"
#include "OgreT507EGLSupport.h"
#include "OgreT507EGLWindow.h"
#include "fbinfo.h"
#include <fcntl.h>
#include <iostream>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace Ogre
{

GLNativeSupport* getGLSupport(int profile) { return new T507EGLSupport(profile); }

T507EGLSupport::T507EGLSupport(int profile) : EGLSupport(profile)
{
    // mNativeDisplay = EGL_DEFAULT_DISPLAY;
    // mGLDisplay = eglGetDisplay(mNativeDisplay);
    mGLDisplay = EGLSupport::getGLDisplay();
    mNativeDisplay == EGL_DEFAULT_DISPLAY;
    printf("*****************************T507EGLSupport %d***********************************\n",__LINE__);
}

ConfigOptionMap T507EGLSupport::getConfigOptions()
{
    ConfigOptionMap mOptions = EGLSupport::getConfigOptions();
    // ConfigOption optOrientation;
    // optOrientation.name = "ScreenInfo";
    // optOrientation.immutable = false;
    // optOrientation.possibleValues.push_back("Landscape");
    // optOrientation.possibleValues.push_back("Portrait");
    // optOrientation.currentValue = optOrientation.possibleValues[0];
    // mOptions[optOrientation.name] = optOrientation;

    // ConfigOption optScaling;
    // optScaling.name = "Content Scaling Factor";
    // optScaling.immutable = false;
    // optScaling.possibleValues.push_back("1");
    // optScaling.currentValue = optScaling.possibleValues[0];
    // mOptions[optScaling.name] = optScaling;

    return mOptions;
}

T507EGLSupport::~T507EGLSupport() {}


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

RenderWindow* T507EGLSupport::newWindow(const String& name, unsigned int width, unsigned int height, bool fullScreen,
                                        const NameValuePairList* miscParams)
{
    printf("*****************************T507EGLSupport %d***********************************\n",__LINE__);
    T507EGLWindow* window = new T507EGLWindow(this);
     NameValuePairList params;
    if (miscParams)
        params = *miscParams;
    
    params["title"] = name;
    params["width"] = StringConverter::toString(width);
    params["height"] = StringConverter::toString(height);
    params["fullScreen"] = StringConverter::toString(fullScreen);
    window->create(name, width, height, fullScreen, miscParams);

    // EGLWindow* window = new EGLWindow(this);

    // int cache_width = 0;
    // int cache_height = 0;
    // fb_info("/dev/fb0", &cache_width, &cache_height);
    // window->create(name, cache_width, cache_height, fullScreen, miscParams);

    return window;
}
} // namespace Ogre
