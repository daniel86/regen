
# list of files in this directory
file(GLOB SOURCES *.cpp)
file(GLOB HEADER_FILES *.h)

install(
    FILES ${HEADER_FILES}
    DESTINATION ${HEADER_INSTALL_PATH}/scenes
)
