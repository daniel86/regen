
set(SOURCES
        textures/cube-image-texture.cpp
        textures/video-texture.cpp
        textures/image-texture.cpp
        textures/texel-transfer.cpp
)

install(
    FILES
        cube-image-texture.h
        image-texture.h
        texel-transfer.h
        video-texture.h
        volume-texture.h
    DESTINATION ${HEADER_INSTALL_PATH}/textures
    )
