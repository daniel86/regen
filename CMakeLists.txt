cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	set(REGEN_DEBUG_BUILD 1)
else()
	set(REGEN_DEBUG_BUILD 0)
endif()
message(STATUS "Build type is ${CMAKE_BUILD_TYPE}.")

if(UNIX)
    message(STATUS "Target platform is UNIX.")
endif()
if(WIN32)
    message(STATUS "Target platform is Windows.")
endif()
if(APPLE)
    message(STATUS "Target platform is Apple.")
endif()

# include custom find files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
        ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)

project(regen)
set(REGEN_MICRO_VERSION 0)
set(REGEN_MINOR_VERSION 1)
set(REGEN_MAJOR_VERSION 0)

set(REGEN_BRIEF_DESCRIPTION "Real-time-Graphics-Engine")

############
#### Compiler configuration
###########

# include extra warnings specific to C++ (not on Win, visual studio is much to verbose then)
if(NOT WIN32)
  add_definitions( -Wall )
endif()

# warns about C++ code which breaks some of the programming guidelines
# given in the books "Effective C++" and "More Effective C++"
# add_definitions( -Weffc++ )

# optimization level
# add_definitions( -O2 )

if(UNIX) # gcc options
    # Math Unit Optimizations
    add_definitions( -mfpmath=sse )

    # take advantage of the additional instructions in the
    # MMX, SSE, SSE2, SSE3 and 3dnow extensions
    add_definitions( -mmmx -msse -msse2 -msse3 -m3dnow )
endif()

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
set(BOOST_COMPONENTS thread system date_time filesystem regex)
if(WIN32)
	# Note: link error on msvs without this
	set(BOOST_COMPONENTS ${BOOST_COMPONENTS} chrono)
endif()
find_package(Boost ${Boost_MIN_VERSION}
	COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
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

if(UNIX)
  find_library_path(LIBZ NAMES libz.a libz z)
  find_library_path(LIBBZ2 NAMES libbz2.a libbz2 bz2)
endif()

message(STATUS "")
message(STATUS "regen will link against following libraries:")
message(STATUS "  OpenGL:${OPENGL_LIBRARIES};")
message(STATUS "  glew:${GLEW_LIBRARIES};")
message(STATUS "  Boost:${Boost_LIBRARIES};")
message(STATUS "  DevIL:${DEVIL_LIBRARIES};")
message(STATUS "  assimp:${ASSIMP_LIBRARIES};")
message(STATUS "  Freetype:${FREETYPE_LIBRARIES};")
message(STATUS "  OpenAL:${OPENAL_LIBRARIES};")
message(STATUS "  alut:${ALUT_LIBRARIES};")
message(STATUS "  FFmpeg:${FFMPEG_LIBRARIES};")
if(UNIX)
  message(STATUS "  libz:${LIBZ_LIBRARIES};")
  message(STATUS "  libbz2:${LIBBZ2_LIBRARIES};")
endif()
message(STATUS "")
message(STATUS "regen will include headers from following directories:")
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

# allow includes without AL/ prefix. Some openAL versions require this.
include_directories(${OPENAL_INCLUDE_DIRS}/AL)

set(REGEN_LIBRARIES
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${Boost_LIBRARIES}
    ${DEVIL_LIBRARIES}
    ${ASSIMP_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${OPENAL_LIBRARIES}
    ${ALUT_LIBRARIES}
    ${FFMPEG_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)
if(UNIX)
  set(REGEN_LIBRARIES
    ${REGEN_LIBRARIES}
    ${LIBZ_LIBRARIES}
    ${LIBBZ2_LIBRARIES}
)
endif ()
string(REPLACE ";" " " REGEN_LIBRARIES_LIST "${REGEN_LIBRARIES}")

###########
###########

# allow includes like <regen/xxx.h> even if the engine is not installed
include_directories(.)
# build library
add_subdirectory(regen)
# build demos
add_subdirectory(applications)

###########
## Packaging
###########

include(InstallRequiredSystemLibraries)
if(UNIX)
  set(CPACK_GENERATOR "TGZ")
endif()
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${REGEN_BRIEF_DESCRIPTION}")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${REGEN_MAJOR_VERSION}")
set(CPACK_PACKAGE_VERSION_MINOR "${REGEN_MINOR_VERSION}")
set(CPACK_PACKAGE_VERSION_PATCH "${REGEN_MICRO_VERSION}")
if(WIN32)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  set(CPACK_PACKAGE_ICON "${CMake_SOURCE_DIR}/regen/res\\\\icon-small.png")
  set(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} ${PROJECT_NAME}")
  set(CPACK_NSIS_HELP_LINK "http:\\\\\\\\daniel86.github.io/regen/")
  set(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\daniel86.github.io/regen/")
  set(CPACK_NSIS_CONTACT "daniel@orgizm.net")
  set(CPACK_NSIS_MODIFY_PATH ON)
endif()
include(CPack)
