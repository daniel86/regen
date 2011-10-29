cmake_minimum_required( VERSION 2.6 FATAL_ERROR )

if(NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

OPTION( ENABLE_DEBUG "Enable Debugging" OFF )

CONFIGURE_FILE(
    ${CMAKE_SOURCE_DIR}/src/common/include/config.h.cmake
    ${CMAKE_SOURCE_DIR}/src/common/include/config.h )

include_directories(/usr/include/freetype2)

project (ogle)

include_directories(src/algebra/include)
include_directories(src/animations/include)
include_directories(src/buffers/include)
include_directories(src/camera/include)
include_directories(src/common/include)
include_directories(src/gui/include)
include_directories(src/model/include)
include_directories(src/scene/include)
include_directories(src/shader/include)
include_directories(src/textures/include)
include_directories(src/shadows/include)

add_subdirectory(src)
add_subdirectory(demos)

