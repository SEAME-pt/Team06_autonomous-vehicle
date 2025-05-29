cmake_minimum_required(VERSION 3.10)

project(ZeroMQLib VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Print debugging information
message(STATUS "System name: ${CMAKE_SYSTEM_NAME}")
message(STATUS "System processor: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "System libraries directory: ${CMAKE_SYSTEM_LIBRARY_PATH}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(ZeroMQLib STATIC ${SOURCES})

# Try to use pkg-config first
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    message(STATUS "PkgConfig found, trying to locate libzmq...")
    pkg_check_modules(PC_ZMQ QUIET libzmq)
endif()

# Find ZeroMQ library directly with broader search paths
find_library(ZMQ_LIBRARY
    NAMES zmq libzmq
    PATHS
        ${PC_ZMQ_LIBRARY_DIRS}
        /usr/local/lib
        /usr/lib
        /usr/lib/arm-linux-gnueabihf
        /usr/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu
        /lib
        /lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu
    DOC "ZeroMQ library"
)

if(NOT ZMQ_LIBRARY)
    # Try to find the library by its full name
    message(STATUS "Searching for specific libzmq.so versions...")
    find_library(ZMQ_LIBRARY
        NAMES libzmq.so.5 zmq.so.5 libzmq.so.4 zmq.so.4 libzmq.so.3 zmq.so.3
        PATHS
            /usr/local/lib
            /usr/lib
            /usr/lib/arm-linux-gnueabihf
            /usr/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu
            /lib
            /lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu
        DOC "ZeroMQ library"
    )
endif()

if(NOT ZMQ_LIBRARY)
    # Last resort - try to find any libzmq.so file in the system
    execute_process(
        COMMAND find /usr -name "libzmq.so*" -o -name "zmq.so*"
        OUTPUT_VARIABLE FOUND_ZMQ_LIBRARIES
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(FOUND_ZMQ_LIBRARIES)
        message(STATUS "Found ZeroMQ libraries using find command: ${FOUND_ZMQ_LIBRARIES}")
        string(REGEX REPLACE "\n" ";" FOUND_ZMQ_LIBRARIES_LIST "${FOUND_ZMQ_LIBRARIES}")
        list(GET FOUND_ZMQ_LIBRARIES_LIST 0 ZMQ_LIBRARY)
    else()
        message(FATAL_ERROR "Could not find libzmq. Searched with names: zmq, libzmq, libzmq.so.[3-5]")
    endif()
endif()

message(STATUS "Found ZeroMQ library: ${ZMQ_LIBRARY}")

# Find cppzmq header (it's a header-only library)
find_path(CPPZMQ_INCLUDE_DIR
    NAMES zmq.hpp
    PATHS
        ${PC_ZMQ_INCLUDE_DIRS}
        /usr/local/include
        /usr/include
    DOC "cppzmq header directory"
)

if(NOT CPPZMQ_INCLUDE_DIR)
    # Try to find it with find command
    execute_process(
        COMMAND find /usr -name "zmq.hpp"
        OUTPUT_VARIABLE FOUND_CPPZMQ_HEADER
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(FOUND_CPPZMQ_HEADER)
        message(STATUS "Found cppzmq header using find command: ${FOUND_CPPZMQ_HEADER}")
        get_filename_component(CPPZMQ_INCLUDE_DIR "${FOUND_CPPZMQ_HEADER}" DIRECTORY)
    else()
        message(FATAL_ERROR "Could not find cppzmq header (zmq.hpp)")
    endif()
endif()

message(STATUS "Found cppzmq include directory: ${CPPZMQ_INCLUDE_DIR}")

# Find ZeroMQ headers
find_path(ZMQ_INCLUDE_DIR
    NAMES zmq.h
    PATHS
        ${PC_ZMQ_INCLUDE_DIRS}
        /usr/local/include
        /usr/include
    DOC "ZeroMQ header directory"
)

if(NOT ZMQ_INCLUDE_DIR)
    # Try to find it with find command
    execute_process(
        COMMAND find /usr -name "zmq.h"
        OUTPUT_VARIABLE FOUND_ZMQ_HEADER
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(FOUND_ZMQ_HEADER)
        message(STATUS "Found ZeroMQ header using find command: ${FOUND_ZMQ_HEADER}")
        get_filename_component(ZMQ_INCLUDE_DIR "${FOUND_ZMQ_HEADER}" DIRECTORY)
    else()
        message(FATAL_ERROR "Could not find zmq.h header")
    endif()
endif()

message(STATUS "Found ZeroMQ include directory: ${ZMQ_INCLUDE_DIR}")

# Make sure compilation flags are set correctly
get_filename_component(ZMQ_LIBRARY_DIR "${ZMQ_LIBRARY}" DIRECTORY)
link_directories(${ZMQ_LIBRARY_DIR})

target_include_directories(ZeroMQLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CPPZMQ_INCLUDE_DIR}
    ${ZMQ_INCLUDE_DIR}
)

# Link ZeroMQ library
target_link_libraries(ZeroMQLib PUBLIC ${ZMQ_LIBRARY})

