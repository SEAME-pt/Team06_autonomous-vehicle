cmake_minimum_required(VERSION 3.10)

# Set the project name
project(VehicleMiddleware)

# Set the C++ standard to C++17 (or a higher version if needed)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the source directory for your project
set(SOURCES
    src/*.cpp
)

# Include directories (headers)
include_directories(
    inc
)

# Optionally, link libraries here, like Qt, if using in the future
# For now, no external dependencies are needed in the final version.

# Add executable to compile the program
add_executable(${PROJECT_NAME} ${SOURCES})

# Link any libraries if needed in the future
# target_link_libraries(${PROJECT_NAME} <library_name>)
