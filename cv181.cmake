# 基础定义
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 工具链路径
set(TOOLCHAIN_DIR "/develop/toolchain_CV181H/gcc/gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf")
set(CMAKE_SYSROOT "/develop/toolchain_CV181H/gcc/sysroot/sysroot-glibc-linaro-2.23-2017.05-arm-linux-gnueabihf/")

# 编译器设置
set(CMAKE_C_COMPILER "${TOOLCHAIN_DIR}/bin/arm-linux-gnueabihf-gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_DIR}/bin/arm-linux-gnueabihf-g++")

# 编译标志
#set(CMAKE_C_FLAGS "-mcpu=cortex-a53 -mfloat-abi=hard")
# set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
set(BUILD_SHARED_LIBS ON)

# # 库搜索路径
# set(CMAKE_FIND_ROOT_PATH "${TOOLCHAIN_DIR}/sysroot")
# set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
# set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

