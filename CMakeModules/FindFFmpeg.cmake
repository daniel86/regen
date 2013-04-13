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


#In ffmpeg code, old version use "#include <header.h>" and newer use "#include <libname/header.h>"
#In OSG ffmpeg plugin, we use "#include <header.h>" for compatibility with old version of ffmpeg

#We have to search the path which contain the header.h (usefull for old version)
#and search the path which contain the libname/header.h (usefull for new version)

#Then we need to include ${FFMPEG_libname_INCLUDE_DIRS} (in old version case, use by ffmpeg header and osg plugin code)
#                                                       (in new version case, use by ffmpeg header) 
#and ${FFMPEG_libname_INCLUDE_DIRS/libname}             (in new version case, use by osg plugin code)

# Macro to find header and lib directories
# example: FFMPEG_FIND(AVFORMAT avformat avformat.h)
MACRO(FFMPEG_FIND varname shortname headername)
    # old version of ffmpeg put header in $prefix/include/[ffmpeg]
    # so try to find header in include directory
    FIND_PATH(FFMPEG_${varname}_INCLUDE_DIRS ${headername}
        PATHS
        $ENV{FFMPEG_DIR}/include
        ~/Library/Frameworks
        /Library/Frameworks
        /usr/local/include
        /usr/include
        /sw/include # Fink
        /opt/local/include # DarwinPorts
        /opt/csw/include # Blastwave
        /opt/include
        /usr/freeware/include
        PATH_SUFFIXES ffmpeg
        DOC "Location of FFMPEG Headers"
    )

    # newer version of ffmpeg put header in $prefix/include/[ffmpeg/]lib${shortname}
    # so try to find lib${shortname}/header in include directory
    IF(NOT FFMPEG_${varname}_INCLUDE_DIRS)
        FIND_PATH(FFMPEG_${varname}_INCLUDE_DIRS lib${shortname}/${headername}
            $ENV{FFMPEG_DIR}/include
            ~/Library/Frameworks
            /Library/Frameworks
            /usr/local/include
            /usr/include/
            /sw/include # Fink
            /opt/local/include # DarwinPorts
            /opt/csw/include # Blastwave
            /opt/include
            /usr/freeware/include
            PATH_SUFFIXES ffmpeg
            DOC "Location of FFMPEG Headers"
        )
    ENDIF()

    FIND_LIBRARY(FFMPEG_${varname}_LIBRARIES
        NAMES ${shortname}
        PATHS
        $ENV{FFMPEG_DIR}/lib
        ~/Library/Frameworks
        /Library/Frameworks
        /usr/local/lib
        /usr/local/lib64
        /usr/lib
        /usr/lib64
        /sw/lib
        /opt/local/lib
        /opt/csw/lib
        /opt/lib
        /usr/freeware/lib64
        DOC "Location of FFMPEG Libraries"
    )

    if (FFMPEG_${varname}_LIBRARIES)
        SET(FFMPEG_${varname}_FOUND 1)
        message("-- Found FFmpeg ${shortname}")
    else()
        message("-- Could NOT find FFmpeg ${shortname}")
    endif()

ENDMACRO(FFMPEG_FIND)

FFMPEG_FIND(LIBAVFORMAT avformat avformat.h)
FFMPEG_FIND(LIBAVCODEC  avcodec  avcodec.h)
FFMPEG_FIND(LIBAVUTIL   avutil   avutil.h)
FFMPEG_FIND(LIBSWSCALE  swscale  swscale.h)
FFMPEG_FIND(LIBAVRESAMPLE  avresample  avresample.h)

set(FFMPEG_ROOT $ENV{FFMPEG_DIR})
if(NOT FFMPEG_LIBAVFORMAT_INCLUDE_DIRS)
    if(FFMPEG_ROOT)
        set(FFMPEG_LIBAVFORMAT_INCLUDE_DIRS "$ENV{FFMPEG_DIR}/include")
        message("XXX forcing ${FFMPEG_LIBAVFORMAT_INCLUDE_DIRS} ")
    else()
        set(FFMPEG_LIBAVFORMAT_FOUND 0)
        message("-- Could NOT find FFmpeg include directory.")
    endif()
endif()

SET(FFMPEG_FOUND "NO")
# Note we don't check FFMPEG_{LIBSWSCALE,LIBAVRESAMPLE}_FOUND here, it's optional.
IF(FFMPEG_LIBAVFORMAT_FOUND AND FFMPEG_LIBAVCODEC_FOUND AND FFMPEG_LIBAVUTIL_FOUND AND FFMPEG_LIBSWSCALE_FOUND)
    SET(FFMPEG_FOUND "YES")
    SET(FFMPEG_INCLUDE_DIRS ${FFMPEG_LIBAVFORMAT_INCLUDE_DIRS})
    SET(FFMPEG_LIBRARY_DIRS ${FFMPEG_LIBAVFORMAT_LIBRARY_DIRS})
    SET(FFMPEG_LIBRARIES
        ${FFMPEG_LIBAVFORMAT_LIBRARIES}
        ${FFMPEG_LIBAVCODEC_LIBRARIES}
        ${FFMPEG_LIBAVUTIL_LIBRARIES})
    if (FFMPEG_LIBSWSCALE_FOUND)
        set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} ${FFMPEG_LIBSWSCALE_LIBRARIES})
    endif()
    if (FFMPEG_LIBAVRESAMPLE_FOUND)
        set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} ${FFMPEG_LIBAVRESAMPLE_LIBRARIES})
    endif()
ELSE()
    MESSAGE("-- Could NOT find FFmpeg.")
ENDIF()

IF(FFMPEG_LIBAVRESAMPLE_FOUND)
    set(HAS_AVRESAMPLE 1)
ELSE()
    set(HAS_AVRESAMPLE 0)
ENDIF()

