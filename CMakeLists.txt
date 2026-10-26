cmake_minimum_required(VERSION 3.10)
project(memsim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Thư viện lõi 
add_library(memsim_core
    core/allocator.cpp
    core/csv_io.cpp
    core/generator.cpp
    core/validator.cpp)
target_include_directories(memsim_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(memsim main.cpp)
target_link_libraries(memsim PRIVATE memsim_core)
