
set(SOURCES
        shader/glsl-types.cpp
        shader/shader-configuration.cpp
        shader/shader-function.cpp
        shader/shader-manager.cpp
        shader/shader-generator.cpp
        shader/normal-mapping.cpp
        shader/light-shader.cpp
        shader/blending-shader.cpp
        shader/filter-shader.cpp
        shader/utility-shader.cpp
        shader/debugging-shader.cpp
        shader/raycast-shader.cpp
        shader/tesselation-shader.cpp
        shader/anti-aliasing.cpp
)

install(
    FILES
        glsl-type.h
        shader-configuration.h
        shader-fragment-output.h
        shader-function.h
        shader-generator.h
        shader-manager.h
        blending-shader.h
        debugging-shader.h
        filter-shader.h
        light-shader.h
        raycast-shader.h
        tesselation-shader.h
        utility-shader.h
        normal-mapping.h
        anti-aliasing.h
    DESTINATION ${HEADER_INSTALL_PATH}/shader
    )
