
include_directories(/usr/include/freetype2)

add_library( font STATIC
        font-manager.cpp
        free-type.cpp
       )

       
install(
    FILES
        font-manager.h
        free-type.h
    DESTINATION include/ogle/av
    )
