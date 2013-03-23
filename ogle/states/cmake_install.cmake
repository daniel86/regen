# Install script for directory: /home/daniel/git/ogle/ogle/states

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
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ogle/states" TYPE FILE FILES
    "/home/daniel/git/ogle/ogle/states/atomic-states.h"
    "/home/daniel/git/ogle/ogle/states/blend-state.h"
    "/home/daniel/git/ogle/ogle/states/blit-state.h"
    "/home/daniel/git/ogle/ogle/states/camera.h"
    "/home/daniel/git/ogle/ogle/states/depth-of-field.h"
    "/home/daniel/git/ogle/ogle/states/depth-state.h"
    "/home/daniel/git/ogle/ogle/states/fbo-state.h"
    "/home/daniel/git/ogle/ogle/states/feedback-state.h"
    "/home/daniel/git/ogle/ogle/states/filter.h"
    "/home/daniel/git/ogle/ogle/states/fullscreen-pass.h"
    "/home/daniel/git/ogle/ogle/states/material-state.h"
    "/home/daniel/git/ogle/ogle/states/model-transformation.h"
    "/home/daniel/git/ogle/ogle/states/picking.h"
    "/home/daniel/git/ogle/ogle/states/shader-configurer.h"
    "/home/daniel/git/ogle/ogle/states/shader-input-state.h"
    "/home/daniel/git/ogle/ogle/states/shader-state.h"
    "/home/daniel/git/ogle/ogle/states/state.h"
    "/home/daniel/git/ogle/ogle/states/state-node.h"
    "/home/daniel/git/ogle/ogle/states/tesselation-state.h"
    "/home/daniel/git/ogle/ogle/states/texture-state.h"
    "/home/daniel/git/ogle/ogle/states/tonemap.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/ogle/states/shader" TYPE FILE FILES
    "/home/daniel/git/ogle/ogle/states/shader/blending.glsl"
    "/home/daniel/git/ogle/ogle/states/shader/blur.glsl"
    "/home/daniel/git/ogle/ogle/states/shader/depth_of_field.glsl"
    "/home/daniel/git/ogle/ogle/states/shader/downsample.glsl"
    "/home/daniel/git/ogle/ogle/states/shader/fxaa.glsl"
    "/home/daniel/git/ogle/ogle/states/shader/material.glsl"
    "/home/daniel/git/ogle/ogle/states/shader/motion_blur.glsl"
    "/home/daniel/git/ogle/ogle/states/shader/picking.glsl"
    "/home/daniel/git/ogle/ogle/states/shader/pixel_velocity.glsl"
    "/home/daniel/git/ogle/ogle/states/shader/sampling.glsl"
    "/home/daniel/git/ogle/ogle/states/shader/tesselation_shader.glsl"
    "/home/daniel/git/ogle/ogle/states/shader/textures.glsl"
    "/home/daniel/git/ogle/ogle/states/shader/tonemap.glsl"
    "/home/daniel/git/ogle/ogle/states/shader/transparency.glsl"
    "/home/daniel/git/ogle/ogle/states/shader/utility.glsl"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

