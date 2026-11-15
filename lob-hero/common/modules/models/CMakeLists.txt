zephyr_interface_library_named(lob_hero_models)

target_include_directories(lob_hero_models
	INTERFACE
		"${CMAKE_CURRENT_LIST_DIR}/inc"
)

add_library(lob_hero::models ALIAS lob_hero_models)
