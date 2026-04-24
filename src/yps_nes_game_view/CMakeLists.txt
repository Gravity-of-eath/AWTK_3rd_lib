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
