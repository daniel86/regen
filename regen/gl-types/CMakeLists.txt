
set(SOURCES
       gl-types/gl-object.cpp
       gl-types/gl-enum.cpp
       gl-types/gl-util.cpp
       gl-types/fbo.cpp
       gl-types/render-state.cpp
       gl-types/shader.cpp
       gl-types/shader-input.cpp
       gl-types/texture.cpp
       gl-types/ubo.cpp
       gl-types/vbo.cpp
       gl-types/shader-input-container.cpp
       gl-types/glsl/preprocessor.cpp
       gl-types/glsl/directive-processor.cpp
       gl-types/glsl/io-processor.cpp
)

install(
    FILES
        gl-object.h
        gl-util.h
        gl-enum.h
        fbo.h
        render-state.h
        shader.h
        shader-input.h
        texture.h
        ubo.h
        vbo.h
        shader-input-container.h
        glsl/preprocessor.h
        glsl/glsl-processor.h
        glsl/directive-processor.h
        glsl/io-processor.h
    DESTINATION ${HEADER_INSTALL_PATH}/gl-types
)
