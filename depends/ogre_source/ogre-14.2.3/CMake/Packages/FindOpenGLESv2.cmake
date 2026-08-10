# - Try to find OpenGLES and EGL
# Once done this will define
#  
#  OPENGLES2_FOUND        - system has OpenGLES
#  OPENGLES2_INCLUDE_DIR  - the GL include directory
#  OPENGLES2_LIBRARIES    - Link these to use OpenGLES
#
#  EGL_FOUND        - system has EGL
#  EGL_INCLUDE_DIR  - the EGL include directory
#  EGL_LIBRARIES    - Link these to use EGL


SET(OPENGLES2_INCLUDE_DIR "/opt/t5sdk/aarch64-buildroot-linux-gnu/sysroot/usr/include")
SET(OPENGLES2_gl_LIBRARY "/opt/t5sdk/aarch64-buildroot-linux-gnu/sysroot/usr/lib/libGLESv2.so")

SET(EGL_INCLUDE_DIR "/opt/t5sdk/aarch64-buildroot-linux-gnu/sysroot/usr/include")
SET(EGL_egl_LIBRARY "/opt/t5sdk/aarch64-buildroot-linux-gnu/sysroot/usr/lib/libEGL.so")

SET( OPENGLES2_FOUND "YES" )


MARK_AS_ADVANCED(
  OPENGLES2_INCLUDE_DIR
  OPENGLES2_gl_LIBRARY
  EGL_INCLUDE_DIR
  EGL_egl_LIBRARY
)

    message("Find GLES 2 OPENGLES2_INCLUDE_DIR= " ${OPENGLES2_INCLUDE_DIR})
