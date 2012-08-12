
set(SOURCES
       utility/event-object.cpp
       utility/logging.cpp
       utility/perlin.cpp
       utility/timeout-manager.cpp
       utility/gl-error.cpp
       )

install(
    FILES
        callable.h
        event-object.h
        gl-error.h
        logging.h
        ordered-stack.h
        perlin.h
        ref-ptr.h
        spectrum.h
        stack.h
        string-util.h
        timeout-manager.h
    DESTINATION ${HEADER_INSTALL_PATH}/utility
    )
