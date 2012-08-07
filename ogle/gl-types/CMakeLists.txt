
add_library( gl-types STATIC
       buffer-object.cpp
       fbo.cpp
       rbo.cpp
       shader.cpp
       tbo.cpp
       ubo.cpp
       texture.cpp
       volume-texture.cpp
       uniform.cpp
       vao.cpp
       vbo.cpp
       vertex-attribute.cpp
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
    DESTINATION include/ogle/gl-types
    )
