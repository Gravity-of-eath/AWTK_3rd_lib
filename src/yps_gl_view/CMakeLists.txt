cmake_minimum_required (VERSION 3.10)

set(LIBRARY_NAME yps_gl_view)

# OGRE 二进制按 PLATFORM 分发；当前 libs/ 下只提供 aarch64 (t507) 版本。
# 没有对应平台库时优雅跳过，避免 ld -lOgre* 报 "incompatible / not found"。
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libs/${PLATFORM}/libOgreMain.so")
    message(WARNING "yps_gl_view: no OGRE libs for PLATFORM=${PLATFORM} under libs/${PLATFORM}/, skipping target. To enable on this platform, populate libs/${PLATFORM}/ with the matching OGRE 14.x .so files.")
    return()
endif()

include_directories(${AWTK_INCLUDE})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Bullet")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Bites")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/MeshLodGenerator")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RTShaderSystem")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Paging")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Plugins")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Property")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Overlay")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/GL")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/GL3Plus")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/glad")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/GLES2")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/GLES2/GLES2")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/GLES2/GLES3")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/GLES2/GLSLES")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/GLES2/KHR")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/KHR")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Terrain")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Volume")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Plugins/DotScene")

# 使用绝对路径，避免依赖 cmake 处理时的 CWD；libs 按 PLATFORM 分目录
link_directories("${CMAKE_CURRENT_SOURCE_DIR}/libs/${PLATFORM}")

file(GLOB SOURCES
include_directories(${AWTK_INCLUDE})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE") 
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Bullet") 
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Bites") 
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/MeshLodGenerator")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RTShaderSystem")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Paging")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Plugins")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Property")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Overlay")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/GL")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/GL3Plus")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/glad")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/GLES2")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/GLES2/GLES2")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/GLES2/GLES3")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/GLES2/GLSLES")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/GLES2/KHR")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/GLES2")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/RenderSystems/KHR")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Terrain")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Volume")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OGRE/Plugins/DotScene")

set(LIBRARY_NAME yps_gl_view)

add_library(${LIBRARY_NAME} ${SOURCES})

target_link_libraries(${LIBRARY_NAME}
    ${AWTK_SO}
    OgreMain
    OgreRTShaderSystem
    EGL
    GLESv2
)

# 让产出的 .so 在运行时能找到同目录下的 OGRE so（按安装布局：所有 .so 放在同一个目录）
set_target_properties(${LIBRARY_NAME} PROPERTIES
    BUILD_RPATH   "$ORIGIN:${CMAKE_CURRENT_SOURCE_DIR}/libs/${PLATFORM}"
    INSTALL_RPATH "$ORIGIN"
)

# 同时安装 OGRE 运行时库到 widget 安装目录，方便运行时按 $ORIGIN 解析
install(
    DIRECTORY libs/${PLATFORM}/
    DESTINATION ${LIBRARY_NAME}
    FILES_MATCHING PATTERN "*.so*"
)

link_directories(./libs)

add_library(${LIBRARY_NAME} ${SOURCES})
target_link_libraries(${LIBRARY_NAME}
    ${AWTK_SO}
    OgreMain
    OgreRTShaderSystem
)

install(
    TARGETS ${LIBRARY_NAME}
    LIBRARY DESTINATION ${LIBRARY_NAME}  # 动态库 (.so)\
)

