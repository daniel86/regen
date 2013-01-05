
set(SOURCES
       utility/event-object.cpp
       utility/logging.cpp
       utility/timeout-manager.cpp
       utility/gl-error.cpp
       utility/string-util.cpp
       )

install(
    FILES
        callable.h
        event-object.h
        gl-error.h
        logging.h
        ordered-stack.h
        ref-ptr.h
        stack.h
        string-util.h
        timeout-manager.h
    DESTINATION ${HEADER_INSTALL_PATH}/utility
    )
