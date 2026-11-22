zephyr_library_named(lob_hero_motors)

file(GLOB SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
)

target_sources(lob_hero_motors
	PRIVATE
		${SOURCES}
)

target_include_directories(lob_hero_motors
	PUBLIC
		"${CMAKE_CURRENT_LIST_DIR}/inc"
)

target_link_libraries(lob_hero_motors
	PRIVATE
		lob_hero_interface
)

add_library(lob_hero::motors ALIAS lob_hero_motors)
