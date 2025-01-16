cmake_minimum_required(VERSION 3.10)

# Project name and version
project(jetson06 VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#set(CMAKE_PREFIX_PATH /home/ndo-vale/Qt5.9.9/5.9.9/gcc_64) #Probably adds a dependency to the laptop. Must check this later.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) #

add_subdirectory(ClusterDisplay/SourceCode)

# Add the executable
#add_executable(jetson06 ${SOURCES})

