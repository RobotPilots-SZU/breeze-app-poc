cmake_minimum_required(VERSION 3.20)

if(NOT BOARD)
	set(BOARD rm_typec)
endif()

set(DTC_OVERLAY_FILE "${WEST_TOPDIR}/lob-hero/down/boards/rm_typec.overlay"
	CACHE STRING "Devicetree overlay for the B2B down test" FORCE)

list(APPEND ZEPHYR_EXTRA_MODULES
	"${WEST_TOPDIR}/breeze-modules"
	"${WEST_TOPDIR}/lob-hero/common"
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lob_hero_b2b_down_test)

set(B2B_TEST_MODE "board" CACHE STRING "B2B test mode")
set_property(CACHE B2B_TEST_MODE PROPERTY STRINGS board raw_loopback module_loopback raw_can raw_isotp can1_can2)
set(B2B_TEST_ROLE "rx" CACHE STRING "Raw test role: tx or rx")
set_property(CACHE B2B_TEST_ROLE PROPERTY STRINGS tx rx)
if(B2B_TEST_MODE STREQUAL "board")
  set(B2B_TEST_SOURCE src/main.cpp)
elseif(B2B_TEST_MODE STREQUAL "raw_loopback")
  set(B2B_TEST_SOURCE src/raw_loopback.cpp)
elseif(B2B_TEST_MODE STREQUAL "module_loopback")
  set(B2B_TEST_SOURCE src/module_loopback.cpp)
elseif(B2B_TEST_MODE STREQUAL "raw_can")
  set(B2B_TEST_SOURCE src/raw_can.cpp)
elseif(B2B_TEST_MODE STREQUAL "raw_isotp")
  set(B2B_TEST_SOURCE src/raw_isotp.cpp)
elseif(B2B_TEST_MODE STREQUAL "can1_can2")
  set(B2B_TEST_SOURCE src/can1_can2.cpp)
else()
  message(FATAL_ERROR "Unknown B2B_TEST_MODE: ${B2B_TEST_MODE}")
endif()

if((B2B_TEST_MODE STREQUAL "raw_can" OR B2B_TEST_MODE STREQUAL "raw_isotp")
  AND NOT (B2B_TEST_ROLE STREQUAL "tx" OR B2B_TEST_ROLE STREQUAL "rx"))
  message(FATAL_ERROR "B2B_TEST_ROLE must be tx or rx")
endif()

target_sources(app PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/${B2B_TEST_SOURCE}"
	"${WEST_TOPDIR}/lob-hero/down/src/b2b/module.cpp"
)

target_include_directories(app PRIVATE
	"${WEST_TOPDIR}/lob-hero/down/inc"
)

if(B2B_TEST_MODE STREQUAL "raw_can" OR B2B_TEST_MODE STREQUAL "raw_isotp")
  string(TOUPPER "${B2B_TEST_ROLE}" B2B_TEST_ROLE_UPPER)
  target_compile_definitions(app PRIVATE B2B_TEST_ROLE_${B2B_TEST_ROLE_UPPER}=1)
endif()

target_link_libraries(app PRIVATE
	lob_hero::interface
	lob_hero::b2b
	breeze::module::b2b
)
