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

if(CONFIG_POC_COMMON)
# 按实际情况添加
#   add_subdirectory(modules/models)
endif()
