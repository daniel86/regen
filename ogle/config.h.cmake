#ifndef __OGLE_CONFIG_H
#define __OGLE_CONFIG_H

/* The name of the project set by PROJECT() command. */
#cmakedefine PROJECT_NAME "@PROJECT_NAME@"

#cmakedefine UNIX
#cmakedefine APPLE
#cmakedefine WIN32

/* Base installation directory. */
#cmakedefine CMAKE_INSTALL_PREFIX "@CMAKE_INSTALL_PREFIX@"

/* Contains the full path to the root of the project source directory,
   i.e. to the nearest directory where CMakeLists.txt contains the PROJECT() command . */
#cmakedefine PROJECT_SOURCE_DIR "@PROJECT_SOURCE_DIR@"

#cmakedefine HAS_OLD_ASSIMP_STRUCTURE @HAS_OLD_ASSIMP_STRUCTURE@

#cmakedefine HAS_AVRESAMPLE @HAS_AVRESAMPLE@

#endif // __OGLE_CONFIG_H

