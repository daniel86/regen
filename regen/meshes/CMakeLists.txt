
set(SOURCES
        meshes/assimp-importer.cpp
        meshes/attribute-less-mesh.cpp
        meshes/box.cpp
        meshes/cone.cpp
        meshes/mesh-state.cpp
        meshes/particles.cpp
        meshes/particle-cloud.cpp
        meshes/rectangle.cpp
        meshes/sphere.cpp
        meshes/sky.cpp
        meshes/texture-mapped-text.cpp
)

install(
    FILES
        assimp-importer.h
        attribute-less-mesh.h
        box.h
        cone.h
        mesh-state.h
        particles.h
        particle-cloud.h
        rectangle.h
        sphere.h
        sky.h
        texture-mapped-text.h
    DESTINATION ${HEADER_INSTALL_PATH}/meshes
    )

install(
    FILES
        shader/cloud_particles.glsl
        shader/gui.glsl
        shader/mesh.glsl
        shader/particles.glsl
        shader/sky.glsl
        shader/sprite.glsl
        shader/texture_mapped_text.glsl
        shader/volume.glsl
    DESTINATION ${SHADER_INSTALL_PATH}/meshes/shader
    )
