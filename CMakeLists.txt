cmake_minimum_required(VERSION 3.14)

project(
  authplugin
  LANGUAGES C)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/minhook")
add_library(authplugin SHARED src/main.c)
target_link_libraries(authplugin PRIVATE minhook)