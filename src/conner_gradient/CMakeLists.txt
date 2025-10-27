cmake_minimum_required (VERSION 3.10)
include_directories(${AWTK_INCLUDE})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(LIBRARY_NAME conner_gradient)

file(GLOB SOURCES 
    ./*.c
    src/*.c
)

add_library(${LIBRARY_NAME} ${SOURCES})

# 为T113平台添加NEON支持
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    target_compile_definitions(${LIBRARY_NAME} PRIVATE __ARM_NEON__)
    target_compile_options(${LIBRARY_NAME} PRIVATE -mfpu=neon)
endif()

target_link_libraries(${LIBRARY_NAME} ${AWTK_SO})

install(
    TARGETS ${LIBRARY_NAME}
    LIBRARY DESTINATION ${LIBRARY_NAME}  # 动态库 (.so)\
)

# 安装头文件
install(
    DIRECTORY include
    DESTINATION ${LIBRARY_NAME}
)
