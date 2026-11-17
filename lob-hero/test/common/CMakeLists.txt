cmake_minimum_required(VERSION 3.20.0)

list(APPEND ZEPHYR_EXTRA_MODULES
    "${WEST_TOPDIR}/lob-hero/common"
)

if(NOT BOARD)
    set(BOARD native_sim/native/64)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lob_hero_common_test)

target_sources(app PRIVATE src/imu.cpp)
target_include_directories(app PRIVATE
    "${ZEPHYR_POC_COMMON_MODULE_DIR}/modules/sensors/src"
)
target_link_libraries(app PRIVATE lob_hero::sensors lob_hero::interface)
