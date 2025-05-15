cmake_minimum_required(VERSION 3.10)

# Set the project name
project(Jetson VERSION 1.0)

# Set the C++ standard to C++17 (or higher if needed)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the source directory for your project and collect source files
file(GLOB SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")

# Check if any source files were found
if(NOT SOURCES)
    message(FATAL_ERROR "No source files found in ${PROJECT_SOURCE_DIR}/src/")
endif()

# Add an executable
add_executable(Middleware ${SOURCES})

# Include project headers
target_include_directories(Middleware PRIVATE
    "${PROJECT_SOURCE_DIR}/inc"
    "${PROJECT_SOURCE_DIR}/../zmq/inc"
)

# Set the library path for ZeroMQLib and libzmq
set(ZMQ_LIB_DIR "${PROJECT_SOURCE_DIR}/../build/zmq")  # Path to the ZeroMQLib build directory
set(ZMQ_LIB "${ZMQ_LIB_DIR}/libZeroMQLib.a")          # Static library file for ZeroMQLib

# Find and link libzmq (ZeroMQ C library)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)

# Link ZeroMQLib and libzmq (ZeroMQ C library)
target_link_libraries(Middleware PRIVATE ${ZMQ_LIB} ${ZMQ_LIBRARIES})

# Find and link pthread
find_package(Threads REQUIRED)
target_link_libraries(Middleware PRIVATE Threads::Threads)
