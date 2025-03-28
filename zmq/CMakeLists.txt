cmake_minimum_required(VERSION 3.10)

project(ZeroMQLib VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(ZeroMQLib STATIC ${SOURCES})

find_package(cppzmq REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)

target_include_directories(ZeroMQLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    /usr/local/include
)

# Link ZeroMQ libraries and export them for dependent targets
target_link_libraries(ZeroMQLib PUBLIC ${ZMQ_LIBRARIES} cppzmq)

