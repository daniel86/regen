
set(SOURCES
       gl-types/buffer-object.cpp
       gl-types/fbo.cpp
       gl-types/rbo.cpp
       gl-types/shader.cpp
       gl-types/tbo.cpp
       gl-types/ubo.cpp
       gl-types/texture.cpp
       gl-types/volume-texture.cpp
       gl-types/uniform.cpp
       gl-types/vao.cpp
       gl-types/vbo.cpp
       gl-types/vertex-attribute.cpp
)
       
install(
    FILES
       buffer-object.h
       fbo.h
       rbo.h
       shader.h
       tbo.h
       ubo.h
       texture.h
       volume-texture.h
       uniform.h
       vao.h
       vbo.h
       vertex-attribute.h
       tesselation-config.h
       geometry-shader-config.h
    DESTINATION ${HEADER_INSTALL_PATH}/gl-types
    )
