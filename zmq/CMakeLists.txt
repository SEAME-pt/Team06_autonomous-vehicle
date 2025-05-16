cmake_minimum_required(VERSION 3.10)

project(ZeroMQLib VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(ZeroMQLib STATIC ${SOURCES})

# Find ZeroMQ using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)

# Find cppzmq header (it's a header-only library)
find_path(CPPZMQ_INCLUDE_DIR
    NAMES zmq.hpp
    PATHS /usr/local/include /usr/include
    DOC "cppzmq header directory"
)

if(NOT CPPZMQ_INCLUDE_DIR)
    message(FATAL_ERROR "Could not find cppzmq header (zmq.hpp)")
endif()

message(STATUS "Found cppzmq include directory: ${CPPZMQ_INCLUDE_DIR}")

target_include_directories(ZeroMQLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CPPZMQ_INCLUDE_DIR}
    ${ZMQ_INCLUDE_DIRS}
)

# Link ZeroMQ libraries
target_link_libraries(ZeroMQLib PUBLIC ${ZMQ_LIBRARIES})

