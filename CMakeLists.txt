cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

if(NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

OPTION(ENABLE_DEBUG "Enable Debugging" OFF)

# include custom find files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
        ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)

project(regen)
SET(ENGINE_MICRO_VERSION 0)
SET(ENGINE_MINOR_VERSION 1)
SET(ENGINE_MAJOR_VERSION 0)

SET(ENGINE_BRIEF_DESCRIPTION "Realtime-Graphics-Engine")

############
#### Compiler configuration
###########

# include extra warnings specific to C++
add_definitions( -Wall )

# warns about C++ code which breaks some of the programming guidelines
# given in the books "Effective C++" and "More Effective C++"
# add_definitions( -Weffc++ )

# optimization level
# add_definitions( -O2 )

# Math Unit Optimizations
add_definitions( -mfpmath=sse )

# take advantage of the additional instructions in the
# MMX, SSE, SSE2, SSE3 and 3dnow extensions
add_definitions( -mmmx -msse -msse2 -msse3 -m3dnow )

add_definitions( -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS )

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fPIC"  )
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fPIC"  )

############
#### Search for Dependencies
###########

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

SET(Boost_MIN_VERSION "1.5.0")
SET(Boost_USE_STATIC_LIBS ON)
SET(Boost_USE_MULTITHREADED ON)
find_package(Boost ${Boost_MIN_VERSION} COMPONENTS
    thread
    system
    date_time
    filesystem
    regex REQUIRED)
find_package(Threads REQUIRED)

# check for Image loading lib
find_library(IL_LIB IL REQUIRED)
find_library(ILU_LIB ILU REQUIRED)

# check for Model loader library
find_package(ASSIMP REQUIRED)
IF(ASSIMP_INCLUDE_DIR_NEW)
    SET(HAS_OLD_ASSIMP_STRUCTURE 0)
ELSE(ASSIMP_INCLUDE_DIR_NEW)
    SET(HAS_OLD_ASSIMP_STRUCTURE 1)
ENDIF(ASSIMP_INCLUDE_DIR_NEW)

# check for Font lib
INCLUDE(FindFreetype)
IF(NOT FREETYPE_FOUND)
  FIND_LIBRARY(FREETYPE_LIBRARIES NAMES libfreetype freetype.dll PATHS "./libs/MacOS" "./libs/Windows" DOC "Freetype library")
  FIND_PATH(FREETYPE_INCLUDE_DIRS ftbuild.h "./includes" DOC "Freetype includes")
ENDIF(NOT FREETYPE_FOUND)
INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_DIRS})

# check for Audio-Video libs
find_package(OpenAL REQUIRED)
find_package(ALUT REQUIRED)
find_package(FFmpeg REQUIRED)
IF (FFMPEG_LIBAVRESAMPLE_FOUND)
    SET(HAS_AVRESAMPLE 1)
ELSE (FFMPEG_LIBAVRESAMPLE_FOUND)
    SET(HAS_AVRESAMPLE 0)
ENDIF (FFMPEG_LIBAVRESAMPLE_FOUND)

SET(ENGINE_LIBS
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${OPENAL_LIBRARY}
    ${ALUT_LIBRARY}
    ${Boost_LIBRARIES}
    ${IL_LIB} ${ILU_LIB}
    ${FFMPEG_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${ASSIMP_LIBRARY}
)
string(REPLACE ";" " " ENGINE_LIBS_LIST "${ENGINE_LIBS}")

###########
###########

include_directories(.)
# build library
add_subdirectory(regen)
# build demos
add_subdirectory(applications)
