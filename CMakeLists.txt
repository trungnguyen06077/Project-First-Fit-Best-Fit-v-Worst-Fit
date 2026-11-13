cmake_minimum_required(VERSION 3.10)
project(memsim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Thư viện lõi 
add_library(memsim_core
    src/core/allocator.cpp
    src/core/csv_io.cpp
    src/core/generator.cpp
    src/core/validator.cpp)
target_include_directories(memsim_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(memsim src/gui/main.cpp)
target_link_libraries(memsim PRIVATE memsim_core)
  enable_testing()
  add_executable(test_main tests/test_main.cpp)
  target_link_libraries(test_main PRIVATE memsim_core)
  add_test(NAME core_tests COMMAND test_main ${CMAKE_SOURCE_DIR}/data)