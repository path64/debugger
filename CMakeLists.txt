cmake_minimum_required(VERSION 2.8)

# stage target
set(PATHDB_STAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
add_custom_target(pathdb-stage ALL)

add_subdirectory(src)
add_subdirectory(etc)
add_subdirectory(share)

