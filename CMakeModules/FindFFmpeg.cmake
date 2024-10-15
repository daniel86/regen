# Locate ffmpeg
# This module defines
# FFMPEG_LIBRARIES
# FFMPEG_FOUND, if false, do not try to link to ffmpeg
# FFMPEG_INCLUDE_DIRS, where to find the headers
#
# $FFMPEG_DIR is an environment variable that would
# correspond to the ./configure --prefix=$FFMPEG_DIR
#
# Created by Robert Osfield.
# Modified for regen by Daniel Beßler.


#In ffmpeg code, old version use "#include <header.h>" and newer use "#include <libname/header.h>"
#In OSG ffmpeg plugin, we use "#include <header.h>" for compatibility with old version of ffmpeg

#We have to search the path which contain the header.h (usefull for old version)
#and search the path which contain the libname/header.h (usefull for new version)

#Then we need to include ${FFMPEG_libname_INCLUDE_DIRS} (in old version case, use by ffmpeg header and osg plugin code)
#                                                       (in new version case, use by ffmpeg header) 
#and ${FFMPEG_libname_INCLUDE_DIRS/libname}             (in new version case, use by osg plugin code)

include(Utility)

# Macro to find header and lib directories
# example: FFMPEG_FIND(AVFORMAT avformat avformat.h)
MACRO(FFMPEG_FIND varname shortname headername)
    # old version of ffmpeg put header in $prefix/include/[ffmpeg]
    # so try to find header in include directory

    find_include_path(
        FFMPEG_${varname}
        ENV FFMPEG_DIR
        NAMES ${headername}
        PATH_SUFFIXES ffmpeg)

    # newer version of ffmpeg put header in $prefix/include/[ffmpeg/]lib${shortname}
    # so try to find lib${shortname}/header in include directory
    if(NOT FFMPEG_${varname}_INCLUDE_DIRS)
        find_include_path(
            FFMPEG_${varname}
            ENV FFMPEG_DIR
            NAMES lib${shortname}/${headername}
            PATH_SUFFIXES ffmpeg)
    endif()

    find_library_path(FFMPEG_${varname} ENV FFMPEG_DIR
        NAMES ${shortname} lib${shortname})

    if (FFMPEG_${varname}_LIBRARIES)
        set(FFMPEG_${varname}_FOUND 1)
        set(HAS_${varname} 1)
        message(STATUS "  ${shortname} found")
    else()
        message(STATUS "  ${shortname} NOT found")
    endif()

ENDMACRO(FFMPEG_FIND)

message(STATUS "Searching for FFmpeg libraries:")
FFMPEG_FIND(LIBAVFORMAT avformat avformat.h)
FFMPEG_FIND(LIBAVCODEC  avcodec  avcodec.h)
FFMPEG_FIND(LIBAVUTIL   avutil   avutil.h)
FFMPEG_FIND(LIBSWSCALE  swscale  swscale.h)
FFMPEG_FIND(LIBSWRESAMPLE  swresample  swresample.h)

# try hard to find include directory
set(FFMPEG_ROOT $ENV{FFMPEG_DIR})
if(NOT FFMPEG_LIBAVFORMAT_INCLUDE_DIRS)
    if(FFMPEG_ROOT)
        set(FFMPEG_LIBAVFORMAT_INCLUDE_DIRS "$ENV{FFMPEG_DIR}/include")
    else()
        set(FFMPEG_LIBAVFORMAT_FOUND 0)
        message(STATUS "Could NOT find FFmpeg include directory.")
    endif()
endif()

set(FFMPEG_FOUND "NO")
# Note we don't check FFMPEG_{LIBSWSCALE,LIBAVRESAMPLE}_FOUND here, it's optional.
if(FFMPEG_LIBAVFORMAT_FOUND AND FFMPEG_LIBAVCODEC_FOUND AND FFMPEG_LIBAVUTIL_FOUND AND FFMPEG_LIBSWSCALE_FOUND)
    set(FFMPEG_FOUND "YES")
    set(FFMPEG_INCLUDE_DIRS ${FFMPEG_LIBAVFORMAT_INCLUDE_DIRS})
    set(FFMPEG_LIBRARY_DIRS ${FFMPEG_LIBAVFORMAT_LIBRARY_DIRS})
    set(FFMPEG_LIBRARIES
        ${FFMPEG_LIBAVFORMAT_LIBRARIES}
        ${FFMPEG_LIBAVCODEC_LIBRARIES}
        ${FFMPEG_LIBAVUTIL_LIBRARIES})
    if (FFMPEG_LIBSWSCALE_FOUND)
        set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} ${FFMPEG_LIBSWSCALE_LIBRARIES})
    endif()
    if (FFMPEG_LIBSWRESAMPLE_FOUND)
        set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} ${FFMPEG_LIBSWRESAMPLE_LIBRARIES})
    endif()
endif()

# handle the QUIETLY and REQUIRED arguments and set XXX_FOUND to TRUE if all listed variables are TRUE
include(${CMAKE_CURRENT_LIST_DIR}/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FFmpeg DEFAULT_MSG FFMPEG_LIBRARIES)

