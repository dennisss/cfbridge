cmake_minimum_required(VERSION 2.8.3)
project(cfbridge)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(LibUSB REQUIRED)
include_directories(
	../Firmware/src/modules/syslink
	include
	${LibUSB_INCLUDE_DIRS}
)

add_executable(bridge
	src/main.c
	src/radio.c
	src/client.c
)
target_link_libraries(bridge

	${LibUSB_LIBRARIES}
)

# TODO: -O3?
set_target_properties(bridge PROPERTIES COMPILE_FLAGS "-g")
