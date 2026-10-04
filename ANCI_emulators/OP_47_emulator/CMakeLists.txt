cmake_minimum_required(VERSION 3.10)
project(OP_47_emulator_r CXX)

# Включаем стандарты C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Добавляем исполняемый файл
add_executable(OP_47_emulator_r op_47_emulator.cpp)

# Опционально: убираем лишнюю отладочную информацию для уменьшения размера бинарника на плате
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -s")
endif()
