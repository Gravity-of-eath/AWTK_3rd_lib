cmake_minimum_required (VERSION 3.10)
include_directories(${AWTK_INCLUDE})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(LIBRARY_NAME recycle_view)

file(GLOB SOURCES
    ./*.c
    src/*.c
)

add_library(${LIBRARY_NAME} ${SOURCES})
target_link_libraries(${LIBRARY_NAME} ${AWTK_SO})

# 纯算术单元测试（不链接 AWTK）
add_executable(recycle_view_math_test
    tests/recycle_view_math_test.c
    src/recycle_view_math.c)
target_include_directories(recycle_view_math_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(recycle_view_math_test m)

# GUI 演示/集成测试（仅 x86 平台编译）
if(PLATFORM STREQUAL "x86")
  add_executable(recycle_view_test tests/recycle_view_test.c)
  target_include_directories(recycle_view_test PRIVATE ${AWTK_INCLUDE}/ext_widgets)
  target_link_libraries(recycle_view_test ${LIBRARY_NAME} ${AWTK_SO} m dl pthread)
  install(TARGETS recycle_view_test RUNTIME DESTINATION ${LIBRARY_NAME})
endif()

install(
    TARGETS ${LIBRARY_NAME}
    LIBRARY DESTINATION ${LIBRARY_NAME}
)

install(
    DIRECTORY include
    DESTINATION ${LIBRARY_NAME}
)
