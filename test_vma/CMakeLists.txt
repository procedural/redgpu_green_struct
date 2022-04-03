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
  /opt/github/procedural/redgpu_ultimate/redgpu_ultimate_c.c
  /opt/github/procedural/redgpu_ultimate/redgpu_ultimate_cpp.cpp
)
