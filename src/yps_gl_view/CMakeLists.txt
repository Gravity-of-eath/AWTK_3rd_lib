cmake_minimum_required (VERSION 3.10)
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

file(GLOB SOURCES 
    ./*.c
    src/*.c
    src/*.cpp
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

# 安装头文件
install(
    DIRECTORY include
    DESTINATION ${LIBRARY_NAME}
)
