cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

if(NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

OPTION(ENABLE_DEBUG "Enable Debugging" OFF)

if(${UNIX})
    message("-- Target platform is UNIX.")
endif(${UNIX})
if(${WIN32})
    message("-- Target platform is Windows.")
endif(${WIN32})
if(${APPLE})
    message("-- Target platform is Apple.")
endif(${APPLE})

# include custom find files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
        ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)

project(regen)
set(ENGINE_MICRO_VERSION 0)
set(ENGINE_MINOR_VERSION 1)
set(ENGINE_MAJOR_VERSION 0)

set(ENGINE_BRIEF_DESCRIPTION "Realtime-Graphics-Engine")

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

# openGL libraries
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
# boost template library
set(Boost_MIN_VERSION "1.5.0")
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost ${Boost_MIN_VERSION} COMPONENTS
    thread
    chrono # Note: link error on msvs without this
    system
    date_time
    filesystem
    regex REQUIRED)
# threading libs
find_package(Threads REQUIRED)
# check for Image loading lib
find_package(DevIL REQUIRED)
# check for Model loader library
find_package(ASSIMP REQUIRED)
# check for Font lib
find_package(Freetype REQUIRED)
# check for Audio-Video libs
find_package(OpenAL REQUIRED)
find_package(ALUT REQUIRED)
find_package(FFmpeg REQUIRED)

# debug lib paths and include dirs
message("-- 
-- OPENGL_LIBRARIES=${OPENGL_LIBRARIES};
-- OPENGL_INCLUDE_DIR=${OPENGL_INCLUDE_DIR};
--
-- GLEW_LIBRARY=${GLEW_LIBRARY};
-- GLEW_INCLUDE_PATH=${GLEW_INCLUDE_PATH};
-- 
-- Boost_LIBRARIES=${Boost_LIBRARIES};
-- Boost_INCLUDE_DIRS=${Boost_INCLUDE_DIRS};
--
-- IL_LIBRARIES=${IL_LIBRARIES};
-- ILU_LIBRARIES=${ILU_LIBRARIES};
-- IL_INCLUDE_DIR=${IL_INCLUDE_DIR};
--
-- ASSIMP_LIBRARY=${ASSIMP_LIBRARY};
-- ASSIMP_INCLUDE_DIR=${ASSIMP_INCLUDE_DIR};
--
-- FREETYPE_LIBRARY=${FREETYPE_LIBRARY};
-- FREETYPE_INCLUDE_DIRS=${FREETYPE_INCLUDE_DIRS};
--
-- OPENAL_LIBRARY=${OPENAL_LIBRARY};
-- OPENAL_INCLUDE_DIR=${OPENAL_INCLUDE_DIR};
--
-- ALUT_LIBRARY=${ALUT_LIBRARY};
-- ALUT_INCLUDE_DIR=${ALUT_INCLUDE_DIR};
--
-- FFMPEG_LIBRARIES=${FFMPEG_LIBRARIES};
-- FFMPEG_INCLUDE_DIRS=${FFMPEG_INCLUDE_DIRS};
-- ")

include_directories(
    ${OPENGL_INCLUDE_DIR}
    ${GLEW_INCLUDE_PATH}
    ${Boost_INCLUDE_DIRS}
    ${IL_INCLUDE_DIR}
    ${ASSIMP_INCLUDE_DIR}
    ${FREETYPE_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIR}
    ${ALUT_INCLUDE_DIR}
    ${FFMPEG_INCLUDE_DIRS}
)

set(ENGINE_LIBS
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARY}
    ${Boost_LIBRARIES}
    ${IL_LIBRARIES} ${ILU_LIBRARIES}
    ${ASSIMP_LIBRARY}
    ${FREETYPE_LIBRARY}
    ${OPENAL_LIBRARY}
    ${ALUT_LIBRARY}
    ${FFMPEG_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)
string(REPLACE ";" " " ENGINE_LIBS_LIST "${ENGINE_LIBS}")

###########
###########

#include_directories($ENV{CMAKE_INCLUDE_PATH}/freetype-2.3.5/include)
#include_directories($ENV{CMAKE_INCLUDE_PATH}/ffmpeg-20130406-git-7775992-win64-dev/include)
#include_directories($ENV{CMAKE_INCLUDE_PATH}/freealut-1.1.0-bin/include)
#include_directories($ENV{CMAKE_INCLUDE_PATH}/assimp-3.0.1270-sdk/include)
#include_directories($ENV{CMAKE_INCLUDE_PATH}/boost_1_53_0)
#include_directories($ENV{CMAKE_INCLUDE_PATH}/DevIL-SDK-x64-1.7.8/include)
#include_directories($ENV{CMAKE_INCLUDE_PATH}/glew-1.9.0/include)
#include_directories($ENV{CMAKE_INCLUDE_PATH}/OpenAL-1.1/include)
include_directories(.)
# build library
add_subdirectory(regen)
# build demos
add_subdirectory(applications)
