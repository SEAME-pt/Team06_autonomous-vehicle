cmake_minimum_required(VERSION 3.10)

# Set the project name and language
project(Middleware CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories for all builds (executables go to bin/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Enable testing support
enable_testing()

# Find required packages
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)

# Add compiler flags for coverage
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")

# Source files
file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "inc/*.hpp")

# Exclude main.cpp from the library sources
list(FILTER SOURCES EXCLUDE REGEX ".*main.cpp$")

# Set ZMQ library path
set(ZMQ_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/zmq")
set(ZMQ_LIB "${ZMQ_LIB_DIR}/libZeroMQLib.a")

# Create the main library
add_library(Middleware STATIC ${SOURCES} ${HEADERS})
target_include_directories(Middleware
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../zmq/inc
)
target_link_libraries(Middleware
    PRIVATE
        gcov
        Threads::Threads
        ${ZMQ_LIB}
        ${ZMQ_LIBRARIES}
)

# Create the executable
add_executable(MiddlewareMain src/main.cpp)
target_include_directories(MiddlewareMain
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../zmq/inc
)
target_link_libraries(MiddlewareMain
    PRIVATE
        Middleware
        gcov
        Threads::Threads
        ${ZMQ_LIB}
        ${ZMQ_LIBRARIES}
)

# Tests
if(GTEST_FOUND)
    message(STATUS "Google Test found - building tests")
    add_subdirectory(test)
else()
    message(WARNING "Google Test not found - skipping tests")
endif()

# Coverage target
add_custom_target(coverage
    COMMAND lcov --directory . --capture --output-file coverage.info
    COMMAND lcov --remove coverage.info '/usr/*' --output-file coverage.info
    COMMAND lcov --list coverage.info
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating coverage report..."
)
