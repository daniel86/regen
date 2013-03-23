# Install script for directory: /home/daniel/git/ogle/ogle/shading

# Set the install prefix
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "/usr/local")
ENDIF(NOT DEFINED CMAKE_INSTALL_PREFIX)
STRING(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
IF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  IF(BUILD_TYPE)
    STRING(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  ELSE(BUILD_TYPE)
    SET(CMAKE_INSTALL_CONFIG_NAME "")
  ENDIF(BUILD_TYPE)
  MESSAGE(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
ENDIF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)

# Set the component getting installed.
IF(NOT CMAKE_INSTALL_COMPONENT)
  IF(COMPONENT)
    MESSAGE(STATUS "Install component: \"${COMPONENT}\"")
    SET(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  ELSE(COMPONENT)
    SET(CMAKE_INSTALL_COMPONENT)
  ENDIF(COMPONENT)
ENDIF(NOT CMAKE_INSTALL_COMPONENT)

# Install shared libraries without execute permission?
IF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  SET(CMAKE_INSTALL_SO_NO_EXE "0")
ENDIF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ogle/shading" TYPE FILE FILES
    "/home/daniel/git/ogle/ogle/shading/ambient-occlusion.h"
    "/home/daniel/git/ogle/ogle/shading/distance-fog.h"
    "/home/daniel/git/ogle/ogle/shading/light-pass.h"
    "/home/daniel/git/ogle/ogle/shading/light-state.h"
    "/home/daniel/git/ogle/ogle/shading/shading-deferred.h"
    "/home/daniel/git/ogle/ogle/shading/shading-direct.h"
    "/home/daniel/git/ogle/ogle/shading/shadow-map.h"
    "/home/daniel/git/ogle/ogle/shading/t-buffer.h"
    "/home/daniel/git/ogle/ogle/shading/volumetric-fog.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/ogle/shading/shader" TYPE FILE FILES
    "/home/daniel/git/ogle/ogle/shading/shader/fog.glsl"
    "/home/daniel/git/ogle/ogle/shading/shader/shading.glsl"
    "/home/daniel/git/ogle/ogle/shading/shader/shadow_mapping.glsl"
    "/home/daniel/git/ogle/ogle/shading/shader/ssao.glsl"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

