cmake_minimum_required(VERSION 3.25)
project(MCSAIEditor VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MCS_ASAN "Compilar com AddressSanitizer (GCC/Clang)" OFF)

find_package(Qt6 6.11.3 EXACT REQUIRED COMPONENTS Core Gui Widgets Concurrent Test)
qt_standard_project_setup()
find_package(FFMPEG REQUIRED)

add_library(mcs_warnings INTERFACE)
if(MSVC)
    target_compile_options(mcs_warnings INTERFACE /W4 /WX /permissive- /utf-8)
else()
    target_compile_options(mcs_warnings INTERFACE -Wall -Wextra -Wpedantic -Werror)
endif()

if(MCS_ASAN AND NOT MSVC)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

enable_testing()
add_subdirectory(src)
add_subdirectory(tests)
