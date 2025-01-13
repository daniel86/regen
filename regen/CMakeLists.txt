
set(HEADER_INSTALL_PATH include/${PROJECT_NAME})
set(SHADER_INSTALL_PATH share/${PROJECT_NAME})

configure_file(config.h.cmake config.h)
configure_file(${PROJECT_NAME}.pc.in ${PROJECT_NAME}.pc)

# add a target to generate API documentation with Doxygen
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/doc/doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/doc/doxyfile @ONLY)
    add_custom_target(doc
        ${DOXYGEN_EXECUTABLE}
        ${CMAKE_CURRENT_BINARY_DIR}/doc/doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
    message(STATUS "target doc generates the documentation.")
endif()

enable_testing()
find_package(GTest REQUIRED)

set(SOURCE_DIRS
    math
    av
    gl-types
    utility
    shapes
    animations
    physics
    textures
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
install(FILES ${PROJECT_NAME}.pc DESTINATION lib/pkgconfig)
install(FILES application.h DESTINATION ${HEADER_INSTALL_PATH})
install(FILES config.h DESTINATION ${HEADER_INSTALL_PATH})

##############
########## Unit Testings
##############

# add an executable target for GTest.
# but the testing code is partly in the *knowrob* library
# where gtest won't find them without using the "--no-as-needed"
# flag for the linker.
add_executable(all_gtests
        tests/gtests.cpp
        tests/shapes/quad-tree-test.cpp)
target_link_libraries(all_gtests
        -Wl,--whole-archive,--no-as-needed
        regen
        -Wl,--no-whole-archive
        ${Boost_Python_COMPONENT}
        ${GTEST_MAIN_LIBRARIES})
