#ifndef __REGEN_CONFIG_H
#define __REGEN_CONFIG_H

/* OS define. */
#cmakedefine UNIX
#cmakedefine APPLE
#cmakedefine WIN32

#define REGEN_PROJECT_NAME "@PROJECT_NAME@"
/* Version Macros. */
#define REGEN_MICRO_VERSION @REGEN_MICRO_VERSION@
#define REGEN_MINOR_VERSION @REGEN_MINOR_VERSION@
#define REGEN_MAJOR_VERSION @REGEN_MAJOR_VERSION@

/* Base installation directory. */
#define REGEN_INSTALL_PREFIX "@CMAKE_INSTALL_PREFIX@"
/* Contains the full path to the root of the project source directory,
   i.e. to the nearest directory where CMakeLists.txt contains the PROJECT() command . */
#define REGEN_SOURCE_DIR "@PROJECT_SOURCE_DIR@"

/* Library defines */
#cmakedefine HAS_GLU
#cmakedefine HAS_XMESA
#cmakedefine HAS_IL
#cmakedefine HAS_ILU
#cmakedefine HAS_ILUT
#cmakedefine HAS_OPENAL
#cmakedefine HAS_ALUT
/* assimp defines */
#cmakedefine HAS_ASSIMP
#cmakedefine HAS_OLD_ASSIMP_STRUCTURE
/* Boost defines */
#define REGEN_BOOST_VERSION @Boost_VERSION@
/* FFmpeg defines */
#cmakedefine HAS_LIBAVUTIL
#cmakedefine HAS_LIBAVFORMAT
#cmakedefine HAS_LIBAVCODEC
#cmakedefine HAS_LIBSWSCALE
#cmakedefine HAS_LIBAVRESAMPLE

#endif // __REGEN_CONFIG_H

