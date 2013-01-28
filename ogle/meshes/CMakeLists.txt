
include_directories(/usr/include/freetype2)

set(SOURCES
        meshes/rectangle.cpp
        meshes/box.cpp
        meshes/sphere.cpp
        meshes/sky.cpp
        meshes/mesh-state.cpp
        meshes/texture-mapped-text.cpp
        meshes/particles.cpp
        meshes/rain-particles.cpp
        meshes/snow-particles.cpp
        meshes/hair-particles.cpp
)

install(
    FILES
        rectangle.h
        box.h
        sky.h
        sphere.h
        mesh-state.h
        texture-mapped-text.h
        particles.h
        rain-particles.h
        snow-particles.h
        hair-particles.h
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
        shader/rain-particles.glsl
    DESTINATION ${SHADER_INSTALL_PATH}
    )
