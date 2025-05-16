cmake_minimum_required(VERSION 3.10)

project(ZeroMQLib VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(ZeroMQLib STATIC ${SOURCES})

# Find ZeroMQ library directly
find_library(ZMQ_LIBRARY
    NAMES zmq libzmq
    PATHS /usr/local/lib /usr/lib /usr/lib/arm-linux-gnueabihf
    DOC "ZeroMQ library"
)

if(NOT ZMQ_LIBRARY)
    message(FATAL_ERROR "Could not find libzmq")
endif()

message(STATUS "Found ZeroMQ library: ${ZMQ_LIBRARY}")

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

# Find ZeroMQ headers
find_path(ZMQ_INCLUDE_DIR
    NAMES zmq.h
    PATHS /usr/local/include /usr/include
    DOC "ZeroMQ header directory"
)

if(NOT ZMQ_INCLUDE_DIR)
    message(FATAL_ERROR "Could not find zmq.h header")
endif()

message(STATUS "Found ZeroMQ include directory: ${ZMQ_INCLUDE_DIR}")

target_include_directories(ZeroMQLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CPPZMQ_INCLUDE_DIR}
    ${ZMQ_INCLUDE_DIR}
)

# Link ZeroMQ library
target_link_libraries(ZeroMQLib PUBLIC ${ZMQ_LIBRARY})

