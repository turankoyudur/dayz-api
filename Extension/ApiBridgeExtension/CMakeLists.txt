cmake_minimum_required(VERSION 3.16)
project(ApiBridgeExtension LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ApiBridgeExtension SHARED
    src/ApiBridgeExtension.cpp
)

target_include_directories(ApiBridgeExtension PRIVATE src)

if(WIN32)
    target_link_libraries(ApiBridgeExtension PRIVATE ws2_32)
    set_target_properties(ApiBridgeExtension PROPERTIES OUTPUT_NAME "ApiBridge_x64")
else()
    set_target_properties(ApiBridgeExtension PROPERTIES OUTPUT_NAME "ApiBridge")
endif()
