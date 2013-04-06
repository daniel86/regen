# Based on http://freya3d.org/browser/CMakeFind/FindAssimp.cmake
# Based on http://www.daimi.au.dk/~cgd/code/extensions/Assimp/FindAssimp.cmake
# - Try to find Assimp
# Once done this will define
#
# ASSIMP_FOUND - system has Assimp
# ASSIMP_INCLUDE_DIR - the Assimp include directory
# ASSIMP_LIBRARY - Link these to use Assimp
# ASSIMP_LIBRARIES

find_path(ASSIMP_INCLUDE_DIR_OLD assimp.h $ENV{ASSIMP_DIR}/include/assimp)
find_path(ASSIMP_INCLUDE_DIR_NEW scene.h $ENV{ASSIMP_DIR}/include/assimp)
 
find_library (ASSIMP_LIBRARY_DEBUG
    NAMES assimpd libassimpd libassimp_d
    PATHS $ENV{ASSIMP_DIR}/lib)
find_library (ASSIMP_LIBRARY_RELEASE
    NAMES assimp libassimp
    PATHS $ENV{ASSIMP_DIR}/lib)

if (ASSIMP_INCLUDE_DIR AND ASSIMP_LIBRARY_RELEASE)
  set(ASSIMP_FOUND TRUE)
endif()

if (ASSIMP_LIBRARY_RELEASE)
    set (ASSIMP_LIBRARY ${ASSIMP_LIBRARY_RELEASE})
endif()

if (ASSIMP_LIBRARY_DEBUG AND ASSIMP_LIBRARY_RELEASE)
    set (ASSIMP_LIBRARY debug ${ASSIMP_LIBRARY_DEBUG} optimized ${ASSIMP_LIBRARY_RELEASE} )
endif()


if (ASSIMP_FOUND)
  MESSAGE("-- Found Assimp ${ASSIMP_LIBRARIES}")
  mark_as_advanced (ASSIMP_INCLUDE_DIR ASSIMP_LIBRARY ASSIMP_LIBRARIES)
endif()

