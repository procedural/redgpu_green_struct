# This CMakeLists.txt project is for Ubuntu only
project(redgpu_green_struct C)
cmake_minimum_required(VERSION 2.8.12)
link_libraries(
  /opt/RedGpuSDK/libredgpu.so
)
add_library(redgpu_green_struct SHARED
  redgpu_green_struct.c
)
