zephyr_library_named(lob_hero_b2b)

target_include_directories(lob_hero_b2b
	PUBLIC
		"${CMAKE_CURRENT_LIST_DIR}/inc"
)

target_sources(lob_hero_b2b PRIVATE
	"./src/transmit.cpp"
)

add_library(lob_hero::b2b ALIAS lob_hero_b2b)
