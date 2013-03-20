cmake_minimum_required( VERSION 2.6 FATAL_ERROR )

if(NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

OPTION( ENABLE_DEBUG "Enable Debugging" OFF )

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)

project (ogle)

############
#### Compiler optimization
###########

# include extra warnings specific to C++
add_definitions( -Wall )
# TODO: use
# add_definitions( -ansi -pedantic )

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

############
###########

find_package(OpenGL REQUIRED)
find_package(OpenAL REQUIRED)

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

find_library(IL_LIB IL REQUIRED)
find_library(ILU_LIB ILU REQUIRED)

find_package(FFmpeg REQUIRED)

IF (FFMPEG_LIBAVRESAMPLE_FOUND)
    SET(HAS_AVRESAMPLE 1)
ELSE (FFMPEG_LIBAVRESAMPLE_FOUND)
    SET(HAS_AVRESAMPLE 0)
ENDIF (FFMPEG_LIBAVRESAMPLE_FOUND)

INCLUDE(FindFreetype)
IF(NOT FREETYPE_FOUND)
  FIND_LIBRARY(FREETYPE_LIBRARIES NAMES libfreetype freetype.dll PATHS "./libs/MacOS" "./libs/Windows" DOC "Freetype library")
  FIND_PATH(FREETYPE_INCLUDE_DIRS ftbuild.h "./includes" DOC "Freetype includes")
ENDIF(NOT FREETYPE_FOUND)
INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_DIRS})

CHECK_INCLUDE_FILES(assimp/aiScene.h USE_OLD_ASSIMP)

SET( OGLE_LIBS
    ogle
    ${OpenGL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${IL_LIB} ${ILU_LIB}
    ${FFMPEG_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    -lalut -lopenal
    -lassimp
    -lGLEW
    -lGL
)

include_directories(.)

# build library
add_subdirectory(ogle)

# build demos
add_subdirectory(applications)
