
set(SOURCES
        filter/filter.cpp
        filter/filter-sequence.cpp
        filter/blur.cpp
)

install(
    FILES
        filter.h
        filter-sequence.h
        downsample.h
        blur.h
    DESTINATION ${HEADER_INSTALL_PATH}/states
    )

install(
    FILES
        shader/blur.glsl
        shader/blurSeparable.glsl
        shader/downsample.glsl
        shader/sampling.glsl
    DESTINATION ${SHADER_INSTALL_PATH}
    )
