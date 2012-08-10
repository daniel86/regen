
set(SOURCES 
        animations/animation.cpp
        animations/animation-buffer.cpp
        animations/animation-manager.cpp
        animations/camera-manipulator.cpp
        animations/vbo-animation.cpp
        animations/vbo-morph-animation.cpp
        animations/vbo-wave-animation.cpp
        animations/animation-node.cpp)

install(
    FILES
        animation.h
        animation-buffer.h
        animation-manager.h
        animation-node.h
        camera-manipulator.h
        vbo-animation.h
        vbo-morph-animation.h
        vbo-wave-animation.h
    DESTINATION ${HEADER_INSTALL_PATH}/animations
    )
