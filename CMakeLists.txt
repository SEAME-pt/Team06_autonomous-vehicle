cmake_minimum_required(VERSION 3.10)

# Project name and version
project(Jetson VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Places all generated binaries in the designated folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Runs the cmake on all these directories
add_subdirectory(ClusterDisplay/SourceCode)
add_subdirectory(Middleware)
add_subdirectory(Controllers)
