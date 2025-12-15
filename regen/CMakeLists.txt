
set(HEADER_INSTALL_PATH include/${PROJECT_NAME})
set(SHADER_INSTALL_PATH share/${PROJECT_NAME})

configure_file(config.h.cmake ${CMAKE_BINARY_DIR}/regen/config.h)
configure_file(${PROJECT_NAME}.pc.in ${CMAKE_BINARY_DIR}/regen/${PROJECT_NAME}.pc)
message(STATUS "Installing config.h to ${CMAKE_BINARY_DIR}/regen/config.h")

set(SOURCE_DIRS
    compute
    gl
    shader
    memory
    utility
    shapes
    animation
    simulation
    behavior
    textures
    shading
    camera
    objects
    passes
    scene
    external)

if (HAS_AV_LIBS)
    list(APPEND SOURCE_DIRS av)
else ()
    message(STATUS "AV libraries not found, excluding AV source directories")
endif()

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

add_custom_target(gen_h
    DEPENDS ${CMAKE_BINARY_DIR}/regen/config.h)
add_dependencies(${PROJECT_NAME} gen_h)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
install(FILES ${CMAKE_BINARY_DIR}/regen/regen.pc DESTINATION lib/pkgconfig)
install(FILES ${CMAKE_BINARY_DIR}/regen/config.h DESTINATION ${HEADER_INSTALL_PATH})
install(FILES regen.h DESTINATION ${HEADER_INSTALL_PATH})
