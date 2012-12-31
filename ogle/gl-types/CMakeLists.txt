
set(SOURCES
       gl-types/buffer-object.cpp
       gl-types/fbo.cpp
       gl-types/rbo.cpp
       gl-types/glsl-directive-processor.cpp
       gl-types/glsl-io-processor.cpp
       gl-types/shader.cpp
       gl-types/shader-input.cpp
       gl-types/tbo.cpp
       gl-types/ubo.cpp
       gl-types/texture.cpp
       gl-types/vao.cpp
       gl-types/vbo.cpp
       gl-types/vertex-attribute.cpp
)
       
install(
    FILES
       buffer-object.h
       fbo.h
       rbo.h
       glsl-directive-processor.h
       glsl-io-processor.h
       shader.h
       shader-input.h
       tbo.h
       ubo.h
       texture.h
       vao.h
       vbo.h
       vertex-attribute.h
    DESTINATION ${HEADER_INSTALL_PATH}/gl-types
    )
