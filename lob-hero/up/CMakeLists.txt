cmake_minimum_required(VERSION 3.20)

if(NOT BOARD)
    set(BOARD rm_typec)
endif()

list(APPEND ZEPHYR_EXTRA_MODULES
    "${WEST_TOPDIR}/breeze-modules"
    "${WEST_TOPDIR}/lob-hero/common"
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(lob-hero-up)

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
		breeze::module::b2b
)
