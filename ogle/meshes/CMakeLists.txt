
include_directories(/usr/include/freetype2)

set(SOURCES
        meshes/attribute-less-mesh.cpp
        meshes/rectangle.cpp
        meshes/box.cpp
        meshes/cone.cpp
        meshes/sphere.cpp
        meshes/sky.cpp
        meshes/mesh-state.cpp
        meshes/texture-mapped-text.cpp
        meshes/particles.cpp
        meshes/particle-cloud.cpp
)

install(
    FILES
        attribute-less-mesh.h
        rectangle.h
        box.h
        cone.h
        sky.h
        sphere.h
        mesh-state.h
        texture-mapped-text.h
        particles.h
        particle-cloud.h
    DESTINATION ${HEADER_INSTALL_PATH}/meshes
    )

install(
    FILES
        shader/sky.glsl
        shader/gui.glsl
        shader/mesh.glsl
        shader/transparent-mesh.glsl
        shader/sprite-sphere.glsl
        shader/volume.glsl
        shader/particles.glsl
        shader/precipitation-particles.glsl
        shader/rain-particles.glsl
        shader/snow-particles.glsl
    DESTINATION ${SHADER_INSTALL_PATH}
    )
