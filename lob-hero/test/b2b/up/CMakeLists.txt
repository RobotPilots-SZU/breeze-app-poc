cmake_minimum_required(VERSION 3.20)

if(NOT BOARD)
	set(BOARD rm_typec)
endif()

set(DTC_OVERLAY_FILE "${WEST_TOPDIR}/lob-hero/up/boards/rm_typec.overlay"
	CACHE STRING "Devicetree overlay for the B2B up test" FORCE)

list(APPEND ZEPHYR_EXTRA_MODULES
	"${WEST_TOPDIR}/breeze-modules"
	"${WEST_TOPDIR}/lob-hero/common"
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lob_hero_b2b_up_test)

target_sources(app PRIVATE
	"${CMAKE_CURRENT_LIST_DIR}/src/main.cpp"
	"${WEST_TOPDIR}/lob-hero/up/src/b2b.cpp"
)

target_include_directories(app PRIVATE
	"${WEST_TOPDIR}/lob-hero/up/inc"
)

target_link_libraries(app PRIVATE
	lob_hero::interface
	lob_hero::b2b
	breeze::module::b2b
)
