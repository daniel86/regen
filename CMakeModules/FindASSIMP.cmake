# Based on http://freya3d.org/browser/CMakeFind/FindAssimp.cmake
# Based on http://www.daimi.au.dk/~cgd/code/extensions/Assimp/FindAssimp.cmake
# - Try to find Assimp
# Once done this will define
#
# ASSIMP_FOUND - system has Assimp
# ASSIMP_INCLUDE_DIR - the Assimp include directory
# ASSIMP_LIBRARY - Link these to use Assimp
# ASSIMP_LIBRARIES

find_path(ASSIMP_INCLUDE_DIR_OLD NAMES assimp.h HINTS ${ASSIMP_INC_SEARCH_PATH} ${ASSIMP_PKGC_INCLUDE_DIRS} PATH_SUFFIXES assimp)
find_path(ASSIMP_INCLUDE_DIR_NEW NAMES scene.h HINTS ${ASSIMP_INC_SEARCH_PATH} ${ASSIMP_PKGC_INCLUDE_DIRS} PATH_SUFFIXES assimp)
 
find_library (ASSIMP_LIBRARY_DEBUG NAMES assimpd libassimpd libassimp_d PATHS ${ASSIMP_SEARCH_PATHS})
find_library (ASSIMP_LIBRARY_RELEASE NAMES assimp libassimp PATHS ${ASSIMP_SEARCH_PATHS})

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

