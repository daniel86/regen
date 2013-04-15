cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(${UNIX})
    message(STATUS "Target platform is UNIX.")
endif()
if(${WIN32})
    message(STATUS "Target platform is Windows.")
endif()
if(${APPLE})
    message(STATUS "Target platform is Apple.")
endif()

# include custom find files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
        ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)

project(regen)
set(REGEN_MICRO_VERSION 0)
set(REGEN_MINOR_VERSION 1)
set(REGEN_MAJOR_VERSION 0)

set(REGEN_BRIEF_DESCRIPTION "Realtime-Graphics-Engine")

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
message(STATUS "")
message(STATUS "${PROJECT_NAME} will link against following libraries:")
message(STATUS "  OpenGL:${OPENGL_LIBRARIES};")
message(STATUS "  glew:${GLEW_LIBRARIES};")
message(STATUS "  Boost:${Boost_LIBRARIES};")
message(STATUS "  DevIL:${IL_LIBRARIES};${ILU_LIBRARIES};")
message(STATUS "  assimp:${ASSIMP_LIBRARIES};")
message(STATUS "  Freetype:${FREETYPE_LIBRARIES};")
message(STATUS "  OpenAL:${OPENAL_LIBRARIES};")
message(STATUS "  alut:${ALUT_LIBRARIES};")
message(STATUS "  FFmpeg:${FFMPEG_LIBRARIES};")
message(STATUS "")
message(STATUS "${PROJECT_NAME} will include headers from following directories:")
if (${OPENGL_INCLUDE_DIR})
    message(STATUS "  OpenGL:${OPENGL_INCLUDE_DIR};")
endif ()
message(STATUS "  glew:${GLEW_INCLUDE_DIRS};")
message(STATUS "  Boost:${Boost_INCLUDE_DIRS};")
message(STATUS "  DevIL:${DEVIL_INCLUDE_DIRS};")
message(STATUS "  assimp:${ASSIMP_INCLUDE_DIRS};")
message(STATUS "  Freetype:${FREETYPE_INCLUDE_DIRS};")
message(STATUS "  OpenAL:${OPENAL_INCLUDE_DIRS};")
message(STATUS "  alut:${ALUT_INCLUDE_DIRS};")
message(STATUS "  FFmpeg:${FFMPEG_INCLUDE_DIRS};")

set(REGEN_LIBRARIES
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${Boost_LIBRARIES}
    ${IL_LIBRARIES}
    ${ILU_LIBRARIES}
    ${ASSIMP_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${OPENAL_LIBRARIES}
    ${ALUT_LIBRARIES}
    ${FFMPEG_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)
string(REPLACE ";" " " REGEN_LIBRARIES_LIST "${REGEN_LIBRARIES}")

###########
###########

include_directories(.)
# build library
add_subdirectory(regen)
# build demos
add_subdirectory(applications)

