
set(SOURCES
       utility/event-object.cpp
       utility/font.cpp
       utility/logging.cpp
       utility/string-util.cpp
       utility/filesystem.cpp
       utility/threading.cpp
       utility/memory-allocator.cpp
       utility/xml.cpp
       )

install(
    FILES
        event-object.h
        font.h
        logging.h
        ref-ptr.h
        stack.h
        string-util.h
        filesystem.h
        threading.h
        memory-allocator.h
    DESTINATION ${HEADER_INSTALL_PATH}/utility
)
