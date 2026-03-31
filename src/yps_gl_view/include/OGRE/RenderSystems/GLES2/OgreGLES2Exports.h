
#ifndef _OgreGLES2Export_H
#define _OgreGLES2Export_H

#ifdef RENDERSYSTEM_GLES2_STATIC_DEFINE
#  define _OgreGLES2Export
#  define RENDERSYSTEM_GLES2_NO_EXPORT
#else
#  ifndef _OgreGLES2Export
#    ifdef RenderSystem_GLES2_EXPORTS
        /* We are building this library */
#      define _OgreGLES2Export __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define _OgreGLES2Export __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef RENDERSYSTEM_GLES2_NO_EXPORT
#    define RENDERSYSTEM_GLES2_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef RENDERSYSTEM_GLES2_DEPRECATED
#  define RENDERSYSTEM_GLES2_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef RENDERSYSTEM_GLES2_DEPRECATED_EXPORT
#  define RENDERSYSTEM_GLES2_DEPRECATED_EXPORT _OgreGLES2Export RENDERSYSTEM_GLES2_DEPRECATED
#endif

#ifndef RENDERSYSTEM_GLES2_DEPRECATED_NO_EXPORT
#  define RENDERSYSTEM_GLES2_DEPRECATED_NO_EXPORT RENDERSYSTEM_GLES2_NO_EXPORT RENDERSYSTEM_GLES2_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef RENDERSYSTEM_GLES2_NO_DEPRECATED
#    define RENDERSYSTEM_GLES2_NO_DEPRECATED
#  endif
#endif

#endif /* _OgreGLES2Export_H */
