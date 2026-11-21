zephyr_interface_library_named(lob_hero_b2b)

target_include_directories(lob_hero_b2b
	INTERFACE
		"${CMAKE_CURRENT_LIST_DIR}/inc"
)

target_link_libraries(lob_hero_b2b INTERFACE
	breeze::module::b2b
	lob_hero::models
)

add_library(lob_hero::b2b ALIAS lob_hero_b2b)
