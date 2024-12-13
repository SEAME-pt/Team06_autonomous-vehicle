cmake_minimum_required(VERSION 3.10)

# Set the project name
project(Jetson VERSION 1.0)

# Set the C++ standard to C++17 (or a higher version if needed)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the source directory for your project
file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/Middleware/src/*.cpp)

# Include directories (headers)
include_directories(${PROJECT_SOURCE_DIR}/Middleware/inc)

# Add an executable
add_executable(jetson06 ${SOURCES})

# Link any libraries if needed in the future
# target_link_libraries(${PROJECT_NAME} <library_name>)
