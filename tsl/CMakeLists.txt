set(CMAKE_AUTOMOC ON)
find_package(Qt5Network)

include_directories(${Qt5Network_INCLUDE_DIRS})

add_library(
	tsl-server
	tsl-server.cpp
)

add_executable(tsl-test
	tsl-test-main.cpp
	tsl-test.cpp
	
)

target_link_libraries(tsl-server Qt5::Network)

target_link_libraries(tsl-test Qt5::Core Qt5::Widgets tsl-server)
