#ifndef __REGEN_CONFIG_H
#define __REGEN_CONFIG_H

#cmakedefine UNIX
#cmakedefine APPLE
#cmakedefine WIN32

#define REGEN_PROJECT_NAME "@PROJECT_NAME@"
#define REGEN_MICRO_VERSION @ENGINE_MICRO_VERSION@
#define REGEN_MINOR_VERSION @ENGINE_MINOR_VERSION@
#define REGEN_MAJOR_VERSION @ENGINE_MAJOR_VERSION@

/* Base installation directory. */
#define REGEN_INSTALL_PREFIX "@CMAKE_INSTALL_PREFIX@"

/* Contains the full path to the root of the project source directory,
   i.e. to the nearest directory where CMakeLists.txt contains the PROJECT() command . */
#define REGEN_SOURCE_DIR "@PROJECT_SOURCE_DIR@"

#cmakedefine SYNCHRONIZE_ANIM_AND_RENDER @SYNCHRONIZE_ANIM_AND_RENDER@

#cmakedefine HAS_OLD_ASSIMP_STRUCTURE @HAS_OLD_ASSIMP_STRUCTURE@

#cmakedefine HAS_AVRESAMPLE @HAS_AVRESAMPLE@

#endif // __REGEN_CONFIG_H

