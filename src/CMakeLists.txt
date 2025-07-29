cmake_minimum_required(VERSION 3.10)
project(Awtk_3rdLibrary)
set(AWTK_SO "${CMAKE_CURRENT_SOURCE_DIR}/../lib/${PLATFORM}/libawtk.so")

message("AWTK_INCLUDE== ${AWTK_INCLUDE}")
add_subdirectory(line_chart)
add_subdirectory(banner_menu)

