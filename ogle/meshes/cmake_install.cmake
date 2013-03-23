# Install script for directory: /home/daniel/git/ogle/ogle/meshes

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
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ogle/meshes" TYPE FILE FILES
    "/home/daniel/git/ogle/ogle/meshes/assimp-importer.h"
    "/home/daniel/git/ogle/ogle/meshes/attribute-less-mesh.h"
    "/home/daniel/git/ogle/ogle/meshes/box.h"
    "/home/daniel/git/ogle/ogle/meshes/cone.h"
    "/home/daniel/git/ogle/ogle/meshes/mesh-state.h"
    "/home/daniel/git/ogle/ogle/meshes/particles.h"
    "/home/daniel/git/ogle/ogle/meshes/particle-cloud.h"
    "/home/daniel/git/ogle/ogle/meshes/rectangle.h"
    "/home/daniel/git/ogle/ogle/meshes/sphere.h"
    "/home/daniel/git/ogle/ogle/meshes/sky.h"
    "/home/daniel/git/ogle/ogle/meshes/texture-mapped-text.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/ogle/meshes/shader" TYPE FILE FILES
    "/home/daniel/git/ogle/ogle/meshes/shader/cloud_particles.glsl"
    "/home/daniel/git/ogle/ogle/meshes/shader/geometry.glsl"
    "/home/daniel/git/ogle/ogle/meshes/shader/gui.glsl"
    "/home/daniel/git/ogle/ogle/meshes/shader/mesh.glsl"
    "/home/daniel/git/ogle/ogle/meshes/shader/particles.glsl"
    "/home/daniel/git/ogle/ogle/meshes/shader/rain_particles.glsl"
    "/home/daniel/git/ogle/ogle/meshes/shader/sky.glsl"
    "/home/daniel/git/ogle/ogle/meshes/shader/snow_particles.glsl"
    "/home/daniel/git/ogle/ogle/meshes/shader/sprite_sphere.glsl"
    "/home/daniel/git/ogle/ogle/meshes/shader/sprite.glsl"
    "/home/daniel/git/ogle/ogle/meshes/shader/texture_mapped_text.glsl"
    "/home/daniel/git/ogle/ogle/meshes/shader/transparent_mesh.glsl"
    "/home/daniel/git/ogle/ogle/meshes/shader/volume.glsl"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

