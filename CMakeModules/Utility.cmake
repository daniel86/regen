
macro(find_include_path _NAME _FIRST_NAME)
    set(_PARSE_NAMES 0)
    set(_PARSE_SUFFIXES 0)
    set(_PARSE_ENV 0)
    set(_PARSED_NAMES "")
    set(_PARSED_SUFFIXES "")
    set(_PARSED_ENV "")
    
    foreach(ARG ${ARGV})
        if(${ARG} STREQUAL "ENV")
            set(_PARSE_SUFFIXES 0)
            set(_PARSE_NAMES 0)
            set(_PARSE_ENV 1)
        endif()
        if(${ARG} STREQUAL "NAMES")
            set(_PARSE_SUFFIXES 0)
            set(_PARSE_NAMES 1)
            set(_PARSE_ENV 0)
        endif()
        if(${ARG} STREQUAL "PATH_SUFFIXES")
            set(_PARSE_SUFFIXES 1)
            set(_PARSE_NAMES 0)
            set(_PARSE_ENV 0)
        endif()
        
        if(_PARSE_ENV AND NOT ${ARG} STREQUAL "ENV")
            set(_PARSED_ENV ${ARG})
        endif()
        if(_PARSE_NAMES AND NOT ${ARG} STREQUAL "NAMES")
            set(_PARSED_NAMES ${_PARSED_NAMES} ${ARG})
        endif()
        if(_PARSE_SUFFIXES AND NOT ${ARG} STREQUAL "PATH_SUFFIXES")
            set(_PARSED_SUFFIXES ${_PARSED_SUFFIXES} ${ARG})
        endif()
    endforeach()
    
    if(_PARSED_ENV)
        set(_PATH_ENV ${_PARSED_ENV})
    else()
        set(_PATH_ENV ${_NAME}_DIR)
    endif()
    if(NOT _PARSED_NAMES)
        set(_PARSED_NAMES ${_FIRST_NAME})
    endif()
    if(_PARSED_SUFFIXES)
        set(_PARSED_SUFFIXES "PATH_SUFFIXES ${_PARSED_SUFFIXES}")
    else()
        set(_PARSED_SUFFIXES "")
    endif()

    find_path(${_NAME}_INCLUDE_DIRS
      ${_PARSED_NAMES}
      PATHS
      $ENV{${_PATH_ENV}}/include
      ~/Library/Frameworks
      /Library/Frameworks
      /usr/local/include
      /usr/include
      /sw/include        # Fink
      /opt/local/include # DarwinPorts
      /opt/csw/include   # Blastwave
      /opt/include
      ${_PARSED_SUFFIXES}
    )
    if(${_NAME}_INCLUDE_DIRS)
        include_directories(${_NAME}_INCLUDE_DIRS)
    endif ()
endmacro()

macro(find_library_path _NAME _FIRST_NAME)
    set(_PARSE_NAMES 0)
    set(_PARSE_SUFFIXES 0)
    set(_PARSE_ENV 0)
    set(_PATH_NAMES "")
    set(_PATH_SUFFIXES "")
    set(_PARSED_ENV "")
    
    foreach(ARG ${ARGV})
        if(${ARG} STREQUAL "ENV")
            set(_PARSE_SUFFIXES 0)
            set(_PARSE_NAMES 0)
            set(_PARSE_ENV 1)
        endif()
        if(${ARG} STREQUAL "NAMES")
            set(_PARSE_SUFFIXES 0)
            set(_PARSE_NAMES 1)
            set(_PARSE_ENV 0)
        endif()
        if(${ARG} STREQUAL "PATH_SUFFIXES")
            set(_PARSE_SUFFIXES 1)
            set(_PARSE_NAMES 0)
            set(_PARSE_ENV 0)
        endif()
        
        if(_PARSE_NAMES AND NOT ${ARG} STREQUAL "NAMES")
            set(_PATH_NAMES ${_PATH_NAMES} ${ARG})
        endif()
        if(_PARSE_SUFFIXES AND NOT ${ARG} STREQUAL "PATH_SUFFIXES")
            set(_PATH_SUFFIXES ${_PATH_SUFFIXES} ${ARG})
        endif()
        if(_PARSE_ENV AND NOT ${ARG} STREQUAL "ENV")
            set(_PARSED_ENV ${ARG})
        endif()
    endforeach()
    
    if(_PARSED_ENV)
        set(_PATH_ENV ${_PARSED_ENV})
    else()
        set(_PATH_ENV ${_NAME}_DIR)
    endif()
    if(NOT _PATH_NAMES)
        set(_PATH_NAMES ${_FIRST_NAME})
    endif()
    set(_PATH_SUFFIXES ${_PATH_SUFFIXES} lib libs)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_PATH_SUFFIXES ${_PATH_SUFFIXES} lib64 libs64 libs/Win64)
    else()
        set(_PATH_SUFFIXES ${_PATH_SUFFIXES} lib32 libs32 libs/Win32)
    endif()
    
    find_library(${_NAME}_LIBRARIES
      ${_PATH_NAMES}
      PATHS
      $ENV{${_PATH_ENV}}
      ~/Library/Frameworks
      /Library/Frameworks
      /usr/local
      /usr
      /sw         # Fink
      /opt/local  # DarwinPorts
      /opt/csw    # Blastwave
      /opt
      PATH_SUFFIXES ${_PATH_SUFFIXES}
    )
    if(${_NAME}_LIBRARIES)
        set(HAS_${_NAME} 1)
    endif()
endmacro()

