
include_directories(/usr/include/freetype2)

set(SOURCES
        font/font-manager.cpp
        font/free-type.cpp
)
       
install(
    FILES
        font-manager.h
        free-type.h
    DESTINATION ${HEADER_INSTALL_PATH}/av
    )
