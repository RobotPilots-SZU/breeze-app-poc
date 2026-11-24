cmake_minimum_required(VERSION 3.20)

if(NOT BOARD)
    set(BOARD rm_typec)
endif()

set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_LIST_DIR}/boards/rm_typec.overlay"
	CACHE STRING "Devicetree overlay for the down application" FORCE)

list(APPEND ZEPHYR_EXTRA_MODULES
    "${WEST_TOPDIR}/breeze-modules"
    "${WEST_TOPDIR}/lob-hero/common"
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(lob-hero-down)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPNDS
	"${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
)

target_sources(app
	PRIVATE
		${APP_SOURCES}
)

target_include_directories(app
	PRIVATE
		"${CMAKE_CURRENT_LIST_DIR}/inc"
)

target_link_libraries(app
	PRIVATE
		lob_hero::interface
		lob_hero::sensors
		lob_hero::motors
		lob_hero::models
		lob_hero::b2b
		breeze::module::b2b
		breeze::module::gimbal
		breeze::module::common
		breeze::module::models
)
