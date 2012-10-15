cmake_minimum_required( VERSION 2.6 FATAL_ERROR )

if(NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

OPTION( ENABLE_DEBUG "Enable Debugging" OFF )

project (ogle)

find_package(GLUT REQUIRED freeglut)

find_package(OpenGL REQUIRED)
find_package(OpenAL REQUIRED)
find_package(Boost REQUIRED COMPONENTS thread system date_time system timer chrono filesystem)

find_library(IL_LIB IL)
find_library(ILU_LIB ILU)

INCLUDE(FindFreetype)
IF(NOT FREETYPE_FOUND)
  FIND_LIBRARY(FREETYPE_LIBRARIES NAMES libfreetype freetype.dll PATHS "./libs/MacOS" "./libs/Windows" DOC "Freetype library")
  FIND_PATH(FREETYPE_INCLUDE_DIRS ftbuild.h "./includes" DOC "Freetype includes")
ENDIF(NOT FREETYPE_FOUND)
INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_DIRS})

SET( OGLE_LIBS
    ogle
    ${OpenGL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${IL_LIB} ${ILU_LIB}
    ${FREETYPE_LIBRARIES}
    -lalut -lopenal
    -lavformat -lavcodec -lavutil -lswscale
    -lassimp
    -lGLEW
    -lGL
)

#include_directories(/usr/include/freetype2)
include_directories(.)

add_subdirectory(ogle)
add_subdirectory(applications)
