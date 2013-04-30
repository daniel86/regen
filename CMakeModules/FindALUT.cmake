include(Utility)

find_include_path(ALUT NAMES AL/alut.h)

find_library_path(ALUT NAMES libalut.a alut libalut)
find_library_path(ALUT_DEBUG  ENV ALUT_DIR  NAMES libalutd.a alutd libalutd)
if(ALUT_LIBRARIES AND ALUT_LIBRARIES_DEBUG)
    set(ALUT_LIBRARIES optimized "${ALUT_LIBRARIES}" debug "${ALUT_LIBRARIES_DEBUG}")
endif()

# handle the QUIETLY and REQUIRED arguments and set XXX_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ALUT DEFAULT_MSG ALUT_LIBRARIES ALUT_INCLUDE_DIRS)

