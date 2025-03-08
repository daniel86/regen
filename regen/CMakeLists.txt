
set(HEADER_INSTALL_PATH include/${PROJECT_NAME})
set(SHADER_INSTALL_PATH share/${PROJECT_NAME})

configure_file(config.h.cmake config.h)
configure_file(${PROJECT_NAME}.pc.in ${PROJECT_NAME}.pc)

set(SOURCE_DIRS
    math
    av
    gl-types
    utility
    shapes
    animations
    physics
    textures
    text
    states
    camera
    meshes
    effects
    scene
    sky
    external)

add_subdirectory(glsl)

# collect sources from sub directories
foreach(DIR ${SOURCE_DIRS})
    add_subdirectory(${DIR})
    get_directory_property(BUFFER DIRECTORY ${DIR} DEFINITION SOURCES)
    list(APPEND SOURCES ${BUFFER})
endforeach(DIR)
file(GLOB SOURCES_CWD *.cpp)
set(SOURCES ${SOURCES} ${SOURCES_CWD})
include_directories(external/libnoise/src/noise)

add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_link_libraries(${PROJECT_NAME} ${REGEN_LIBRARIES})

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
install(FILES ${CMAKE_BINARY_DIR}/regen/regen.pc DESTINATION lib/pkgconfig)
install(FILES ${CMAKE_BINARY_DIR}/regen/config.h DESTINATION ${HEADER_INSTALL_PATH})
install(FILES application.h DESTINATION ${HEADER_INSTALL_PATH})
