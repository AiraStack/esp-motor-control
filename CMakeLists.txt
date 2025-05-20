cmake_minimum_required(VERSION 3.31)
project(nospeedget)

set(CMAKE_CXX_STANDARD 14)

include_directories(lib/BLEComm)
include_directories(lib/MotorControl)
include_directories(lib/MotorControl/examples)
include_directories(lib/MotorControl/examples/BasicMovements)
include_directories(lib/SensorControl)

add_executable(nospeedget
        lib/BLEComm/BLEComm.cpp
        lib/BLEComm/BLEComm.h
        lib/MotorControl/examples/BasicMovements/BasicMovements.ino
        lib/MotorControl/MotorControl.cpp
        lib/MotorControl/MotorControl.h
        lib/SensorControl/SensorControl.cpp
        lib/SensorControl/SensorControl.h
        src/main.cpp
        test/src/ble_test.cpp)
