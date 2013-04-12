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
message("OPENGL_LIBRARIES=${OPENGL_LIBRARIES};")
message("GLEW_LIBRARY=${GLEW_LIBRARY};")

SET(Boost_MIN_VERSION "1.5.0")
SET(Boost_USE_STATIC_LIBS ON)
find_package(Boost ${Boost_MIN_VERSION} COMPONENTS
    thread
    chrono # Note: link error on msvs without this
    system
    date_time
    filesystem
    regex REQUIRED)
find_package(Threads REQUIRED)
message("Boost_LIBRARIES=${Boost_LIBRARIES};")

# check for Image loading lib
find_package(DevIL REQUIRED)
message("IL_LIBRARIES=${IL_LIBRARIES};")
message("ILU_LIBRARIES=${ILU_LIBRARIES};")

# check for Model loader library
find_package(ASSIMP REQUIRED)
IF(ASSIMP_INCLUDE_DIR_NEW)
    SET(HAS_OLD_ASSIMP_STRUCTURE 0)
ELSE(ASSIMP_INCLUDE_DIR_NEW)
    SET(HAS_OLD_ASSIMP_STRUCTURE 1)
ENDIF(ASSIMP_INCLUDE_DIR_NEW)
message("ASSIMP_LIBRARY=${ASSIMP_LIBRARY};")

# check for Font lib
INCLUDE(FindFreetype)
IF(NOT FREETYPE_FOUND)
  FIND_LIBRARY(FREETYPE_LIBRARIES NAMES libfreetype freetype.dll PATHS "./libs/MacOS" "./libs/Windows" DOC "Freetype library")
  FIND_PATH(FREETYPE_INCLUDE_DIRS ftbuild.h "./includes" DOC "Freetype includes")
ENDIF(NOT FREETYPE_FOUND)
INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_DIRS})
message("FREETYPE_LIBRARIES=${FREETYPE_LIBRARIES};")

# check for Audio-Video libs
find_package(OpenAL REQUIRED)
find_package(ALUT REQUIRED)
find_package(FFmpeg REQUIRED)
IF (FFMPEG_LIBAVRESAMPLE_FOUND)
    SET(HAS_AVRESAMPLE 1)
ELSE (FFMPEG_LIBAVRESAMPLE_FOUND)
    SET(HAS_AVRESAMPLE 0)
ENDIF (FFMPEG_LIBAVRESAMPLE_FOUND)
message("OPENAL_LIBRARY=${OPENAL_LIBRARY};")
message("ALUT_LIBRARY=${ALUT_LIBRARY};")
message("FFMPEG_LIBRARIES=${FFMPEG_LIBRARIES};")

SET(ENGINE_LIBS
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARY}
    ${OPENAL_LIBRARY}
    ${ALUT_LIBRARY}
    ${Boost_LIBRARIES}
    ${IL_LIBRARIES} ${ILU_LIBRARIES}
    ${FFMPEG_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${ASSIMP_LIBRARY}
)
string(REPLACE ";" " " ENGINE_LIBS_LIST "${ENGINE_LIBS}")

###########
###########

#link_directories($ENV{CMAKE_INCLUDE_PATH}/boost_1_53_0/bin.v2/libs/thread/build/msvc-11.0/debug/link-static/threading-multi)
#link_directories($ENV{CMAKE_INCLUDE_PATH}/boost_1_53_0/bin.v2/libs/date_time/build/msvc-11.0/debug/link-static/threading-multi)
#link_directories($ENV{CMAKE_INCLUDE_PATH}/boost_1_53_0/bin.v2/libs/system/build/msvc-11.0/debug/link-static/threading-multi)
#link_directories($ENV{CMAKE_INCLUDE_PATH}/boost_1_53_0/bin.v2/libs/chrono/build/msvc-11.0/debug/link-static/threading-multi)
#link_directories($ENV{CMAKE_INCLUDE_PATH}/boost_1_53_0/bin.v2/libs/regex/build/msvc-11.0/debug/link-static/threading-multi)
#link_directories($ENV{CMAKE_INCLUDE_PATH}/boost_1_53_0/bin.v2/libs/filesystem/build/msvc-11.0/debug/link-static/threading-multi)
#link_directories($ENV{CMAKE_INCLUDE_PATH}/DevIL-SDK-x64-1.7.8)
#link_directories($ENV{CMAKE_INCLUDE_PATH}/ffmpeg-20130406-git-7775992-win64-dev/lib)
#link_directories($ENV{CMAKE_INCLUDE_PATH}/freealut-1.1.0-bin/lib)
#link_directories($ENV{CMAKE_INCLUDE_PATH}/freetype-2.3.5/lib)
#link_directories($ENV{CMAKE_INCLUDE_PATH}/glew-1.9.0/lib)
#link_directories($ENV{CMAKE_INCLUDE_PATH}/OpenAL-1.1/lib/Win64)
#link_directories($ENV{CMAKE_INCLUDE_PATH}/assimp-3.0.1270-sdk/lib)

include_directories($ENV{CMAKE_INCLUDE_PATH}/boost_1_53_0)
include_directories($ENV{CMAKE_INCLUDE_PATH}/ffmpeg-20130406-git-7775992-win64-dev/include)
include_directories($ENV{CMAKE_INCLUDE_PATH}/assimp-3.0.1270-sdk/include)
include_directories($ENV{CMAKE_INCLUDE_PATH}/freealut-1.1.0-bin/include)
include_directories($ENV{CMAKE_INCLUDE_PATH}/freetype-2.3.5/include)
include_directories($ENV{CMAKE_INCLUDE_PATH}/DevIL-SDK-x64-1.7.8/include)
include_directories($ENV{CMAKE_INCLUDE_PATH}/glew-1.9.0/include)
include_directories($ENV{CMAKE_INCLUDE_PATH}/OpenAL-1.1/include)
include_directories(.)
# build library
add_subdirectory(regen)
# build demos
add_subdirectory(applications)
