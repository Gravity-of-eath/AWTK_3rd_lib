cmake_minimum_required(VERSION 3.10)
project(Awtk_3rdLibrary)
set(AWTK_SO "${CMAKE_CURRENT_SOURCE_DIR}/../lib/${PLATFORM}/libawtk.so")
# 默认使用 RelWithDebInfo：开 -O2 的同时保留 -g 调试符号。
# 之前是 Debug(-O0)，像素级绘制的控件(conner_gradient 等)性能损失非常大。
# 需要纯调试构建时用: cmake -DCMAKE_BUILD_TYPE=Debug ...
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "build type" FORCE)
endif()
message(STATUS "CMAKE_BUILD_TYPE == ${CMAKE_BUILD_TYPE}")

# 导出 compile_commands.json 供 clangd/IDE 使用（原来误写成了 -D 宏定义）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Release 也保留调试符号
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")

message("AWTK_INCLUDE== ${AWTK_INCLUDE}")

# 没有GPU的平台不编译需要GL支持的控件
set(NO_GPU_PLATFORMS "t113" "cv181")
if(PLATFORM IN_LIST NO_GPU_PLATFORMS)
  set(HAS_GPU OFF)
else()
  set(HAS_GPU ON)
endif()

add_subdirectory(line_chart)
add_subdirectory(banner_menu)
add_subdirectory(conner_gradient)
add_subdirectory(shadow_label)
add_subdirectory(yps_gauge)
add_subdirectory(auto_scale_view)
add_subdirectory(jump_label)
add_subdirectory(yps_nes_game_view)
add_subdirectory(recycle_view)

if(HAS_GPU)
  add_subdirectory(blur_view)
  add_subdirectory(yps_gl_view)
  add_subdirectory(breath_ellipse)
endif()

