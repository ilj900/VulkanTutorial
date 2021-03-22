cmake_minimum_required(VERSION 3.13)

project(vulkan_tutorial)

find_package(Vulkan REQUIRED)

add_subdirectory(glfw)
add_subdirectory(glm)

set(CMAKE_CXX_STANDARD 17)

include_directories(include
                    glfw/incldue
                    ${VULKAN_PATH}/Include
                    glm)

link_directories(libs)

set(SOURCE src/main.cpp)

set(INCLUDE include/main.h)

add_executable(vulkan_tutorial ${SOURCE} ${INCLUDE})

target_link_libraries(vulkan_tutorial glfw ${GLFW_LIBRARIES} Vulkan::Vulkan)