
set(SOURCES 
        animations/animation.cpp
        animations/animation-manager.cpp
        animations/camera-manipulator.cpp
        animations/animation-node.cpp
        animations/mesh-animation.cpp
        animations/bones.cpp
        )

install(
    FILES
        animation.h
        animation-manager.h
        animation-node.h
        bones.h
        camera-manipulator.h
        mesh-animation.h
    DESTINATION ${HEADER_INSTALL_PATH}/animations
    )

install(
    FILES
        shader/mesh-animation.glsl
    DESTINATION ${SHADER_INSTALL_PATH}
    )
