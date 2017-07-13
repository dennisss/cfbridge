cmake_minimum_required(VERSION 2.8.3)
project(cfbridge)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(LibUSB REQUIRED)
include_directories(
	lib/Firmware/src/modules/syslink
	include
	${LIBUSB_1_INCLUDE_DIRS}
	${LibUSB_INCLUDE_DIRS}
)

add_executable(bridge
	src/main.cpp
	src/crazyradio.cpp
	src/udpclient.cpp
	src/bridge.cpp
)
target_link_libraries(bridge
	${LIBUSB_1_LIBRARIES}
	${LibUSB_LIBRARIES}
)

# TODO: -O3?
set_target_properties(bridge PROPERTIES COMPILE_FLAGS "-g -std=c++11")
