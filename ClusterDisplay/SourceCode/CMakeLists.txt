cmake_minimum_required(VERSION 2.8.12)

project(ClusterDisplay LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Core Quick REQUIRED)

include_directories(inc)

add_executable(${PROJECT_NAME} main.cpp
    src/QtSubscriber.cpp inc/QtSubscriber.hpp
    src/BatteryIconObj.cpp inc/BatteryIconObj.hpp
    src/TestBattery.cpp inc/TestBattery.hpp
    src/SpeedometerObj.cpp inc/SpeedometerObj.hpp
    src/TestSpeedSensor.cpp inc/TestSpeedSensor.hpp
    qml.qrc)
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Quick zmq)
