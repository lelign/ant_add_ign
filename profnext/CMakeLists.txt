cmake_minimum_required(VERSION 2.8.11)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)

IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O3")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3")
        add_definitions(-DQT_NO_EXCEPTIONS)
ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")



find_package(Qt5SerialPort)
find_package(Threads)


include_directories(${Qt5Core_INCLUDE_DIRS})
include_directories(${Qt5SerialPort_INCLUDE_DIRS})
SET (CMAKE_POSITION_INDEPENDENT_CODE ON)


add_executable(profnext-test
        5566.c
        profnext-frontpanel.cpp
        profnext-frontpanel-main.cpp
        profnext-test.cpp
)
target_link_libraries(profnext-test Qt5::SerialPort)

INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/profnext-test DESTINATION bin)
