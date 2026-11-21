# Copyright (c) 2025 RobotPilots-SZU
# SPDX-License-Identifier: Apache-2.0
#

add_library(lob_hero_interface INTERFACE)
target_link_libraries(lob_hero_interface
	INTERFACE
		breeze::module::models
		breeze::module::common
		ramenx
)
add_library(lob_hero::interface ALIAS lob_hero_interface)

add_subdirectory(modules/models)
add_subdirectory(modules/sensors)
add_subdirectory(modules/b2b)
