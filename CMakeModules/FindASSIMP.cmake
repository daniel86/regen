
include(Utility)

find_include_path(ASSIMP_OLD  ENV ASSIMP_DIR  NAMES assimp/assimp.h)
find_include_path(ASSIMP_NEW  ENV ASSIMP_DIR  NAMES assimp/scene.h)
if(ASSIMP_NEW_INCLUDE_DIRS)
    set(HAS_OLD_ASSIMP_STRUCTURE 0)
    set(ASSIMP_INCLUDE_DIRS ${ASSIMP_NEW_INCLUDE_DIRS})
else()
    set(HAS_OLD_ASSIMP_STRUCTURE 1)
    set(ASSIMP_INCLUDE_DIRS ${ASSIMP_OLD_INCLUDE_DIRS})
endif()

find_library_path(ASSIMP NAMES assimp libassimp)
find_library_path(ASSIMP_DEBUG  ENV ASSIMP_DIR  NAMES assimpd libassimpd libassimp_d)
if(ASSIMP_LIBRARIES AND ASSIMP_DEBUG_LIBRARIES)
    set(ASSIMP_LIBRARIES optimized "${ASSIMP_LIBRARIES}" debug "${ASSIMP_DEBUG_LIBRARIES}")
endif()

# handle the QUIETLY and REQUIRED arguments and set XXX_FOUND to TRUE if all listed variables are TRUE
include(${CMAKE_CURRENT_LIST_DIR}/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ASSIMP DEFAULT_MSG ASSIMP_LIBRARIES)

