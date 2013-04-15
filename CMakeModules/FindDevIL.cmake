
include(Utility)

find_include_path(DEVIL NAMES IL/il.h)

find_library_path(DEVIL NAMES libIL libDEVIL IL DEVIL)
find_library_path(ILU ENV DEVIL_DIR NAMES libILU ILU ilu)

set(DEVIL_LIBRARIES ${DEVIL_LIBRARIES} ${ILU_LIBRARIES})

# handle the QUIETLY and REQUIRED arguments and set XXX_FOUND to TRUE if all listed variables are TRUE
include(${CMAKE_CURRENT_LIST_DIR}/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(IL DEFAULT_MSG HAS_DEVIL HAS_ILU DEVIL_INCLUDE_DIRS)

