
set(SOURCES
        meshes/rectangle.cpp
        meshes/box.cpp
        meshes/sphere.cpp
        meshes/sky.cpp
        meshes/particles.cpp
        meshes/mesh-state.cpp
)

install(
    FILES
        rectangle.h
        box.h
        sky.h
        sphere.h
        particles.h
        mesh-state.h
    DESTINATION ${HEADER_INSTALL_PATH}/models
    )

install(
    FILES
        shader/sky.glsl
        shader/particles.glsl
        shader/gui.glsl
        shader/mesh.glsl
        shader/transparent-mesh.glsl
        shader/sprite-sphere.glsl
        shader/volume.glsl
    DESTINATION ${SHADER_INSTALL_PATH}
    )
