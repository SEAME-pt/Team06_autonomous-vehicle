cmake_minimum_required(VERSION 3.10)

# Project name and version
project(Jetson VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(ClusterDisplay/SourceCode)

# Add the executable
add_executable(jetson06 ${SOURCES})

