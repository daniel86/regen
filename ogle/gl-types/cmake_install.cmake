# Install script for directory: /home/daniel/git/ogle/ogle/gl-types

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
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ogle/gl-types" TYPE FILE FILES
    "/home/daniel/git/ogle/ogle/gl-types/buffer-object.h"
    "/home/daniel/git/ogle/ogle/gl-types/fbo.h"
    "/home/daniel/git/ogle/ogle/gl-types/gl-enum.h"
    "/home/daniel/git/ogle/ogle/gl-types/glsl-directive-processor.h"
    "/home/daniel/git/ogle/ogle/gl-types/glsl-io-processor.h"
    "/home/daniel/git/ogle/ogle/gl-types/rbo.h"
    "/home/daniel/git/ogle/ogle/gl-types/render-state.h"
    "/home/daniel/git/ogle/ogle/gl-types/shader.h"
    "/home/daniel/git/ogle/ogle/gl-types/shader-input.h"
    "/home/daniel/git/ogle/ogle/gl-types/tbo.h"
    "/home/daniel/git/ogle/ogle/gl-types/texture.h"
    "/home/daniel/git/ogle/ogle/gl-types/ubo.h"
    "/home/daniel/git/ogle/ogle/gl-types/vao.h"
    "/home/daniel/git/ogle/ogle/gl-types/vbo.h"
    "/home/daniel/git/ogle/ogle/gl-types/vbo-manager.h"
    "/home/daniel/git/ogle/ogle/gl-types/vertex-attribute.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

