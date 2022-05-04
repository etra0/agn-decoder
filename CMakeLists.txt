cmake_minimum_required(VERSION 3.8)
project("agn-decoder" C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED true)

add_subdirectory(src)
add_subdirectory(vendor)
