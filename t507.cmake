# 基础定义
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 工具链路径
set(TOOLCHAIN_DIR "/opt/t5sdk")
set(CMAKE_SYSROOT "${TOOLCHAIN_DIR}/aarch64-buildroot-linux-gnu/sysroot")

# 编译器设置
set(CMAKE_C_COMPILER "${TOOLCHAIN_DIR}/bin/aarch64-linux-gnu-gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_DIR}/bin/aarch64-linux-gnu-g++")

# 编译标志
#set(CMAKE_C_FLAGS "-mcpu=cortex-a53 -mfloat-abi=hard")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
set(BUILD_SHARED_LIBS ON)
set(OGRE_STATIC OFF) 

# 库搜索路径
set(CMAKE_FIND_ROOT_PATH "${TOOLCHAIN_DIR}/sysroot")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# T507专用库路径
set(GL_INCLUDE_DIR "${CMAKE_SYSROOT}/usr/include")
set(GLES2_LIBRARIES "${CMAKE_SYSROOT}/usr/lib/libGLESv2.so")
set(EGL_LIBRARIES "${CMAKE_SYSROOT}/usr/lib/libEGL.so")
set(PATH "${CMAKE_SYSROOT}/usr/lib/libEGL.so")
