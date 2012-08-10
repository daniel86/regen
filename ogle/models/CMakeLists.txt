
set(SOURCES
        models/quad.cpp
        models/cube.cpp
        models/sphere.cpp
        models/sky-box.cpp
        models/terrain.cpp
        models/text.cpp
)

install(
    FILES
        cube.h
        quad.h
        sky-box.h
        sphere.h
        terrain.h
        text.h
    DESTINATION ${HEADER_INSTALL_PATH}/models
    )
