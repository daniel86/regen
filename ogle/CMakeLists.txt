
add_definitions( -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS )
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fPIC"  )
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_DEBUG} -fPIC"  )

set(HEADER_INSTALL_PATH include/ogle)
set(SHADER_INSTALL_PATH share/ogle/shader)

configure_file( config.h.cmake config.h )

include_directories(/usr/include/freetype2)

set(SOURCES)
set(BUFFER)

foreach(DIR
            algebra
            av
            gl-types
            utility
            animations
            states
            textures
            meshes
            shadows
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
    LIBRARY DESTINATION lib
)

