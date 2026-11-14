cmake_minimum_required(VERSION 3.16)
project(memsim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Thư viện lõi (KHÔNG dùng Qt)
add_library(memsim_core
    src/core/allocator.cpp
    src/core/csv_io.cpp
    src/core/generator.cpp
    src/core/validator.cpp)
target_include_directories(memsim_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Bản dòng lệnh (không cần Qt)
add_executable(memsim_cli src/cli_main.cpp)
target_link_libraries(memsim_cli PRIVATE memsim_core)

# Giao diện Qt 6
find_package(Qt6 COMPONENTS Widgets)
if(Qt6_FOUND)
    add_executable(memsim_gui
        src/gui/main.cpp
        src/gui/MainWindow.cpp
        src/gui/MainWindow.hpp
        src/gui/MemoryView.cpp
        src/gui/MemoryView.hpp)
    target_link_libraries(memsim_gui PRIVATE memsim_core Qt6::Widgets)
else()
    message(WARNING "Khong tim thay Qt6: bo qua target memsim_gui")
endif()
