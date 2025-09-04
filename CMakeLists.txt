cmake_minimum_required(VERSION 3.10)

# Project name and version
project(Jetson VERSION 1.0)

# Option to control test building
option(BUILD_TESTS "Build test binaries" OFF)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Configure clang-tidy
# set(CMAKE_CXX_CLANG_TIDY
#     clang-tidy;
#     -checks=*;
#     -extra-arg=-std=c++17)

# Places all generated binaries in the designated folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Runs the cmake on all these directories
add_subdirectory(zmq)
# Pass BUILD_TESTS option to Middleware subdirectory
add_subdirectory(Middleware)
add_subdirectory(Controller)
