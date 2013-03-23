
set(SOURCES
        textures/texture-loader.cpp
        textures/noise-texture.cpp
        textures/texture-updater.cpp
)

install(
    FILES
        texture-loader.h
        noise-texture.h
        texture-updater.h
    DESTINATION ${HEADER_INSTALL_PATH}/textures
    )
