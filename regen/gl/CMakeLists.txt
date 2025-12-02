
# list of files in this directory
file(GLOB SOURCES *.cpp queries/*.cpp states/*.cpp)
file(GLOB HEADER_FILES *.h queries/*.h states/*.h)

install(
    FILES ${HEADER_FILES}
    DESTINATION ${HEADER_INSTALL_PATH}/gl
)
