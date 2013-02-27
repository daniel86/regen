
set(SOURCES
       utility/event-object.cpp
       utility/logging.cpp
       utility/timeout-manager.cpp
       utility/gl-util.cpp
       utility/string-util.cpp
       utility/font-manager.cpp
       utility/free-type.cpp
       )

install(
    FILES
        callable.h
        event-object.h
        gl-util.h
        logging.h
        ordered-stack.h
        ref-ptr.h
        stack.h
        string-util.h
        timeout-manager.h
        font-manager.h
        free-type.h
    DESTINATION ${HEADER_INSTALL_PATH}/utility
    )
