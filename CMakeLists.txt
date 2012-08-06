cmake_minimum_required( VERSION 2.6 FATAL_ERROR )

if(NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

OPTION( ENABLE_DEBUG "Enable Debugging" OFF )

project (ogle)

#include_directories(/usr/include/freetype2)
include_directories(.)

configure_file( config.h.cmake config.h )

add_subdirectory(ogle)
