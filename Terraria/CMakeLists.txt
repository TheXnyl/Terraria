cmake_minimum_required(VERSION 3.20)
project(Terraria LANGUAGES CXX)

file(GLOB SRC_FILES
  src/*.cpp
)

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE Lynx)

add_debug_warnings(${PROJECT_NAME})

add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Textures
  ${CMAKE_CURRENT_BINARY_DIR}/Resources/Textures
  COMMENT "Copying textures folder"
)

# add_custom_command(
#   TARGET ${PROJECT_NAME} POST_BUILD
#   COMMAND ${CMAKE_COMMAND} -E copy_directory
#   ${CMAKE_SOURCE_DIR}/Resources/Shaders/Bin
#   ${CMAKE_BINARY_DIR}/Resources/Shaders/Bin
#   COMMENT "Compiling shaders"
# )

add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Shaders/Bin
  ${CMAKE_CURRENT_BINARY_DIR}/Resources/Shaders/bin
  COMMENT "Copying compiled shader binaries"
)
