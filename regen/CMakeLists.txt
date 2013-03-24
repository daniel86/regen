
set(HEADER_INSTALL_PATH include/regen)
set(SHADER_INSTALL_PATH share/regen)

configure_file(config.h.cmake config.h)
configure_file(regen.pc.in regen.pc)

set(SOURCES)
set(BUFFER)

# add a target to generate API documentation with Doxygen
find_package(Doxygen QUIET)
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
    message("-- 'make doc' generates the documentation.")
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
include_directories(external/libnoise/src/noise)

add_library(regen SHARED ${SOURCES} application.cpp)

install(TARGETS regen LIBRARY DESTINATION lib)
install(FILES regen.pc DESTINATION lib/pkgconfig)
install(FILES application.h DESTINATION ${HEADER_INSTALL_PATH})
