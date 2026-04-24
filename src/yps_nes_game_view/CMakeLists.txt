cmake_minimum_required(VERSION 3.10)

include_directories(${AWTK_INCLUDE})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

set(LIBRARY_NAME yps_nes_game_view)

file(GLOB SOURCES
    src/*.c
)

add_library(${LIBRARY_NAME} ${SOURCES})
target_link_libraries(${LIBRARY_NAME} ${AWTK_SO})

install(TARGETS ${LIBRARY_NAME} LIBRARY DESTINATION ${LIBRARY_NAME})
install(DIRECTORY include DESTINATION ${LIBRARY_NAME})

if(PLATFORM STREQUAL "x86")
  add_executable(test_nes_keymap
      tests/test_nes_keymap.c
      src/nes_keymap.c)
  target_include_directories(test_nes_keymap PRIVATE ${AWTK_INCLUDE})
  install(TARGETS test_nes_keymap RUNTIME DESTINATION ${LIBRARY_NAME})

  add_executable(test_nes_letterbox
      tests/test_nes_letterbox.c
      src/nes_letterbox.c)
  install(TARGETS test_nes_letterbox RUNTIME DESTINATION ${LIBRARY_NAME})

  add_executable(test_nes_srampath
      tests/test_nes_srampath.c
      src/nes_srampath.c)
  install(TARGETS test_nes_srampath RUNTIME DESTINATION ${LIBRARY_NAME})
endif()
