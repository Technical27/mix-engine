cmake_minimum_required(VERSION 3.16)
project(mix-engine)

set (CMAKE_CXX_STANDARD 17)

set (COMMON_FLAGS "-std=c++17 -Wall -Wextra -pedantic -msse2 -mavx")

set (CMAKE_CXX_FLAGS_DEBUG "${COMMON_FLAGS} -Og -g3 -DUSE_VALIDATION_LAYERS")
set (CMAKE_CXX_FLAGS_RELEASE "${COMMON_FLAGS} -O3 -s")

find_package(SDL2 REQUIRED FATAL_ERROR)
find_package(Vulkan REQUIRED FATAL_ERROR)
find_package(glm REQUIRED FATAL_ERROR)

add_executable(main
  src/main.cpp

  src/engine/vulkan/init.cpp
  src/engine/vulkan/validationLayers.cpp
  src/engine/vulkan/device.cpp
  src/engine/vulkan/swapchain.cpp
  src/engine/vulkan/pipeline.cpp
  src/engine/vulkan/frame.cpp
  src/engine/vulkan/buffer.cpp
  src/engine/vulkan/image.cpp

  src/engine/engine.cpp

  src/engine/utils/file.cpp
)

include_directories("${CMAKE_SOURCE_DIR}/stb" "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(main SDL2 Vulkan::Vulkan glm)
