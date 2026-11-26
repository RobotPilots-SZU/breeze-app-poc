cmake_minimum_required(VERSION 3.20.0)

if(NOT BOARD)
    set(BOARD damiao_mc02)
endif()

# clangd reads the actual Zephyr compiler flags and generated include paths.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(infantry_dog LANGUAGES C CXX ASM)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)

# Record libstdc++ paths explicitly for clangd 14's standalone header parsing.
# Ask the selected compiler so these paths follow the installed toolchain.
execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} -E -x c++ -v -
    INPUT_FILE /dev/null
    OUTPUT_QUIET
    ERROR_VARIABLE compiler_include_search
    COMMAND_ERROR_IS_FATAL ANY
)
string(REPLACE "\n" ";" compiler_include_search "${compiler_include_search}")
foreach(include_dir IN LISTS compiler_include_search)
    string(STRIP "${include_dir}" include_dir)
    if(IS_DIRECTORY "${include_dir}" AND include_dir MATCHES "/include/c\\+\\+/")
        target_compile_options(app PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-isystem${include_dir}>")
    endif()
endforeach()

target_sources(app PRIVATE
    src/main.cpp
    src/chassis_mec.cpp
    src/chassis_remote.cpp
)

if(CONFIG_ALGO)
    target_link_libraries(app PRIVATE robotpilots::algorithm)
endif()
