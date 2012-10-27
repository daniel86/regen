
set(SOURCES
        render-tree/picker.cpp
        render-tree/state-node.cpp
        render-tree/render-tree.cpp
)

install(
    FILES
        picker.h
        state-node.h
        render-tree.h
    DESTINATION ${HEADER_INSTALL_PATH}/render-tree
    )
