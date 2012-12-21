
add_definitions( -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS )
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fPIC"  )
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_DEBUG} -fPIC"  )

configure_file( config.h.cmake config.h )

include_directories(/usr/include/freetype2)

set(HEADER_INSTALL_PATH include/ogle)

set(SOURCES)
set(BUFFER)

foreach(DIR
            algebra
            av
            gl-types
            utility
            font
            animations
            states
            textures
            shader
            meshes
            shadows
            render-tree
            external)
    add_subdirectory(${DIR})
    get_directory_property(BUFFER DIRECTORY ${DIR} DEFINITION SOURCES)
    list(APPEND SOURCES ${BUFFER})
endforeach(DIR)

add_library(ogle SHARED ${SOURCES})

install(
    FILES
        exceptions/io-exceptions.h
        exceptions/gl-exceptions.h
    DESTINATION include/ogle/exceptions
    )

install(
    TARGETS
        ogle
    LIBRARY DESTINATION lib/ogle
)

