cmake_minimum_required(VERSION 3.10)

set(LIBRARY_NAME yps_gl_view)
set(OGRE_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/${PLATFORM}")

# OGRE binaries are distributed per platform. Do not add this target when
# the matching prebuilt libraries are not available (for example x86 builds
# in this repository, which only contain the aarch64/t507 OGRE binaries).
if(NOT EXISTS "${OGRE_LIB_DIR}/libOgreMain.so")
    message(WARNING
        "yps_gl_view: no OGRE libs for PLATFORM=${PLATFORM} under "
        "${OGRE_LIB_DIR}, skipping target."
    )
    return()
endif()

if(NOT EXISTS "${OGRE_LIB_DIR}/libOgreRTShaderSystem.so")
    message(WARNING
        "yps_gl_view: libOgreRTShaderSystem.so is missing from "
        "${OGRE_LIB_DIR}, skipping target."
    )
    return()
endif()

# Keep the source list explicit. The previous file contained an unterminated
# file(GLOB ...) command and duplicate add_library() calls, which left the
# target with no sources and caused CMake configuration/build failures.
set(SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/yps_gl_view.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/yps_gl_view_register.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ogre_awtk_app.cpp"
)

add_library(${LIBRARY_NAME} ${SOURCES})

target_include_directories(${LIBRARY_NAME}
    PRIVATE
        ${AWTK_INCLUDE}
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Bullet"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Bites"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/MeshLodGenerator"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RTShaderSystem"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Paging"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Plugins"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Property"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Overlay"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/GL"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/GL3Plus"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/glad"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/GLES2"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/GLES2/GLES2"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/GLES2/GLES3"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/GLES2/GLSLES"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/GLES2/KHR"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/KHR"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Terrain"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Volume"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Plugins/DotScene"
)

# Use absolute paths for the bundled OGRE libraries. This avoids accidental
# resolution of host libraries and works consistently with cross toolchains.
target_link_libraries(${LIBRARY_NAME}
    PRIVATE
        ${AWTK_SO}
        "${OGRE_LIB_DIR}/libOgreMain.so"
        "${OGRE_LIB_DIR}/libOgreRTShaderSystem.so"
        EGL
        GLESv2
)

# The installed OGRE libraries are placed beside yps_gl_view.so, so $ORIGIN
# is sufficient at runtime. Keep the source-tree path for build-tree tests.
set_target_properties(${LIBRARY_NAME} PROPERTIES
    BUILD_RPATH "$ORIGIN:${OGRE_LIB_DIR}"
    INSTALL_RPATH "$ORIGIN"
)

install(
    TARGETS ${LIBRARY_NAME}
    LIBRARY DESTINATION ${LIBRARY_NAME}
)

install(
    DIRECTORY "${OGRE_LIB_DIR}/"
    DESTINATION ${LIBRARY_NAME}
    FILES_MATCHING PATTERN "*.so*"
)
