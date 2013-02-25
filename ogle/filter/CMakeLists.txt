
set(SOURCES
        filter/filter.cpp
)

install(
    FILES
        filter.h
    DESTINATION ${HEADER_INSTALL_PATH}/states
    )

install(
    FILES
        shader/blur.glsl
        shader/downsample.glsl
        shader/sampling.glsl
    DESTINATION ${SHADER_INSTALL_PATH}
    )
