cmake_minimum_required(VERSION 3.10)
project(scte_emulator CXX)

# Включаем стандарты C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Добавляем исполняемый файл
add_executable(scte_tty_gen scte_dev_tty.cpp)

# Опционально: убираем лишнюю отладочную информацию для уменьшения размера бинарника на плате
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -s")
endif()
