cmake_minimum_required(VERSION 3.10)
include_directories(${AWTK_INCLUDE})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

set(LIBRARY_NAME yps_nes_game_view)

set(WIDGET_SRCS
    src/yps_nes_game_view.c
    src/yps_nes_game_view_register.c
    src/nes_keymap.c
    src/nes_letterbox.c
    src/nes_srampath.c
)

# Audio backend selection
find_package(ALSA QUIET)
if(ALSA_FOUND)
    message(STATUS "yps_nes_game_view: ALSA found, using ALSA audio backend")
    list(APPEND WIDGET_SRCS src/nes_audio_alsa.c)
    add_definitions(-DYPS_NES_HAVE_ALSA=1)
    include_directories(${ALSA_INCLUDE_DIRS})
    set(NES_AUDIO_LIBS ${ALSA_LIBRARIES})
else()
    message(STATUS "yps_nes_game_view: ALSA not found, using null audio backend")
    list(APPEND WIDGET_SRCS src/nes_audio_null.c)
    set(NES_AUDIO_LIBS "")
endif()

add_library(${LIBRARY_NAME} ${WIDGET_SRCS})
target_link_libraries(${LIBRARY_NAME} ${AWTK_SO} ${NES_AUDIO_LIBS} pthread)

install(TARGETS ${LIBRARY_NAME} LIBRARY DESTINATION ${LIBRARY_NAME})
install(DIRECTORY include DESTINATION ${LIBRARY_NAME})

if(PLATFORM STREQUAL "x86")
  add_executable(test_nes_keymap
      tests/test_nes_keymap.c src/nes_keymap.c)
  target_include_directories(test_nes_keymap PRIVATE ${AWTK_INCLUDE})
  install(TARGETS test_nes_keymap RUNTIME DESTINATION ${LIBRARY_NAME})

  add_executable(test_nes_letterbox
      tests/test_nes_letterbox.c src/nes_letterbox.c)
  install(TARGETS test_nes_letterbox RUNTIME DESTINATION ${LIBRARY_NAME})

  add_executable(test_nes_srampath
      tests/test_nes_srampath.c src/nes_srampath.c)
  install(TARGETS test_nes_srampath RUNTIME DESTINATION ${LIBRARY_NAME})

  # Always build null-audio test (uses null impl directly)
  add_executable(test_audio_null
      tests/test_audio_null.c src/nes_audio_null.c)
  install(TARGETS test_audio_null RUNTIME DESTINATION ${LIBRARY_NAME})
endif()
