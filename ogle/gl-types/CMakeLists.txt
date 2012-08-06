
add_library( gl-types STATIC
       buffer-object.cpp
       fbo.cpp
       rbo.cpp
       shader.cpp
       tbo.cpp
       texture.cpp
       volume-texture.cpp
       uniform.cpp
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
       texture.h
       volume-texture.h
       uniform.h
       vbo.h
       vertex-attribute.h
       tesselation.h
    DESTINATION include/ogle/gl-types
    )
