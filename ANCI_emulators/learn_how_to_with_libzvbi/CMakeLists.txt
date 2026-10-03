cmake_minimum_required(VERSION 3.10)
project(learn_how_to_with_libzvbi VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Поиск пакетов (лучше сгруппировать в начале)
find_package(Qt5 COMPONENTS Core REQUIRED) 
find_package(Threads REQUIRED)

# 2. Объявление исполняемых файлов
add_executable(libzvbi_injector injector.cpp)
add_executable(libzvbi_decoder decoder.cpp)

# 3. Линковка для libxvbi_injector
target_link_libraries(libzvbi_injector PRIVATE Threads::Threads)

# 4. Линковка для libzvbi_decoder (ДОБАВЛЯЕМ СЮДА Qt5::Core)
target_link_libraries(libzvbi_decoder PRIVATE zvbi)

# Вывод информации о сборке
message(STATUS "Конфигурация инжектора OP-47 завершена. Целевой стандарт: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Конфигурация декодера OP-47 завершена. Целевой стандарт: C++${CMAKE_CXX_STANDARD}")

