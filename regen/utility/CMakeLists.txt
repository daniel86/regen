
set(SOURCES
       utility/event-object.cpp
       utility/font.cpp
       utility/logging.cpp
       utility/string-util.cpp
       utility/filesystem.cpp
       )

install(
    FILES
        event-object.h
        font.h
        interfaces.h
        logging.h
        ordered-stack.h
        ref-ptr.h
        stack.h
        string-util.h
        filesystem.h
        threading.h
    DESTINATION ${HEADER_INSTALL_PATH}/utility
)
