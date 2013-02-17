
set(SOURCES
        render-tree/shader-configurer.cpp
        render-tree/picker.cpp
        render-tree/state-node.cpp
        render-tree/render-tree.cpp
)

install(
    FILES
        shader-configurer.h
        picker.h
        state-node.h
        render-tree.h
    DESTINATION ${HEADER_INSTALL_PATH}/render-tree
    )

install(
    FILES
        shader/deferred-shading.glsl
        shader/picking.glsl
    DESTINATION ${SHADER_INSTALL_PATH}
    )
