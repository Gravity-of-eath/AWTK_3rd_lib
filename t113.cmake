# 基础定义
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 工具链路径
set(TOOLCHAIN_DIR "/develop/toolchain_t113_musl")
# set(CMAKE_SYSROOT "${TOOLCHAIN_DIR}/aarch64-buildroot-linux-gnu/sysroot")

# 编译器设置
set(CMAKE_C_COMPILER "${TOOLCHAIN_DIR}/bin/arm-openwrt-linux-muslgnueabi-gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_DIR}/bin/arm-openwrt-linux-muslgnueabi-g++")

# 编译标志
# T113 是双核 Cortex-A7(ARMv7-A)；readelf -A lib/t113/libawtk.so 确认为
# Tag_CPU_name: Cortex-A7 / Tag_ABI_VFP_args: VFP registers(硬浮点)。
# 这里只指定 -mcpu 做指令选择与调度，不动 -mfloat-abi/-mfpu，保持与
# 预编译 libawtk.so 相同的 ABI。
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-a7")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-a7")
set(BUILD_SHARED_LIBS ON)

# 库搜索路径
set(CMAKE_FIND_ROOT_PATH "${TOOLCHAIN_DIR}/sysroot")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

