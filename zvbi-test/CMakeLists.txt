cmake_minimum_required(VERSION 2.8.11)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../cmake_modules)

find_package(Threads)
find_package(Zvbi)

include_directories(${ZVBI_INCLUDE_DIRS})

add_executable(zvbi-test
        zvbi-test.c
)
target_link_libraries(zvbi-test ${ZVBI_LIBRARY})


add_executable(anc-dump
        anc-dump.c
)

INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/anc-dump DESTINATION bin)
