cmake_minimum_required(VERSION 3.10)

# Set the project name
project(Jetson VERSION 1.0)

# Set the C++ standard to C++17 (or a higher version if needed)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the source directory for your project
file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/Middleware/src/*.cpp)

# Add an executable
add_executable(jetson06 ${SOURCES})

# Include directories (headers)
target_include_directories(jetson06 PRIVATE ${PROJECT_SOURCE_DIR}/Middleware/inc)

# Find and link pthread
find_package(Threads REQUIRED)
target_link_libraries(jetson06 PRIVATE Threads::Threads)
