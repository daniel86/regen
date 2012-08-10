
set(SOURCES
        render-tree/render-tree.cpp
        render-tree/state-node.cpp
)

install(
    FILES
        render-tree.h
        state-node.h
    DESTINATION ${HEADER_INSTALL_PATH}/render-tree
    )
