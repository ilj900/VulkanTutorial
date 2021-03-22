cmake_minimum_required(VERSION 3.13)

project(vulkan_tutorial)

set(CMAKE_CXX_STANDARD 14)

include_directories(include)

set(SOURCE src/main.cpp)

set(INCLUDE include/main.h)

add_executable(vulkan_tutorial ${SOURCE} ${INCLUDE})