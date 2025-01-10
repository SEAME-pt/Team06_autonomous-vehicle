cmake_minimum_required(VERSION 3.10)

# Set the project name
project(Jetson VERSION 1.0)

# Set the C++ standard to C++17 (or a higher version if needed)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the source directory for your project
file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)

# Add an executable
add_executable(Middleware ${SOURCES})

# Include directories (headers)
target_include_directories(Middleware PRIVATE ${PROJECT_SOURCE_DIR}/inc)

# Add the paths for the external libraries
# Assuming ZeroMQ is installed in /usr/local
target_include_directories(Middleware PRIVATE /usr/local/include)

# Link the ZeroMQ library
target_link_libraries(Middleware PRIVATE zmq)

# Link the standard C++ library
target_link_libraries(Middleware PRIVATE stdc++)

# Add the library path for ZeroMQ (if it's in /usr/local/lib)
link_directories(/usr/local/lib)

# Find and link pthread
find_package(Threads REQUIRED)
target_link_libraries(Middleware PRIVATE Threads::Threads)
