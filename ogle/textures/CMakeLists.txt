
set(SOURCES
        textures/texture-loader.cpp
        textures/noise-texture.cpp
        textures/texture-update-operation.cpp
        textures/texture-updater.cpp
)

install(
    FILES
        texture-loader.h
        noise-texture.h
        texture-updater.h
        texture-update-operation.h
    DESTINATION ${HEADER_INSTALL_PATH}/textures
    )

install(
    FILES
        shader/downsample.glsl
    DESTINATION ${SHADER_INSTALL_PATH}
    )
