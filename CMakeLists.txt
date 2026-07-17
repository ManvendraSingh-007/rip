cmake_minimum_required(VERSION 3.15)
project(Rip VERSION 1.0 LANGUAGES CXX)

# Force modern C++17 standard
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Point CMake to your header folder
include_directories(include)

# Define files to compile
add_executable(rip 
    src/main.cpp 
    src/rip_archive.cpp
)

