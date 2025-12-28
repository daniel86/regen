cmake_minimum_required(VERSION 3.31)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
# We support sanitizing builds below. By default, we use AddressSanitizer
# for "Profile" builds.
# To enable MemorySanitizer, set SANITIZER to "Memory".
if(NOT SANITIZER)
    set(SANITIZER "Address" CACHE STRING "Enable sanitizers: Address, Memory")
endif()

message(STATUS "Build type is ${CMAKE_BUILD_TYPE}.")
if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	set(REGEN_DEBUG_BUILD 1)
    add_definitions( -O2 )
elseif(${CMAKE_BUILD_TYPE} STREQUAL "Profile")
    # Profiling using sanitizer
	set(REGEN_DEBUG_BUILD 1)
	if(${SANITIZER} STREQUAL "Memory")
        set(SANITIZE_FLAGS "-fsanitize=memory -fno-omit-frame-pointer -g -O1")
        add_link_options(-fsanitize=memory)
	else()
        set(SANITIZE_FLAGS "-fsanitize=address,undefined -fno-omit-frame-pointer -g -O1")
        add_link_options(-fsanitize=address,undefined)
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZE_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZE_FLAGS}")
    if(ENABLE_TSAN)
        set(SANITIZE_FLAGS "-fsanitize=thread -fno-omit-frame-pointer -g -O1")
    endif()
    set(CMAKE_BUILD_TYPE Debug)
else()
    # Non-debug build
	set(REGEN_DEBUG_BUILD 0)
    add_definitions( -O2 )
endif()

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

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS "-fPIC")
set(CMAKE_CXX_FLAGS "-std=c++0x -pthread ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_STANDARD 23)

# include extra warnings specific to C++ (not on Win, visual studio is much to verbose then)
if(NOT WIN32)
  add_definitions( -Wall )
endif()

# warns about C++ code which breaks some of the programming guidelines
# given in the books "Effective C++" and "More Effective C++"
# add_definitions( -Weffc++ )

if(UNIX) # gcc options
    add_definitions( -mfpmath=sse -march=native )
endif()

# perform more aggressive floating-point optimizations
# add_definitions( -ffast-math )

# enable macros for C++11 standard limits and constants
add_definitions( -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS )

############
#### Search for Dependencies
###########

set(REGEN_DISABLE_AV OFF)

# GL libraries
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

# boost template library
set(Boost_MIN_VERSION "1.5.0")
set(BOOST_COMPONENTS thread date_time filesystem regex)
if(WIN32)
    set(Boost_USE_STATIC_LIBS ON)
endif()
find_package(Boost ${Boost_MIN_VERSION}
	COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
# threading libs
find_package(Threads REQUIRED)
# Font library: text rendering support
find_package(Freetype REQUIRED)
# JSON library: for serialization
find_package(nlohmann_json REQUIRED)
set(JSON_LIBRARIES nlohmann_json::nlohmann_json)

# Image loading libraries: required for loading textures from files
find_package(PNG REQUIRED)
find_package(DevIL REQUIRED)
if(STB_LIBRARIES)
    set(HAS_STB 1)
    message(STATUS "STB library found.")
else()
    message(STATUS "STB library not found.")
endif()
set(IMG_LIBRARIES
        ${PNG_LIBRARIES}
        ${DEVIL_LIBRARIES}
        ${STB_LIBRARIES})
set(IMG_INCLUDE_DIRS
        ${PNG_INCLUDE_DIRS}
        ${DEVIL_INCLUDE_DIRS}
        ${STB_INCLUDE_DIRS})

# Audio-Video libraries: optional, audio/video playback support
find_package(OpenAL)
find_package(ALUT)
find_package(FFmpeg)
# Define AV_LIBRARIES + AV_INCLUDE_DIRS macro
set(AV_LIBRARIES "")
set(AV_INCLUDE_DIRS "")
if(FFMPEG_LIBRARIES AND OPENAL_FOUND AND ALUT_FOUND AND NOT REGEN_DISABLE_AV)
    message(STATUS "FFMPEG, OpenAL and ALUT libraries found.")
    set(AV_LIBRARIES ${FFMPEG_LIBRARIES} ${OPENAL_LIBRARIES} ${ALUT_LIBRARIES})
    set(AV_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIRS} ${OPENAL_INCLUDE_DIRS} ${ALUT_INCLUDE_DIRS})
    set(HAS_AV_LIBS 1)
else()
    message(STATUS "AV support disabled.")
    set(HAS_AV_LIBS 0)
endif()

# Model loader library: 3D model loading support
find_package(ASSIMP REQUIRED)
# Physics library: physics simulation support
find_package(Bullet REQUIRED)

if(UNIX)
  find_library_path(LIBZ NAMES libz.a libz z)
  find_library_path(LIBBZ2 NAMES libbz2.a libbz2 bz2)
endif()

message(STATUS "")
message(STATUS "regen will link against following libraries:")
message(STATUS "  OpenGL: ${OPENGL_LIBRARIES};")
message(STATUS "  glew: ${GLEW_LIBRARIES};")
message(STATUS "  Boost: ${Boost_LIBRARIES};")
message(STATUS "  JSON: ${JSON_LIBRARIES};")
message(STATUS "  Fonts: ${FREETYPE_LIBRARIES};")
message(STATUS "  Images: ${IMG_LIBRARIES};")
message(STATUS "  Audio+Video: ${AV_LIBRARIES};")
message(STATUS "  Models: ${ASSIMP_LIBRARIES};")
message(STATUS "  Physics: ${BULLET_LIBRARIES};")
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
message(STATUS "  Images:${IMG_INCLUDE_DIRS};")
message(STATUS "  Audio+Video:${AV_INCLUDE_DIRS};")
message(STATUS "  Models:${ASSIMP_INCLUDE_DIRS};")
message(STATUS "  Fonts:${FREETYPE_INCLUDE_DIRS};")
message(STATUS "  Physics:${BULLET_INCLUDE_DIRS};")

enable_testing()
find_package(GTest REQUIRED)

if (HAS_AV_LIBS)
    # allow includes without AL/ prefix. Some openAL versions require this.
    include_directories(${OPENAL_INCLUDE_DIRS}/AL)
endif()
include_directories(${BULLET_INCLUDE_DIRS})

set(REGEN_LIBRARIES
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${Boost_LIBRARIES}
    ${JSON_LIBRARIES}
    ${ASSIMP_LIBRARIES}
    ${FREETYPE_LIBRARIES} -lbrotlidec
    ${IMG_LIBRARIES}
    ${AV_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${BULLET_LIBRARIES}
)
if(UNIX)
  set(REGEN_LIBRARIES
    ${REGEN_LIBRARIES}
    ${LIBZ_LIBRARIES}
    ${LIBBZ2_LIBRARIES}
)
endif ()

# add a target to generate API documentation with Doxygen
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/doc/doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/doc/doxyfile @ONLY)

    if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/doxygen-awesome.css")
        message(STATUS "Downloading \"doxygen-awesome.css\" into directory \"${CMAKE_CURRENT_BINARY_DIR}\".")
        file(DOWNLOAD
                https://raw.githubusercontent.com/jothepro/doxygen-awesome-css/v2.3.2/doxygen-awesome.css
                ${CMAKE_CURRENT_BINARY_DIR}/doxygen-awesome.css)
    endif ()

    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doc/doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/regen
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
    message(STATUS "target doc generates the documentation.")
endif()

###########
###########

# allow includes like <regen/xxx.h> even if the engine is not installed
include_directories(.)
# allow include of generated files
include_directories(${CMAKE_CURRENT_BINARY_DIR})
# build library
add_subdirectory(regen)
# build demos
add_subdirectory(applications)

###########
###########

set(SHARE_INSTALL_PATH share/${PROJECT_NAME})
install(FILES img/logo.png DESTINATION ${SHARE_INSTALL_PATH}/img)
install(FILES img/icon.png DESTINATION ${SHARE_INSTALL_PATH}/img)
install(FILES img/icon-small.png DESTINATION ${SHARE_INSTALL_PATH}/img)

##############
########## Unit Testings
##############

# add an executable target for GTest.
# but the testing code is partly in the *knowrob* library
# where gtest won't find them without using the "--no-as-needed"
# flag for the linker.
add_executable(all_gtests
        tests/gtests.cpp
        tests/shapes/quad-tree-test.cpp)
target_link_libraries(all_gtests
        -Wl,--whole-archive,--no-as-needed
        regen
        -Wl,--no-whole-archive
        ${Boost_Python_COMPONENT}
        ${GTEST_MAIN_LIBRARIES})

##############
########## packaging
##############

include(InstallRequiredSystemLibraries)
if(UNIX)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Daniel Beßler <danielb@uni-bremen.de>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgl-dev, libboost-thread-dev, libboost-system-dev, \
                libboost-date-time-dev, libboost-filesystem-dev, libboost-regex-dev, libassimp-dev, \
                libopenal-dev, libdevil-dev, libfreetype-dev, libbullet-dev, libglew-dev, \
                libglu1-mesa-dev, libgl1-mesa-dev, libpng-dev")
    if (NOT REGEN_DISABLE_AV)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, \
            libopenal-dev, libalut-dev, libavcodec-dev, libavformat-dev, \
            libavutil-dev, libswscale-dev")
    endif()
endif()
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_HOMEPAGE_URL ${PROJECT_WEBSITE})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${REGEN_BRIEF_DESCRIPTION}")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${REGEN_MAJOR_VERSION}")
set(CPACK_PACKAGE_VERSION_MINOR "${REGEN_MINOR_VERSION}")
set(CPACK_PACKAGE_VERSION_PATCH "${REGEN_MICRO_VERSION}")
if(WIN32)
    # There is a bug in NSI that does not handle full unix paths properly. Make
    # sure there is at least one set of four (4) backlashes.
    set(CPACK_PACKAGE_ICON "${CMake_SOURCE_DIR}/regen/res\\\\icon-small.png")
    set(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} ${PROJECT_NAME}")
    set(CPACK_NSIS_HELP_LINK "http:\\\\\\\\daniel86.github.io/regen/")
    set(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\daniel86.github.io/regen/")
    set(CPACK_NSIS_CONTACT "daniel@orgizm.net")
    set(CPACK_NSIS_MODIFY_PATH ON)
endif()
include(CPack)
