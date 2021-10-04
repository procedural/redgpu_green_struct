# This CMakeLists.txt project is for Ubuntu only
project(test)
cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_CXX_STANDARD 14)
link_libraries(
  /opt/RedGpuSDK/libredgpu.so
  /opt/RedGpuSDK/sdk/1.2.135.0/x86_64/lib/libvulkan.so
)
add_executable(test
  test.c
  ../redgpu_green_struct.c
  ../../redgpu_memory_allocator_vma/redgpu_memory_allocator.cpp
  ../../redgpu_memory_allocator_vma/redgpu_memory_allocator_functions.cpp
)
