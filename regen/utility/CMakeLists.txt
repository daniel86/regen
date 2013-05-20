
# list of all source files in this directory
file(GLOB SOURCES *.cpp)

install(
    FILES
        event-object.h
        font.h
        logging.h
        ref-ptr.h
        stack.h
        state-stacks.h
        string-util.h
        filesystem.h
        threading.h
        memory-allocator.h
    DESTINATION ${HEADER_INSTALL_PATH}/utility
)
