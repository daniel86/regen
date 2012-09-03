
set(SOURCES
        render-tree/render-tree.cpp
        render-tree/state-node.cpp
        render-tree/picker.cpp
)

install(
    FILES
        render-tree.h
        state-node.h
        picker.h
    DESTINATION ${HEADER_INSTALL_PATH}/render-tree
    )
