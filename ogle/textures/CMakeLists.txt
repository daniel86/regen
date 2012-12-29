
set(SOURCES
        textures/texture-loader.cpp
        textures/video-texture.cpp
        textures/noise-texture.cpp
        textures/raw-texture.cpp
        textures/spectral-texture.cpp
        textures/texture-update-operation.cpp
        textures/texture-updater.cpp
)

install(
    FILES
        texture-loader.h
        video-texture.h
        noise-texture.h
        raw-texture.h
        volume-texture.h
        spectral-texture.h
        texture-updater.h
        texture-update-operation.h
    DESTINATION ${HEADER_INSTALL_PATH}/textures
    )
