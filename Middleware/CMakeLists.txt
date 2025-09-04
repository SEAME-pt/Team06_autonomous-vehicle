cmake_minimum_required(VERSION 3.10)

# Set the project name
project(ClusterDisplayMiddleware)

# Set the C++ standard to C++17 (or higher if needed)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option to enable code coverage
option(CODE_COVERAGE "Enable coverage reporting" OFF)
if(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  message(STATUS "Enabling code coverage")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 --coverage -fprofile-arcs -ftest-coverage")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

# Find required packages
find_package(Threads REQUIRED)

# Find ZeroMQ using PkgConfig
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)

# Look for cppzmq wrapper
find_package(cppzmq QUIET)
if(NOT cppzmq_FOUND)
    message(STATUS "cppzmq not found via CMake, checking manually")
    find_path(CPPZMQ_INCLUDE_DIRS zmq.hpp PATHS /usr/local/include /usr/include)
    if(NOT CPPZMQ_INCLUDE_DIRS)
        message(FATAL_ERROR "cppzmq headers not found")
    endif()
endif()

# Set the ZeroMQLib path
set(ZMQ_LIB "${CMAKE_BINARY_DIR}/zmq/libZeroMQLib.a")

# Include directories for all targets
include_directories(
    "${PROJECT_SOURCE_DIR}/inc"
    "${PROJECT_SOURCE_DIR}/../zmq/inc"
    ${ZMQ_INCLUDE_DIRS}
    ${CPPZMQ_INCLUDE_DIRS}
)

# Set the source directory for your project and collect source files
file(GLOB SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")

# Check if any source files were found
if(NOT SOURCES)
    message(FATAL_ERROR "No source files found in ${PROJECT_SOURCE_DIR}/src/")
endif()

# Create middleware library (exclude main.cpp)
file(GLOB LIB_SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")
list(FILTER LIB_SOURCES EXCLUDE REGEX ".*main\\.cpp$")
add_library(middleware STATIC ${LIB_SOURCES})
target_link_libraries(middleware ${ZMQ_LIB} ${ZMQ_LIBRARIES} Threads::Threads)

# Main executable - use the original name "Middleware"
add_executable(Middleware "${PROJECT_SOURCE_DIR}/src/main.cpp")
# Link against both the middleware library and the ZeroMQ libraries
target_link_libraries(Middleware middleware ${ZMQ_LIB} ${ZMQ_LIBRARIES} Threads::Threads)

# Use BUILD_TESTS option from parent CMakeLists.txt
# If not defined, default to OFF for production builds
if(NOT DEFINED BUILD_TESTS)
    set(BUILD_TESTS OFF)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
