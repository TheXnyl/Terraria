cmake_minimum_required(VERSION 3.20)
project(Lynx LANGUAGES C CXX)

file(GLOB SRC_FILES
  src/*.cpp
  Dependencies/single_file_libs/implementation.cpp
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
add_debug_warnings(${PROJECT_NAME})

target_include_directories(Lynx
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
      Dependencies/single_file_libs/include
)

find_package(Vulkan REQUIRED)

add_subdirectory(Dependencies/glfw)
add_subdirectory(Dependencies/glm)

target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan glfw)

target_compile_definitions(${PROJECT_NAME} PUBLIC VK_NO_PROTOTYPES)
