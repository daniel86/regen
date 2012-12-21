
set(SOURCES
        textures/texture-loader.cpp
        textures/video-texture.cpp
        textures/spectral-texture.cpp
)

install(
    FILES
        texture-loader.h
        video-texture.h
        volume-texture.h
        spectral-texture.h
    DESTINATION ${HEADER_INSTALL_PATH}/textures
    )
