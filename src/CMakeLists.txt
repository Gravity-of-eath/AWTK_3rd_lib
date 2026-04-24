cmake_minimum_required(VERSION 3.10)
project(Awtk_3rdLibrary)
set(AWTK_SO "${CMAKE_CURRENT_SOURCE_DIR}/../lib/${PLATFORM}/libawtk.so")
# 保留调试符号
set(CMAKE_BUILD_TYPE Debug)

# 或者对于 Release 模式也保留调试符号
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g -DCMAKE_EXPORT_COMPILE_COMMANDS=ON")

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

if(HAS_GPU)
  add_subdirectory(blur_view)
  add_subdirectory(yps_gl_view)
  add_subdirectory(breath_ellipse)
endif()
