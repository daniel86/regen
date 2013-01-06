
include_directories(/usr/include/freetype2)

set(SOURCES
        text/font-manager.cpp
        text/free-type.cpp
        text/texture-mapped-text.cpp
)
       
install(
    FILES
        font-manager.h
        free-type.h
        texture-mapped-text.h
    DESTINATION ${HEADER_INSTALL_PATH}/av
    )
