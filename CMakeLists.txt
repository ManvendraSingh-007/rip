cmake_minimum_required(VERSION 3.12)
project(Rip VERSION 1.0 LANGUAGES CXX)

# Force modern C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Point CMake to your header folder
include_directories(include)

# Define files to compile
add_executable(rip 
    src/main.cpp 
    src/tar_parser.cpp
)

