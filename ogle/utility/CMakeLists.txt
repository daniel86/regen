
add_library( utility STATIC
       event-object.cpp
       logging.cpp
       perlin.cpp
       timeout-manager.cpp
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
    DESTINATION include/ogle/utility
    )
