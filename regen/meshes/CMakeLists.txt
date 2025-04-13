
# list of files in this directory
file(GLOB SOURCES *.cpp)
file(GLOB HEADER_FILES *.h)

install(
    FILES ${HEADER_FILES}
    DESTINATION ${HEADER_INSTALL_PATH}/meshes
)

foreach(DIR primitives lod)
    add_subdirectory(${DIR})
    get_directory_property(BUFFER DIRECTORY ${DIR} DEFINITION SOURCES)
    list(APPEND SOURCES ${BUFFER})
endforeach(DIR)
