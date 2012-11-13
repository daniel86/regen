
set(SOURCES
        render-tree/picker.cpp
        render-tree/state-node.cpp
        render-tree/render-tree.cpp
        render-tree/shading-deferred.cpp
        render-tree/shading-forward.cpp
        render-tree/blur-node.cpp
)

install(
    FILES
        picker.h
        state-node.h
        render-tree.h
        shading-deferred.h
        shading-forward.h
        shading-interface.h
        blur-node.h
    DESTINATION ${HEADER_INSTALL_PATH}/render-tree
    )
