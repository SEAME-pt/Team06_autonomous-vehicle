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
add_executable(Controller ${SOURCES})

# Include project headers
target_include_directories(Controller PRIVATE
    "${PROJECT_SOURCE_DIR}/inc"
    "${PROJECT_SOURCE_DIR}/../zmq/inc"
)

# Find and link libzmq (ZeroMQ C library)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)

# Link ZeroMQLib and libzmq (ZeroMQ C library)
target_link_libraries(Controller PRIVATE ZeroMQLib ${ZMQ_LIBRARIES})

# Find and link pthread
find_package(Threads REQUIRED)
target_link_libraries(Controller PRIVATE Threads::Threads)
