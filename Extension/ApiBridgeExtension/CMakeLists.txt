cmake_minimum_required(VERSION 3.16)
project(ApiBridgeExtension)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ApiBridgeExtension SHARED src/ApiBridgeExtension.cpp)

if (WIN32)
  target_compile_definitions(ApiBridgeExtension PRIVATE _WIN32_WINNT=0x0601)
  target_link_libraries(ApiBridgeExtension PRIVATE ws2_32)
  set_target_properties(ApiBridgeExtension PROPERTIES OUTPUT_NAME "ApiBridge_x64")
endif()
