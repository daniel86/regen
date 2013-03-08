
add_definitions( -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS )
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fPIC"  )
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_DEBUG} -fPIC"  )

set(HEADER_INSTALL_PATH include/ogle)
set(SHADER_INSTALL_PATH share/ogle/shader)

configure_file( config.h.cmake config.h )

include_directories(/usr/include/freetype2)

set(SOURCES)
set(BUFFER)

# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/doxyfile @ONLY)
    add_custom_target(doc
        ${DOXYGEN_EXECUTABLE}
        ${CMAKE_CURRENT_BINARY_DIR}/doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
endif(DOXYGEN_FOUND)

foreach(DIR
            algebra
            av
            gl-types
            utility
            animations
            states
            meshes
            shading
            textures
            external)
    add_subdirectory(${DIR})
    get_directory_property(BUFFER DIRECTORY ${DIR} DEFINITION SOURCES)
    list(APPEND SOURCES ${BUFFER})
endforeach(DIR)

add_library(ogle SHARED ${SOURCES})

install(
    TARGETS
        ogle
    LIBRARY DESTINATION lib
)

