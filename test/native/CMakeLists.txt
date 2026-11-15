cmake_minimum_required(VERSION 3.20.0)

if(NOT BOARD)
    set(BOARD native_sim/native/64)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(breeze_poc_test_native)

file(GLOB SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
target_sources(app
	PRIVATE
		${SOURCES}
)
