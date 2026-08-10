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


SET(SDL2_INCLUDE_DIRS "${SDL2_PATH}/include/SDL2")
SET(SDL2_LIBRARIES "${SDL2_PATH}/libSDL2.so")

SET( SDL2_FOUND "YES" )


MARK_AS_ADVANCED(
  SDL2_INCLUDE_DIRS
  SDL2_LIBRARIES
)

message("Find SDL2_LIBRARIES 2 = " ${SDL2_INCLUDE_DIRS})
