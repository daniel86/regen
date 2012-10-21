
set(SOURCES 
        animations/animation.cpp
        animations/animation-manager.cpp
        animations/camera-manipulator.cpp
        animations/animation-node.cpp
        animations/mesh-animation.cpp
        animations/texture-update-operation.cpp
        animations/texture-updater.cpp
        )

install(
    FILES
        animation.h
        animation-manager.h
        animation-node.h
        camera-manipulator.h
        mesh-animation.h
        texture-updater.h
        texture-update-operation.h
    DESTINATION ${HEADER_INSTALL_PATH}/animations
    )
