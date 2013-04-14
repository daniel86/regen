
include(Utility)

find_include_path(DEVIL NAMES IL/il.h)

find_library_path(IL ENV DEVIL_DIR NAMES libIL libDEVIL IL DEVIL)
find_library_path(ILUT ENV DEVIL_DIR NAMES libILUT ILUT ilut)
find_library_path(ILU ENV DEVIL_DIR NAMES libILU ILU ilu)

# handle the QUIETLY and REQUIRED arguments and set XXX_FOUND to TRUE if all listed variables are TRUE
include(${CMAKE_CURRENT_LIST_DIR}/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(IL DEFAULT_MSG 
                                  IL_LIBRARIES ILU_LIBRARIES 
                                  ILUT_LIBRARIES DEVIL_INCLUDE_DIRS)

