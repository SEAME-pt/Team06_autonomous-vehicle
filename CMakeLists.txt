cmake_minimum_required(VERSION 3.10)

# Project name and version
project(Jetson VERSION 1.0)

# Option to control test building
option(BUILD_TESTS "Build test binaries" ON)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find Qt packages required by cluster display
find_package(Qt5 COMPONENTS Core Quick REQUIRED)

# Places all generated binaries in the designated folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set absolute path for cluster display
set(CLUSTER_DISPLAY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/modules/cluster-display/ClusterDisplay)

# Runs the cmake on all these directories
add_subdirectory(zmq)
add_subdirectory(${CLUSTER_DISPLAY_PATH})
add_subdirectory(Middleware)
add_subdirectory(Controller)
