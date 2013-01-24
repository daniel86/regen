
set(SOURCES
        render-tree/shader-configurer.cpp
        render-tree/picker.cpp
        render-tree/state-node.cpp
        render-tree/render-tree.cpp
        render-tree/shading-deferred.cpp
        render-tree/blur-node.cpp
)

install(
    FILES
        shader-configurer.h
        picker.h
        state-node.h
        render-tree.h
        shading-deferred.h
        blur-node.h
    DESTINATION ${HEADER_INSTALL_PATH}/render-tree
    )

install(
    FILES
        shader/deferred-shading.glsl
        shader/picking.glsl
        shader/blur.glsl
    DESTINATION ${SHADER_INSTALL_PATH}
    )
