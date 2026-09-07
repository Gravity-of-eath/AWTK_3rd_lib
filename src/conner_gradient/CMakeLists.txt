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
# if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    # target_compile_definitions(${LIBRARY_NAME} PRIVATE __ARM_NEON__)
    # target_compile_options(${LIBRARY_NAME} PRIVATE -mfpu=neon)
# endif()

target_link_libraries(${LIBRARY_NAME} ${AWTK_SO})

# 帧间缓存的运行时验证程序（仅 x86 桌面）
if(PLATFORM STREQUAL "x86")
  add_executable(conner_gradient_cache_test tests/conner_gradient_cache_test.c)
  target_include_directories(conner_gradient_cache_test PRIVATE ${AWTK_INCLUDE}/ext_widgets)
  target_link_libraries(conner_gradient_cache_test ${LIBRARY_NAME} ${AWTK_SO} m dl pthread)
  install(TARGETS conner_gradient_cache_test RUNTIME DESTINATION ${LIBRARY_NAME})

  # 把渲染结果导出为 PNG，供肉眼检查
  add_executable(conner_gradient_shot tests/conner_gradient_shot.c)
  target_include_directories(conner_gradient_shot PRIVATE ${AWTK_INCLUDE}/ext_widgets)
  target_link_libraries(conner_gradient_shot ${LIBRARY_NAME} ${AWTK_SO} z m dl pthread)
  install(TARGETS conner_gradient_shot RUNTIME DESTINATION ${LIBRARY_NAME})

  # 桌面动态 demo
  add_executable(conner_gradient_demo tests/conner_gradient_demo.c)
  target_include_directories(conner_gradient_demo PRIVATE ${AWTK_INCLUDE}/ext_widgets)
  target_link_libraries(conner_gradient_demo ${LIBRARY_NAME} ${AWTK_SO} m dl pthread)
  install(TARGETS conner_gradient_demo RUNTIME DESTINATION ${LIBRARY_NAME})
endif()

install(
    TARGETS ${LIBRARY_NAME}
    LIBRARY DESTINATION ${LIBRARY_NAME}  # 动态库 (.so)\
)

# 安装头文件
install(
    DIRECTORY include
    DESTINATION ${LIBRARY_NAME}
)
