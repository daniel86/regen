
set(SOURCES
       utility/event-object.cpp
       utility/font-manager.cpp
       utility/free-type.cpp
       utility/gl-util.cpp
       utility/logging.cpp
       utility/string-util.cpp
       utility/filesystem.cpp
       )

install(
    FILES
        event-object.h
        font-manager.h
        free-type.h
        gl-util.h
        interfaces.h
        logging.h
        ordered-stack.h
        ref-ptr.h
        stack.h
        string-util.h
        filesystem.h
    DESTINATION ${HEADER_INSTALL_PATH}/utility
    )
