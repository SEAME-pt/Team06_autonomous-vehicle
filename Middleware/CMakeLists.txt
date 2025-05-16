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
find_package(ZeroMQ QUIET)

if(NOT ZeroMQ_FOUND)
    # If ZeroMQ is not found, use the package from the system
    find_library(ZeroMQ_LIBRARIES NAMES zmq)
    if(NOT ZeroMQ_LIBRARIES)
        message(FATAL_ERROR "ZeroMQ library not found")
    endif()
endif()

# Set the ZeroMQLib path
set(ZMQ_LIB "${CMAKE_BINARY_DIR}/zmq/libZeroMQLib.a")

# Include directories for all targets
include_directories(
    "${PROJECT_SOURCE_DIR}/inc"
    "${PROJECT_SOURCE_DIR}/../zmq/inc"
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
target_link_libraries(middleware ${ZMQ_LIB} ${ZeroMQ_LIBRARIES} Threads::Threads)

# Main executable - use the original name "Middleware"
add_executable(Middleware "${PROJECT_SOURCE_DIR}/src/main.cpp")
target_link_libraries(Middleware middleware ${ZMQ_LIB} ${ZeroMQ_LIBRARIES} Threads::Threads)

# Option to enable testing
option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
