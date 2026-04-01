cmake_minimum_required (VERSION 3.10)
include_directories(${AWTK_INCLUDE})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(LIBRARY_NAME breath_ellipse)

file(GLOB SOURCES
    ./*.c
    src/*.c
)

add_library(${LIBRARY_NAME} ${SOURCES})
target_link_libraries(${LIBRARY_NAME} ${AWTK_SO})

add_executable(breath_ellipse_view_test tests/breath_ellipse_view_test.c src/breath_ellipse_math.c)
target_link_libraries(breath_ellipse_view_test m)

install(
    TARGETS ${LIBRARY_NAME}
    LIBRARY DESTINATION ${LIBRARY_NAME}
)

install(
    DIRECTORY include
    DESTINATION ${LIBRARY_NAME}
)
