#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "Horde3D.h"
#include "stdbool.h"
#include "stdint.h"
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

typedef struct _EGLWindowData {
  EGLSurface surface;
  int width;
  int height;
  bool isOffscreen;
} EGLWindowData;

typedef enum _Platform {
  Windows,
  Linux,
  MacOS,
  Android,
  IOS,
  Emscripten
} Platform;

typedef enum _RenderAPI { OpenGL2 = 2, OpenGL4 = 4, OpenGLES3 = 8 } RenderAPI;

typedef struct _FbData {
  uint32_t width;
  uint32_t height;
  uint32_t size;
  uint8_t *data;
} FbData;

static EGLDisplay _eglDisplay = EGL_NO_DISPLAY;
static EGLContext _eglContext = EGL_NO_CONTEXT;
// Platform we are running on
static Platform _curPlatform;

// Used render interface
static RenderAPI _curRenderAPI;

char _name[128];
int32_t _width;
int32_t _height;
bool _isOffscreen;
float _fov;
float _near_plane;
float _far_plane;
EGLSurface surface = EGL_NO_SURFACE;

EGLWindowData *createWindow(char *name, int32_t width, int32_t height, bool off_screen,
                float fov, float near_plane, float far_plane);

bool destroyWindow(EGLWindowData *windowData);

void swapBuffers(EGLWindowData *windowData);

FbData *getFbImage();

void release();