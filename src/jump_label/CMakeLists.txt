cmake_minimum_required (VERSION 3.10)
include_directories(${AWTK_INCLUDE})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(LIBRARY_NAME jump_label)

file(GLOB SOURCES
    ./*.c
    src/*.c
)

add_library(${LIBRARY_NAME} ${SOURCES})
target_link_libraries(${LIBRARY_NAME} ${AWTK_SO})

add_executable(jump_label_test tests/jump_label_test.c src/jump_label_math.c)
target_include_directories(jump_label_test PRIVATE ${AWTK_INCLUDE})
target_link_libraries(jump_label_test m)

install(
    TARGETS ${LIBRARY_NAME}
    LIBRARY DESTINATION ${LIBRARY_NAME}
)

install(
    DIRECTORY include
    DESTINATION ${LIBRARY_NAME}
)
