zephyr_library_named(lob_hero_sensors)

file(GLOB SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
)

target_sources(lob_hero_sensors
	PRIVATE
		${SOURCES}
)

target_include_directories(lob_hero_sensors
	PUBLIC
		"${CMAKE_CURRENT_LIST_DIR}/inc"
)

target_link_libraries(lob_hero_sensors
	PRIVATE
		lob_hero_interface
)

add_library(lob_hero::sensors ALIAS lob_hero_sensors)
