
set(SOURCES
        textures/cube-image-texture.cpp
        textures/video-texture.cpp
        textures/image-texture.cpp
        textures/spectral-texture.cpp
)

install(
    FILES
        cube-image-texture.h
        image-texture.h
        video-texture.h
        volume-texture.h
        spectral-texture.h
    DESTINATION ${HEADER_INSTALL_PATH}/textures
    )
